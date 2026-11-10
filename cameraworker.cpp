// ===== cameraworker.cpp =====
// 摄像头线程：常开抓帧、跳帧 + 引导框裁剪，把帧交给识别线程。

#include "cameraworker.h"
#include <QDebug>
#include <QImage>

CameraWorker::CameraWorker(QObject *parent)
    : QObject{parent}
{
    // 接红外传感器；当前先只负责"摄像头常开 + 抓帧预览"
    //模拟有红外传感器
    this->readSensor();//定时器模拟5秒假装读到一次距离，真机改成读Linux驱动
}

void CameraWorker::openCamera()
{
    // 已经在采集就不重复打开，避免重复创建定时器、重复抢占设备
    if (this->cap.isOpened())
        return;

    // 本机只有一个 USB 摄像头 -> 编号 0
    // 驱动异常时 open 也会抛 cv::Exception，子线程里不接就是整个进程退出
    bool opened = false;
    try {
        opened = this->cap.open(0, cv::CAP_DSHOW);
    } catch (const cv::Exception &e) {
        qDebug() << "[openCamera] open 抛异常:" << e.what();
    }

    if (!opened || !this->cap.isOpened()) {
        // 摄像头可能刚被别的程序释放，驱动回收句柄有延迟，短暂重试几次
        if (++m_openRetry <= 5) {
            qDebug() << "摄像头打开失败，第" << m_openRetry << "次重试";
            QTimer::singleShot(300, this, [this]() { this->openCamera(); });
        } else {
            m_openRetry = 0;
            qDebug() << "摄像头打开失败，放弃重试（可能被其他程序占用）";
        }
        return;
    }
    m_openRetry = 0;

    // 定时器只创建一次，之后复用
    if (!this->cameraTimer) {
        this->cameraTimer = new QTimer(this);
        connect(this->cameraTimer,&QTimer::timeout,this,&CameraWorker::grabFrame);
    }

    // 每隔30ms左右对捕获到的帧做处理
    this->cameraTimer->start(1000/30);
}

void CameraWorker::closeCamera()
{
    qDebug() << "[closeCamera] 开始释放摄像头";

    // 只 stop，不 deleteLater：定时器留着下次 openCamera 直接复用，
    // 避免“边删边用”的野指针风险
    if (this->cameraTimer)
        this->cameraTimer->stop();

    // 释放设备：不 release 的话录入程序永远打不开摄像头
    // 注意：release 在驱动异常时会抛 cv::Exception，子线程里没人接
    // 就会 std::terminate，表现就是“程序直接没了、什么都不打印”
    if (this->cap.isOpened()) {
        try {
            this->cap.release();
        } catch (const cv::Exception &e) {
            qDebug() << "[closeCamera] release 抛异常:" << e.what();
        }
    }

    // 清掉识别状态，避免恢复时还停在上一轮
    this->is_processing = false;
    this->isRecoginze = false;
    m_openRetry = 0;
    qDebug() << "[closeCamera] 完成，设备已释放";

    // 通知主线程：设备真的放掉了，可以启动录入程序
    emit this->cameraClosed();
}

void CameraWorker::grabFrame()
{
    // 设备已释放就别再读
    if (!this->cap.isOpened())
        return;

    // 捕获帧：设备被抢占/驱动异常时 OpenCV 会抛异常，必须兜住
    cv::Mat frame;
    try {
        this->cap >> frame;
    } catch (const cv::Exception &e) {
        qDebug() << "[grabFrame] 抓帧抛异常:" << e.what();
        return;
    }
    if(frame.empty())
    {
        qDebug() <<"摄像头打开失败!";
        return;
    }

    // 跳帧：每5帧检测一次，没有必要每一帧都去获取引导框内的
    int seq = m_frameCounter.fetchAndAddOrdered(1);
    //this->isRecoginze为真，有人靠近，开始做人脸检测 并且当前没有帧在做处理，才做后续逻辑
    if(seq%5==0 && !is_processing && isRecoginze){
        // 帧处理，裁剪引导框内的人脸图像
        int cropW = frame.cols * 0.4;
        int cropH = cropW * 4 / 3; // 3:4 → 高 = 宽 * 4/3
        // 保护：防止超出帧高
        if (cropH > frame.rows) {
            cropH = frame.rows;
            cropW = cropH * 3 / 4;
        }
        int cropX = (frame.cols - cropW) / 2;
        int cropY = (frame.rows - cropH) / 2;
        cv::Rect roi(cropX, cropY, cropW, cropH);
        cv::Mat faceRegion = frame(roi).clone();

        emit this->handleFrame(faceRegion);
        is_processing = true;  // 表明当前正在处理帧
    }

    // BGR -> RGB
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    // 将Mat转为QImage,方便页面展示
    QImage image(frame.data,frame.cols,frame.rows,frame.step,QImage::Format_RGB888);

    // 发信号，通知页面做展示
    emit frameReady(image.copy());
}

void CameraWorker::frameHandleFinished(bool isFinished)
{
    this->is_processing = false;
    if(isFinished)
    {
        //说明本次人脸识别结束了
        isRecoginze = false;
    }
}

void CameraWorker::readSensor(){
    // 以 this 为父对象：定时器会跟着 CameraWorker 一起搬到子线程，
    // 否则它会留在 GUI 线程，跨线程读写 isRecoginze
    this->recogTimer=new QTimer(this);
    connect(this->recogTimer,&QTimer::timeout,[this](){
        //1.调用Linux驱动获取距离，模拟有人
        int distance=50;
        if(distance<100){
             //如果距离小于100，有东西来了
            qDebug()<<"有人来了！";
             //弱光环境开启补光灯
            qDebug()<<"(模拟)弱光环境，打开补光灯！)";
            //要检测了，修改状态
            this->isRecoginze=true;
        }
    });
    //启动定时器，每5秒触发一次
    this->recogTimer->start(5000);
}
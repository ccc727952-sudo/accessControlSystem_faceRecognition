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
    // 打开摄像头
    // 本机只有一个 USB 摄像头 -> 编号 0
    cap.open(0, cv::CAP_DSHOW);

    // 每隔30ms左右对捕获到的帧做处理
    this->cameraTimer = new QTimer;
    connect(this->cameraTimer,&QTimer::timeout,this,&CameraWorker::grabFrame);

    this->cameraTimer->start(1000/30);
}

void CameraWorker::grabFrame()
{
    // 捕获帧
    cv::Mat frame;
    this->cap >> frame;
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
    this->recogTimer=new QTimer;
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
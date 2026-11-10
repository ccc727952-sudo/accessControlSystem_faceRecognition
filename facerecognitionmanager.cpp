// ===== facerecognitionmanager.cpp =====
// 门面类：两条 QThread + QVideoSink，把摄像头帧送进识别线程。

#include "facerecognitionmanager.h"
#include <QVideoFrameFormat>
#include <QVideoFrame>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>

// 编译期兜底路径：由 CMakeLists.txt 的 target_compile_definitions 传入
#ifndef FACE_INPUT_EXE
#define FACE_INPUT_EXE ""
#endif

FaceRecognitionManager::FaceRecognitionManager(QObject *parent)
    : QObject{parent}
{
    this->cameraWorker = new CameraWorker;
    this->cameraWorkerThread = new QThread;
    this->cameraWorker->moveToThread(this->cameraWorkerThread);

    this->recogWorker = new FaceRecognitionHandleWorker;
    this->recogWorkerThread = new QThread;
    this->recogWorker->moveToThread(this->recogWorkerThread);
    //采集->识别
    connect(this->cameraWorker,&CameraWorker::handleFrame,this->recogWorker,&FaceRecognitionHandleWorker::handleFaceRecognition);
    //识别完成->采集
    connect(this->recogWorker,&FaceRecognitionHandleWorker::frameHandleFinished,this->cameraWorker,&CameraWorker::frameHandleFinished);
    connect(this->cameraWorker,&CameraWorker::frameReady,this,&FaceRecognitionManager::showFrame);
    connect(this,&FaceRecognitionManager::notifyOpenCamera,this->cameraWorker,&CameraWorker::openCamera);


    this->cameraWorkerThread->start();
    this->recogWorkerThread->start();
}

// 启动独立的人脸录入程序
// 用 startDetached：录入程序作为独立进程运行，不阻塞主程序，
// 关掉主程序也不会连带杀掉录入窗口。
bool FaceRecognitionManager::launchFaceInput()
{
    // 路径优先级：环境变量 > 编译期宏，方便不重新编译就换路径
    QString exePath = qEnvironmentVariable("FACE_INPUT_EXE");
    if (exePath.isEmpty())
        exePath = QStringLiteral(FACE_INPUT_EXE);

    if (exePath.isEmpty() || !QFileInfo::exists(exePath)) {
        qWarning() << "人脸录入程序不存在，请先编译 faceInput 工程:" << exePath;
        return false;
    }

    // 工作目录设为 exe 所在目录，避免程序内部用相对路径找资源时出错
    const QString workDir = QFileInfo(exePath).absolutePath();
    const bool ok = QProcess::startDetached(exePath, QStringList(), workDir);

    if (ok)
        qDebug() << "人脸录入程序已启动:" << exePath;
    else
        qWarning() << "人脸录入程序启动失败:" << exePath;

    return ok;
}

void FaceRecognitionManager::openCamera()
{
    emit this->notifyOpenCamera();
}

QVideoSink *FaceRecognitionManager::displaySink()
{
    return this->m_displaySink;
}

void FaceRecognitionManager::setDisplaySink(QVideoSink *sink)
{
    this->m_displaySink = sink;
    emit displaySinkChanged();
}

void FaceRecognitionManager::showFrame(const QImage &image)
{
    if (!m_displaySink || image.isNull()) return;

    QVideoFrameFormat fmt(image.size(), QVideoFrameFormat::Format_RGBX8888);
    QVideoFrame frame(fmt);
    if (!frame.map(QVideoFrame::WriteOnly)) return;

    QImage converted = image.convertToFormat(QImage::Format_RGBX8888);

    // 关键：单平面格式，一次性拷贝整个缓冲区
    memcpy(frame.bits(0), converted.constBits(), converted.sizeInBytes());

    frame.unmap();
    m_displaySink->setVideoFrame(frame);
}
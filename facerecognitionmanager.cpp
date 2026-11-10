// ===== facerecognitionmanager.cpp =====
// 门面类：两条 QThread + QVideoSink，把摄像头帧送进识别线程。

#include "facerecognitionmanager.h"
#include <QVideoFrameFormat>
#include <QVideoFrame>

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
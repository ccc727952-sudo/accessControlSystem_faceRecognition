// ===== facerecognitionmanager.h =====
// 门面类声明：Q_PROPERTY(displaySink) 供 QML 绑定。

#ifndef FACERECOGNITIONMANAGER_H
#define FACERECOGNITIONMANAGER_H

#include <QObject>
#include <QVideoSink>
#include "cameraworker.h"
#include <QThread>
#include "facerecognitionhandleworker.h"

class FaceRecognitionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink* displaySink READ displaySink WRITE setDisplaySink NOTIFY displaySinkChanged)
public:
    explicit FaceRecognitionManager(QObject *parent = nullptr);

    Q_INVOKABLE void openCamera();

    QVideoSink* displaySink();
    void setDisplaySink(QVideoSink* sink);
signals:
    void displaySinkChanged();   // ← 必须手动声明

    void notifyOpenCamera();

public slots:
    void showFrame(const QImage& image);

private:
    QVideoSink *m_displaySink = nullptr;
    CameraWorker *cameraWorker = nullptr;
    QThread *cameraWorkerThread = nullptr;
    //识别对象和识别子线程
    FaceRecognitionHandleWorker *recogWorker=nullptr;
    QThread *recogWorkerThread=nullptr;
};

#endif // FACERECOGNITIONMANAGER_H
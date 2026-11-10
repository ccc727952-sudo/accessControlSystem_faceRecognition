/* 人脸识别管理类，包含人脸识别处理子线程，摄像头子线程，人脸录入子线程，MQTT 子线程，数据库子线程
，人脸数据子线程，人脸特征子线程，人脸识别子线程，人脸检测子线程 */
// ===== facerecognitionmanager.h =====
// 门面类声明：Q_PROPERTY(displaySink) 供 QML 绑定。

#ifndef FACERECOGNITIONMANAGER_H
#define FACERECOGNITIONMANAGER_H

#include <QObject>
#include <QVideoSink>
#include "cameraworker.h"
#include <QThread>
#include "facerecognitionhandleworker.h"

class QProcess;// 人脸录入程序进程，用于启动 faceInput 工程的独立进程

class FaceRecognitionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink* displaySink READ displaySink WRITE setDisplaySink NOTIFY displaySinkChanged)
public:
    explicit FaceRecognitionManager(QObject *parent = nullptr);

    Q_INVOKABLE void openCamera();//打开摄像头，启动摄像头子线程

    // 启动独立的人脸录入程序（QProcess 分离进程），返回是否启动成功
    Q_INVOKABLE bool launchFaceInput();

    QVideoSink* displaySink();
    void setDisplaySink(QVideoSink* sink);
signals:
    void displaySinkChanged();   // ← 必须手动声明

    void notifyOpenCamera();
    void notifyCloseCamera();   // 通知摄像头线程释放设备，让给人脸录入程序

public slots:
    void showFrame(const QImage& image);

    // 收到“摄像头已真正释放”的信号后再启动录入程序，替代固定 500ms 猜时间
    void startPendingFaceInput();

private:
    // 真正把录入程序拉起来：设置工作目录 + start
    void startFaceInputProcess(const QString &exePath);

    QVideoSink *m_displaySink = nullptr;
    CameraWorker *cameraWorker = nullptr;
    QThread *cameraWorkerThread = nullptr;
    //识别对象和识别子线程
    FaceRecognitionHandleWorker *recogWorker=nullptr;
    QThread *recogWorkerThread=nullptr;

    // 人脸录入程序进程：用非 detached 方式启动，才能收到"已退出"通知
    QProcess *m_faceInputProcess = nullptr;
    // 等摄像头释放期间暂存的 exe 路径，为空表示没有待启动任务
    QString m_pendingFaceInputExe;
};

#endif // FACERECOGNITIONMANAGER_H
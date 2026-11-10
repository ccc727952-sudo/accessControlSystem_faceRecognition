// ===== cameraworker.h =====
// 摄像头线程类声明：抓帧 / 跳帧 / 处理中标志。

#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <QTimer>

class CameraWorker : public QObject
{
    Q_OBJECT
public:
    explicit CameraWorker(QObject *parent = nullptr);

    void readSensor();
signals:
    void frameReady(const QImage& image);
    void handleFrame(const cv::Mat& frame);
    void cameraClosed();   // 设备已真正释放，可以安全启动人脸录入程序

public slots:
    void openCamera();
    // 释放摄像头：USB 摄像头是独占设备，要让给别人才关得掉
    void closeCamera();
    void grabFrame();
    void frameHandleFinished(bool isFinished);

private:
    cv::VideoCapture cap;
    QTimer *cameraTimer = nullptr;
    int m_openRetry = 0; // 打开失败的重试计数，驱动释放句柄需要时间
    QTimer *recogTimer;
    bool isRecoginze = false;
    bool is_processing = false; // 是否有人正在做识别，要等前一帧处理完再处理下一帧
    // 计数器，做跳帧处理的
    QAtomicInt m_frameCounter;
};

#endif // CAMERAWORKER_H
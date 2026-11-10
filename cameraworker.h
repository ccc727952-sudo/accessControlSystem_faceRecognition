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

public slots:
    void openCamera();
    void grabFrame();
    void frameHandleFinished(bool isFinished);

private:
    cv::VideoCapture cap;
    QTimer *cameraTimer;
    QTimer *recogTimer;
    bool isRecoginze = false;
    bool is_processing = false; // 是否有人正在做识别，要等前一帧处理完再处理下一帧
    // 计数器，做跳帧处理的
    QAtomicInt m_frameCounter;
};

#endif // CAMERAWORKER_H
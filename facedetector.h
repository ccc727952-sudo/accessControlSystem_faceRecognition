// ===== facedetector.h =====
// YOLO 检测器声明与 FaceDetection 结果结构。

#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <QObject>
#include <QImage>
#include <QRectF>
#include <QVector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct FaceDetection {
    cv::Rect box;        // 在输入图像坐标系下的矩形
    float confidence;    // 置信度
};

class FaceDetector
{
public:
    FaceDetector();
    ~FaceDetector();

    // 加载 ONNX 模型，inputSize 必须与导出模型时一致（YOLOv5 通常是 640）
    bool load(const QString &onnxPath, int inputSize = 640);

    // 对 BGR 图像做检测，返回所有检测到的人脸
    QVector<FaceDetection> detect(const cv::Mat &bgrImage,
                                  float confThreshold = 0.25f,
                                  float nmsThreshold = 0.45f);

private:
    cv::dnn::Net m_net;
    int m_inputSize = 640;
    bool m_loaded = false;

    // YOLOv5 后处理：解码一个尺度的输出
    void decodeOutput(const cv::Mat &output, int stride,
                      const cv::Size &inputSize,
                      const cv::Size &originalSize,
                      float confThreshold,
                      QVector<cv::Rect> &boxes,
                      QVector<float> &confidences);

    // 通用 NMS
    void nms(QVector<cv::Rect> &boxes, QVector<float> &confidences,
             float nmsThreshold);
};

#endif // FACEDETECTOR_H
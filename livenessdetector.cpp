// ===== livenessdetector.cpp =====
// 静默活体：MiniFASNetV2 ONNX，80x80 输入，输出 3 类 softmax 取"真人"概率。

#include "livenessdetector.h"
#include <QDebug>
#include <algorithm>
#include "modelpaths.h"

LivenessDetector::LivenessDetector()
{
    try {
        m_net = cv::dnn::readNetFromONNX(ModelPaths::miniFAS().toStdString());
        if (m_net.empty()) {
            qDebug() << "LivenessDetector: 模型加载失败（网络为空）";
        }
        qDebug() << "LivenessDetector: MiniFASNetV2 加载成功";
    } catch (const cv::Exception& e) {
        qDebug() << "LivenessDetector: 加载异常:" << e.what();
    }
}

cv::Rect LivenessDetector::expandCropRect(const cv::Rect& faceRect,
                                          const cv::Size& frameSize,
                                          float scale)
{
    int cx = faceRect.x + faceRect.width / 2;
    int cy = faceRect.y + faceRect.height / 2;

    int cropW = static_cast<int>(faceRect.width * scale);
    int cropH = static_cast<int>(faceRect.height * scale);

    int x = cx - cropW / 2;
    int y = cy - cropH / 2;

    // 边界保护
    x = std::max(0, x);
    y = std::max(0, y);
    cropW = std::min(cropW, frameSize.width - x);
    cropH = std::min(cropH, frameSize.height - y);

    return cv::Rect(x, y, cropW, cropH);
}

float LivenessDetector::predict(const cv::Mat& frame, const cv::Rect& faceRect)
{

    // MiniFASNetV2 标准裁剪倍数为 2.7
    cv::Rect cropRect = expandCropRect(faceRect, frame.size(), 2.7f);
    if (cropRect.width <= 0 || cropRect.height <= 0) return 0.0f;

    cv::Mat faceCrop = frame(cropRect).clone();

    // 预处理：BGR，80x80
    // 默认：原始像素 [0,255]（实测有效的版本）
    // 如果结果异常，注释下面一行，启用下一行：
    // cv::Mat blob = cv::dnn::blobFromImage(faceCrop, 1.0/255.0, cv::Size(80, 80), cv::Scalar(), true, false);
    cv::Mat blob = cv::dnn::blobFromImage(faceCrop, 1.0, cv::Size(80, 80), cv::Scalar(), true, false);

    m_net.setInput(blob);
    cv::Mat output = m_net.forward();  // [1, 3]

    // 手动计算 softmax
    cv::Mat prob(1, 3, CV_32F);
    float maxVal = 0;
    for (int i = 0; i < 3; ++i)
        maxVal = std::max(maxVal, output.at<float>(0, i));

    float sum = 0;
    for (int i = 0; i < 3; ++i) {
        prob.at<float>(0, i) = std::exp(output.at<float>(0, i) - maxVal);
        sum += prob.at<float>(0, i);
    }
    for (int i = 0; i < 3; ++i)
        prob.at<float>(0, i) /= sum;

    float liveScore = prob.at<float>(0, 1);  // 真人概率，范围 [0, 1]
    return liveScore;
}
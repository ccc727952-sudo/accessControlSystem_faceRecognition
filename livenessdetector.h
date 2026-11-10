// ===== livenessdetector.h =====
// 静默活体检测器声明。

#ifndef LIVENESSDETECTOR_H
#define LIVENESSDETECTOR_H

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

class LivenessDetector
{
public:
    LivenessDetector();
    ~LivenessDetector() = default;

    // 对整帧 + 人脸框做活体判断
    // 返回真人概率 [0,1]，调用前需确保 isLoaded() 为 true
    float predict(const cv::Mat& frame, const cv::Rect& faceRect);

private:
    cv::dnn::Net m_net;

    // 按人脸框中心裁剪，scale 为扩展倍数
    cv::Rect expandCropRect(const cv::Rect& faceRect,
                            const cv::Size& frameSize,
                            float scale);
};

#endif // LIVENESSDETECTOR_H
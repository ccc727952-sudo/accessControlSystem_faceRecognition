#ifndef FACERECOGNITIONHANDLEWORKER_H
#define FACERECOGNITIONHANDLEWORKER_H

#include <QObject>
#include <QDebug>
#include <vector>
#include <opencv2/opencv.hpp>
#include "facedetector.h"
#include "modelpaths.h"
#include "livenessdetector.h"
#include "dlibutil.h"
#include "facedatadao.h"
//状态枚举
static enum{
    START,FACE_DETECTED,LIVENESS_DETECTED,FEATURES_EXTRACTED
}work_state=START;

//主界面负责显示，人脸识别线程负责识别  YOLO+MiniFASNetv2 + dilb ResNet
class FaceRecognitionHandleWorker : public QObject
{
    Q_OBJECT
public:
    explicit FaceRecognitionHandleWorker(QObject *parent = nullptr);

signals:
    //一帧处理完通知CameraWorker true标识本次识别已结束
    void frameHandleFinished(bool isFinished);
public slots:
    void handleFaceRecognition(const cv::Mat& frame);
private:
    // 本地库没命中时，把特征经 MQTT 发给后端做云端识别，最长等待 timeoutMs 毫秒
    // 返回是否放行；matchedName 非空时写回后端识别出的姓名
    bool recognizeOnCloud(const std::vector<float>& features, QString *matchedName = nullptr);

    //128维
    std::vector<float> features;
    //人脸检测器
    FaceDetector *faceDetector=nullptr;
    //静默活体检测
    LivenessDetector *livenessDetector=nullptr;
    //人脸特征提取
    DlibUtil* dlibUtil=nullptr;
    //查询得到余弦相似度，人脸比较结果
    FaceDataDao faceDataDao;
};

#endif // FACERECOGNITIONHANDLEWORKER_H

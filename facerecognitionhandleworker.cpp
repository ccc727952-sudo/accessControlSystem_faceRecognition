// ===== facerecognitionhandleworker.cpp =====
// 人脸识别处理子线程：负责处理摄像头帧，调用人脸检测器、活体检测器、特征提取工具、数据库操作。
// 人脸检测器：使用 YOLOv8 模型，检测人脸位置。
// 活体检测器：使用 LivenessNet 模型，判断是否为活体。
// 特征提取工具：使用 Dlib 模型，提取人脸特征。
// 数据库操作：使用 SQLite 数据库，存储人脸特征和匹配结果。
//
#include "facerecognitionhandleworker.h"
#include "speechutils.h"
#include "mqttutil.h"
#include <QEventLoop>
#include <QMetaObject>
#include <QTimer>
FaceRecognitionHandleWorker::FaceRecognitionHandleWorker(QObject *parent)
    : QObject{parent}
{
        //创建人脸检测器
    this->faceDetector=new FaceDetector;
    if(!this->faceDetector->load(ModelPaths::yoloFace(),640)){
        qDebug()<<"人脸模型加载失败！";
        return;
    }
    //活体检测
    this->livenessDetector=new LivenessDetector;
    //人脸特征提取工具
    this->dlibUtil=new DlibUtil(ModelPaths::landmarks());
}

void FaceRecognitionHandleWorker::handleFaceRecognition(const cv::Mat& frame){
    bool isFinished=false;
    auto faces=this->faceDetector->detect(frame,0.25f,0.45f);
    //本帧没有检测到人脸，回到七点，避免越界错误
    if(faces.empty()){
        work_state=START;
        emit this->frameHandleFinished(false);
        return;
    }
    switch (work_state) {
    case START:{
        if(faces.size()>0){
            qDebug()<<"人脸检测通过";
            work_state=FACE_DETECTED;
        }
        break;
    }
    case FACE_DETECTED:{
        float liveScore=this->livenessDetector->predict(frame,faces[0].box);
        if(liveScore>0.5f){
            qDebug()<<"活体检测通过......";
            work_state=LIVENESS_DETECTED;
        }
        break;
    }
    case LIVENESS_DETECTED:{
        this->features=this->dlibUtil->getFaceFeatures(frame,faces[0].box);
        if(!this->features.empty()){
            qDebug()<<"人脸特征提取成功......";
            work_state=FEATURES_EXTRACTED;
        }
        break;
    }
    case FEATURES_EXTRACTED:{
        //余弦距离范围 [0,2]，0 表示方向完全一致，2表示完全相反，对于人脸特征同一个人的不同照片之间余弦相似度通常再0.5以上
        auto match=this->faceDataDao.findNearest(features);
        if(match)qDebug()<<"match->distance:"<<match->distance;
        if(match.has_value() && match->distance<0.04f){
            qDebug()<<"识别成功"<<match->name<<"距离"<<match->distance;
            SpeechUtils::getInstance()->speek("识别通过，请通行!");
            work_state=START;
            isFinished=true;
        }else{
            //本地没有匹配上：把 128 维特征经 MQTT 发给后端，等云端识别结果
            if(this->recognizeOnCloud(features)){
                qDebug()<<"云端识别通过";
                SpeechUtils::getInstance()->speek("云端识别通过，请通行!",3000);
                isFinished=true;
            }else{
                qDebug()<<"识别失败，请录入人脸";
                SpeechUtils::getInstance()->speek("识别失败，请录入人脸!",3000);
            }
            work_state=START;//本轮结束，重新走状态机
        }
        break;
    }
    //下一个状态
    default:
        break;
    }
    emit this->frameHandleFinished(isFinished);
    // qDebug()<<"[识别线程]：收到一帧"<<frame.cols<<"x"<<frame.rows;
    // //状态机
    // emit this->frameHandleFinished(false);
}

// 云端识别：发布特征 -> 阻塞等响应（最多 3 秒），超时/未连接一律判负
bool FaceRecognitionHandleWorker::recognizeOnCloud(const std::vector<float> &features)
{
    if (features.empty())
        return false;

    // 128 个 float 的原始字节（512 字节）转 Base64，与后端约定 features 字段为 Base64 字符串
    const QByteArray raw(reinterpret_cast<const char *>(features.data()),
                         static_cast<int>(features.size() * sizeof(float)));

    MqttUtil &mqtt = MqttUtil::getInstance();

    // MQTT 对象活在 GUI 线程：用 BlockingQueuedConnection 借用它的线程发布请求，
    // 这里只会阻塞识别线程，界面和预览不受影响
    bool sent = false;
    const bool invoked = QMetaObject::invokeMethod(
        &mqtt, "requestCloudRecognition",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, sent),
        Q_ARG(QByteArray, raw.toBase64()));

    if (!invoked || !sent) {
        qDebug() << "云端识别请求发送失败（MQTT 未连接？）";
        return false;
    }

    bool passed = false;
    bool received = false;

    // 在识别线程里跑局部事件循环，等主线程把响应通过信号投递过来
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    const QMetaObject::Connection conn = connect(
        &mqtt, &MqttUtil::cloudRecognitionResult, this,
        [&](bool ok) {
            passed = ok;
            received = true;
            loop.quit();
        });// 云端识别结果信号槽，处理响应并退出事件循环
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);// 3 秒后超时

    timer.start(3000);
    loop.exec();// 阻塞等响应（最多 3 秒），超时/未连接一律判负
    disconnect(conn);// 断开信号槽连接，防止重复触发

    if (!received) {
        qDebug() << "云端识别超时";
        return false;
    }

    return passed;
}


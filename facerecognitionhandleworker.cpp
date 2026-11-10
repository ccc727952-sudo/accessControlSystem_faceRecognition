#include "facerecognitionhandleworker.h"
#include "speechutils.h"
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
        if(match.has_value() && match->distance<0.5f){
            qDebug()<<"识别成功"<<match->name<<"距离"<<match->distance;
            SpeechUtils::getInstance()->speek("识别通过，请通行!");
            work_state=START;
            isFinished=true;
        }else{
            //本地没有匹配上，发送HTTP请求到云端再认一次
            //暂时没有写云端，所以模拟直接通过
            bool successForClound=true;
            if(successForClound){
                qDebug()<<"云端识别通过，以及发送串口消息打开闸门";
                SpeechUtils::getInstance()->speek("识别通过，请通行!",3000);
                isFinished=true;
            }else{
                qDebug()<<"识别失败，请录入人脸";
                SpeechUtils::getInstance()->speek("识别失败，请录入人脸!",3000);
                work_state=START;//本轮结束，重新走状态机
            }
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


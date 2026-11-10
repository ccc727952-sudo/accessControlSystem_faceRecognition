// ===== dlibutil.cpp =====
// dlib：68 关键点 -> 对齐 150x150 face chip -> ResNet 128 维描述子。

#include "dlibutil.h"
#include "modelpaths.h"

DlibUtil::DlibUtil(const QString &landmarkModelPath,QObject *parent)
    : QObject{parent}
{
    // 加载dlib模型文件
    dlib::deserialize(landmarkModelPath.toStdString()) >> m_sp;

    dlib::deserialize(ModelPaths::resnet().toStdString()) >> net;
}

void DlibUtil::showLandmarks(const cv::Mat &image, const cv::Rect &faceBox)
{
    // 裁剪到图像范围内，防止越界
    cv::Rect safeBox = faceBox & cv::Rect(0, 0, image.cols, image.rows);
    if (safeBox.area() <= 0) {
        qDebug() << "坐标越界!";
        return;
    };

    try {
        dlib::cv_image<dlib::bgr_pixel> dlibImg(image);
        // 将opencv的坐标转为dlib的坐标对象
        dlib::rectangle dlibRect = toDlibRect(safeBox);

        dlib::full_object_detection landmarks = m_sp(dlibImg, dlibRect);

        // 图上绘制坐标点
        for (int i = 0; i < 68; ++i) {
            cv::circle(image,
                       cv::Point(landmarks.part(i).x(), landmarks.part(i).y()),
                       2, cv::Scalar(0, 255, 0), -1); // 画绿色圆点
            cv::putText(image, std::to_string(i),
                        cv::Point(landmarks.part(i).x(), landmarks.part(i).y()),
                        cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 0, 255), 1);
        }

    } catch (const std::exception& e) {
        qWarning() << "关键点提取失败:" << e.what();

    }
}

dlib::full_object_detection DlibUtil::getLandmarks(const cv::Mat &image, const cv::Rect &faceBox)
{
    dlib::full_object_detection landmarks;

    // 裁剪到图像范围内，防止越界
    cv::Rect safeBox = faceBox & cv::Rect(0, 0, image.cols, image.rows);
    if (safeBox.area() <= 0) {
        qDebug() << "坐标越界!";
        return landmarks;
    };

    try {
        dlib::cv_image<dlib::bgr_pixel> dlibImg(image);
        // 将opencv的坐标转为dlib的坐标对象
        dlib::rectangle dlibRect = toDlibRect(safeBox);

        landmarks = m_sp(dlibImg, dlibRect);

    } catch (const std::exception& e) {
        qWarning() << "关键点提取失败:" << e.what();
    }
    return landmarks;
}

std::vector<float> DlibUtil::getFaceFeatures(const cv::Mat &bgrImage, const cv::Rect &faceBox)
{
    dlib::full_object_detection shape = this->getLandmarks(bgrImage,faceBox);

    // 1. 将 OpenCV Mat 包装成 dlib 图像
    dlib::cv_image<dlib::bgr_pixel> dlibImg(bgrImage);

    // 2. 关键步骤：人脸对齐，提取 150x150 的 face chip
    //    get_face_chip_details 的第二个参数是输出尺寸（必须是 150），第三个是 padding
    dlib::matrix<dlib::rgb_pixel> face_chip;
    dlib::extract_image_chip(dlibImg,
                             dlib::get_face_chip_details(shape, 150, 0.25),
                             face_chip);

    // 3. 前向推理，得到 128D 描述子
    dlib::matrix<float, 0, 1> descriptor = net(face_chip);

    // 4. 转为 std::vector<float> 方便存储或比对
    std::vector<float> feature(descriptor.size());
    for (long i = 0; i < descriptor.size(); ++i) {
        feature[i] = descriptor(i);
    }
    return feature;
}

float DlibUtil::calcDistance(std::vector<float> features1, std::vector<float> features2)
{
    float dist = 0.0;
    for (size_t i = 0; i < features1.size(); ++i) {
        double diff = features1[i] - features2[i];
        dist += diff * diff;
    }
    dist = std::sqrt(dist);
    //bool samePerson = (dist < 0.6);  // 0.6 是 dlib 官方推荐的阈值
    return dist;
}

dlib::rectangle DlibUtil::toDlibRect(const cv::Rect &r)
{
    // OpenCV 右/下边界不包含，dlib 包含，所以减 1
    return dlib::rectangle(
        static_cast<long>(r.x),
        static_cast<long>(r.y),
        static_cast<long>(r.x + r.width  - 1),
        static_cast<long>(r.y + r.height - 1));
}
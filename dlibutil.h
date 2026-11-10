// ===== dlibutil.h =====
// dlib ResNet 网络定义与 DlibUtil 声明（网络结构来自官方示例）。

#ifndef DLIBUTIL_H
#define DLIBUTIL_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <dlib/opencv.h>
#include <dlib/dnn.h>
#include <dlib/image_processing.h>
#include <QDebug>

// 在 dlibutil.h 中，包含头文件之后，类定义之前，加入这段网络结构定义
// 这段代码直接来自 dlib 官方示例 dnn_face_recognition_ex.cpp
// ===== dlib 人脸识别 ResNet 网络定义（带 dlib:: 前缀）=====
// 来源：dlib 官方示例 dnn_face_recognition_ex.cpp

template <template <int, template <typename> class, int, typename> class block,
         int N, template <typename> class BN, typename SUBNET>
using residual = dlib::add_prev1<block<N, BN, 1, dlib::tag1<SUBNET>>>;

template <template <int, template <typename> class, int, typename> class block,
         int N, template <typename> class BN, typename SUBNET>
using residual_down = dlib::add_prev2<dlib::avg_pool<
    2, 2, 2, 2, dlib::skip1<dlib::tag2<block<N, BN, 2, dlib::tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block = BN<dlib::con<N, 3, 3, 1, 1,
                           dlib::relu<BN<dlib::con<N, 3, 3, stride, stride, SUBNET>>>>>;

template <int N, typename SUBNET>
using ares = dlib::relu<residual<block, N, dlib::affine, SUBNET>>;

template <int N, typename SUBNET>
using ares_down = dlib::relu<residual_down<block, N, dlib::affine, SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256, SUBNET>;
template <typename SUBNET> using alevel1 = ares<256, ares<256, ares_down<256, SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128, ares<128, ares_down<128, SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64, ares<64, ares<64, ares_down<64, SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32, ares<32, ares<32, SUBNET>>>;

using anet_type = dlib::loss_metric<dlib::fc_no_bias<128, dlib::avg_pool_everything<
                                                              alevel0<
                                                                  alevel1<
                                                                      alevel2<
                                                                          alevel3<
                                                                              alevel4<
                                                                                  dlib::max_pool<3, 3, 2, 2, dlib::relu<dlib::affine<dlib::con<32, 7, 7, 2, 2,
                                                                                                                                               dlib::input_rgb_image_sized<150>
                                                                                                                                               >>>>>>>>>>>>;
// ===== 网络定义结束 =====

class DlibUtil : public QObject
{
    Q_OBJECT
public:
    explicit DlibUtil(const QString &landmarkModelPath,QObject *parent = nullptr);

    // 对单张 BGR 图 + YOLO 人脸框，提取 68 点并展示
    void showLandmarks(const cv::Mat& bgrImage,const cv::Rect& faceBox);

    // 对单张 BGR 图 + YOLO 人脸框，提取 68 点并返回
    dlib::full_object_detection getLandmarks(const cv::Mat& bgrImage,const cv::Rect& faceBox);

    std::vector<float> getFaceFeatures(const cv::Mat& bgrImage, const cv::Rect &faceBox);

    // 计算两张人脸的欧式距离
    float calcDistance(std::vector<float> features1,std::vector<float> features2);
signals:

private:
    dlib::shape_predictor m_sp;

    anet_type net;

    // 坐标转换：OpenCV Rect -> dlib rectangle
    dlib::rectangle toDlibRect(const cv::Rect& r);
};

#endif // DLIBUTIL_H
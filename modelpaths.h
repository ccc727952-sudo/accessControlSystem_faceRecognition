#ifndef MODELPATHS_H
#define MODELPATHS_H

#include <QString>

// =============================================================================
// 【ModelPaths】模型文件路径管理
//
//   为什么要有这个文件？
//   工程里模型路径是写死的（"D:/model/yolo-face.onnx"），
//   换一台电脑、换一个文件夹就得改源码、重新编译。
//   这里把它抽出来，改成三级查找：
//
//     ① 环境变量 FACE_MODEL_DIR   —— 运行前临时换模型，最灵活
//     ② 编译期宏 FACE_MODEL_DIR  —— CMakeLists.txt 里写死一个绝对路径（默认）
//     ③ exe 同级的 resources/model —— 兜底
//
//   ★ 三级都没配也没关系，程序会给出明确提示，不会"莫名其妙不工作"。
// =============================================================================
namespace ModelPaths {

// 模型所在的目录（末尾不带斜杠）
QString dir();

// ① YOLO 人脸检测模型（OpenCV DNN 用）
QString yoloFace();

// ② dlib 68 关键点形状预测器
QString landmarks();

// ③ dlib 人脸识别 ResNet 模型（提取 128 维特征向量）
QString resnet();

// ④ Day4 新增：MiniFASNetV2 静默活体模型（ONNX）
QString miniFAS();

}   // namespace ModelPaths

#endif // MODELPATHS_H
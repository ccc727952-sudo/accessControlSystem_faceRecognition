#include "modelpaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

// =============================================================================
// 【ModelPaths.cpp】模型文件路径管理

// 编译期兜底路径：由 CMakeLists.txt 里的 target_compile_definitions 传进来
#ifndef FACE_MODEL_DIR
#define FACE_MODEL_DIR ""
#endif

QString ModelPaths::dir()
{
    // ① 环境变量优先：运行前 set FACE_MODEL_DIR=... 就能换一套模型
    const QString env = qEnvironmentVariable("FACE_MODEL_DIR");
    if (!env.isEmpty()) {
        return QDir(env).absolutePath();
    }

    // ② 编译期指定的绝对路径（CMakeLists.txt 里配的那一个）
    const QString compiled = QStringLiteral(FACE_MODEL_DIR);
    if (!compiled.isEmpty()) {
        return QDir(compiled).absolutePath();
    }

    // ③ 兜底：exe 同级目录下的 resources/model
    return QDir(QCoreApplication::applicationDirPath())
        .absoluteFilePath(QStringLiteral("resources/model"));
}

QString ModelPaths::yoloFace()
{
    return QDir(dir()).absoluteFilePath(QStringLiteral("yolo-face.onnx"));
}

QString ModelPaths::landmarks()
{
    return QDir(dir()).absoluteFilePath(
        QStringLiteral("shape_predictor_68_face_landmarks.dat"));
}

QString ModelPaths::resnet()
{
    return QDir(dir()).absoluteFilePath(
        QStringLiteral("dlib_face_recognition_resnet_model_v1.dat"));
}

QString ModelPaths::miniFAS()
{
    // 新增：静默活体模型，与人脸检测/特征模型放同一目录
    return QDir(dir()).absoluteFilePath(
        QStringLiteral("MiniFASNetV2.onnx"));
}
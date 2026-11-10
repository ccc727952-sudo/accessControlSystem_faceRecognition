// ===== facedetector.cpp =====
// YOLO 人脸检测：ONNX 单输出 [1,5,8400] -> 框解码 -> NMS。

#include "facedetector.h"
#include <QDebug>
#include <algorithm>

FaceDetector::FaceDetector() {}

FaceDetector::~FaceDetector() {}

bool FaceDetector::load(const QString &onnxPath, int inputSize)
{
    m_inputSize = inputSize;
    try {
        m_net = cv::dnn::readNetFromONNX(onnxPath.toStdString());
    } catch (const cv::Exception &e) {
        qWarning() << "加载 ONNX 失败:" << e.what();
        return false;
    }

    if (m_net.empty()) {
        qWarning() << "ONNX 模型为空:" << onnxPath;
        return false;
    }

    // 如果用 GPU 可打开下面两行（需要 OpenCV 编译 CUDA 支持）
    // m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
    // m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

    m_loaded = true;
    qDebug() << "ONNX 模型加载成功:" << onnxPath << "输入尺寸:" << inputSize;
    return true;
}

QVector<FaceDetection> FaceDetector::detect(const cv::Mat &bgrImage,
                                            float confThreshold,
                                            float nmsThreshold)
{
    QVector<FaceDetection> results;
    if (!m_loaded || bgrImage.empty())
        return results;

    // 1. 预处理：保持和训练时一致 (640x640, RGB)
    cv::Mat blob;
    cv::dnn::blobFromImage(bgrImage, blob,
                           1.0 / 255.0,
                           cv::Size(m_inputSize, m_inputSize),
                           cv::Scalar(0, 0, 0),
                           true,   // swapRB: BGR -> RGB
                           false);

    m_net.setInput(blob);

    // 2. 前向推理，得到单一输出
    std::vector<cv::Mat> outputs;
    m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());

    // for (size_t i = 0; i < outputs.size(); ++i) {
    //     qDebug() << "output" << i << "dims:" << outputs[i].dims;
    //     for (int d = 0; d < outputs[i].dims; ++d) {
    //         qDebug() << "  size[" << d << "] =" << outputs[i].size[d];
    //     }
    // }

    if (outputs.empty()) return results;

    cv::Mat output = outputs[0];   // 形状 [1, 5, 8400]

    // 转置成 [8400, 5]
    int C = output.size[1];        // 5
    int N = output.size[2];        // 8400

    cv::Mat tmp = output.reshape(1, C);   // [5, 8400]
    cv::Mat transposed;
    cv::transpose(tmp, transposed);       // [8400, 5]

    // qDebug() << "解析: rows =" << transposed.rows << "cols =" << transposed.cols;

    QVector<cv::Rect> boxes;
    QVector<float> confidences;

    // 坐标是 640 输入空间下的像素值，按比例缩放到原图
    const float scaleX = static_cast<float>(bgrImage.cols) / m_inputSize;
    const float scaleY = static_cast<float>(bgrImage.rows) / m_inputSize;

    for (int i = 0; i < transposed.rows; ++i) {
        const float *row = transposed.ptr<float>(i);

        float cx = row[0];
        float cy = row[1];
        float w  = row[2];
        float h  = row[3];
        float score = row[4];

        if (score < confThreshold) continue;

        int left   = static_cast<int>((cx - w / 2.0f) * scaleX);
        int top    = static_cast<int>((cy - h / 2.0f) * scaleY);
        int width  = static_cast<int>(w * scaleX);
        int height = static_cast<int>(h * scaleY);

        left   = std::max(0, left);
        top    = std::max(0, top);
        width  = std::min(width, bgrImage.cols - left);
        height = std::min(height, bgrImage.rows - top);
        if (width <= 0 || height <= 0) continue;

        boxes.append(cv::Rect(left, top, width, height));
        confidences.append(score);
    }

    nms(boxes, confidences, nmsThreshold);

    // 6. 组装结果
    for (int i = 0; i < boxes.size(); ++i) {
        FaceDetection d;
        d.box = boxes[i];
        d.confidence = confidences[i];
        results.append(d);
    }

    return results;
}

void FaceDetector::decodeOutput(const cv::Mat &output, int stride,
                                const cv::Size &inputSize,
                                const cv::Size &originalSize,
                                float confThreshold,
                                QVector<cv::Rect> &boxes,
                                QVector<float> &confidences)
{
    // YOLOv5 输出 shape: [1, N, C]，N 是该尺度的 anchor 数，C = 5 + num_classes
    // 对于单类别人脸，C = 6
    const int numClasses = output.size[2] - 5;
    const int numAnchors = output.size[1];

    // 该尺度特征图大小
    const int gridH = inputSize.height / stride;
    const int gridW = inputSize.width / stride;

    const float *data = output.ptr<float>();

    // 缩放比例：输入空间 -> 原图空间
    const float scaleX = static_cast<float>(originalSize.width) / inputSize.width;
    const float scaleY = static_cast<float>(originalSize.height) / inputSize.height;

    for (int a = 0; a < numAnchors; ++a) {
        const float *row = data + a * output.size[2];

        // row[0..3] = cx, cy, w, h（相对于网格，已 sigmoid 过）
        // row[4] = obj_conf
        // row[5..] = 类别置信度
        float objConf = row[4];
        if (objConf < confThreshold)
            continue;

        // 找最大类别
        float maxCls = 0.f;
        int maxIdx = -1;
        for (int c = 0; c < numClasses; ++c) {
            float clsConf = row[5 + c];
            if (clsConf > maxCls) {
                maxCls = clsConf;
                maxIdx = c;
            }
        }
        float score = objConf * maxCls;
        if (score < confThreshold)
            continue;

        // 解码中心点和宽高（YOLOv5 输出已经是相对输入尺寸的归一化值乘以输入尺寸）
        float cx = row[0];
        float cy = row[1];
        float w  = row[2];
        float h  = row[3];

        // 有些导出版本输出的是相对网格的偏移，需要额外处理：
        // cx = (row[0] * 2 - 0.5 + gridX) * stride 等。
        // 这里假设导出时已包含后处理（常见于 export.py 的 --include onnx 带 Detect 层）。
        // 如果框明显不对，需要改成上面的网格解码公式。

        float x = (cx - w / 2.f) * scaleX;
        float y = (cy - h / 2.f) * scaleY;
        float bw = w * scaleX;
        float bh = h * scaleY;

        // 边界裁剪
        int left = std::max(0, static_cast<int>(x));
        int top = std::max(0, static_cast<int>(y));
        int right = std::min(originalSize.width, static_cast<int>(x + bw));
        int bottom = std::min(originalSize.height, static_cast<int>(y + bh));
        if (right <= left || bottom <= top)
            continue;

        boxes.append(cv::Rect(left, top, right - left, bottom - top));
        confidences.append(score);
    }
}

void FaceDetector::nms(QVector<cv::Rect> &boxes,
                       QVector<float> &confidences,
                       float nmsThreshold)
{
    std::vector<cv::Rect> b(boxes.begin(), boxes.end());
    std::vector<float> c(confidences.begin(), confidences.end());

    std::vector<int> indices;
    cv::dnn::NMSBoxes(b, c, 0.0f, nmsThreshold, indices);

    QVector<cv::Rect> outBoxes;
    QVector<float> outConf;
    outBoxes.reserve(indices.size());
    outConf.reserve(indices.size());
    for (int idx : indices) {
        outBoxes.append(b[idx]);
        outConf.append(c[idx]);
    }
    boxes = outBoxes;
    confidences = outConf;
}
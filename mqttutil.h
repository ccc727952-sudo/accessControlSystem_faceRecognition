#ifndef MQTTUTIL_H
#define MQTTUTIL_H

#include <QObject>
#include <QByteArray>
#include <QMqttClient>

// MqttUtil：MQTT 通信工具类（单例）
// 负责连接本地 EMQX broker、订阅人脸特征主题，并把收到的消息交给逻辑层处理。
class MqttUtil : public QObject
{
    Q_OBJECT
private:
    explicit MqttUtil(QObject *parent = nullptr);

public:
    // 禁用拷贝，保证全局只有一个 MQTT 连接
    MqttUtil(const MqttUtil& obj) = delete;
    void operator=(const MqttUtil& obj) = delete;

    // 全局唯一实例：main() 里调用一次即可启动连接
    static MqttUtil& getInstance();

    // 订阅住户人脸特征消息（函数名沿用原型拼写，保持不变）
    void subcribeFaceData();

    // 云端人脸识别：把一帧人脸的 128 维特征（Base64）发布到请求主题，
    // 返回本次请求 id；未连接或发布失败返回空字符串，调用方据此直接判负。
    // 注意：必须在 MqttUtil 所在线程（GUI 线程）调用。
    Q_INVOKABLE QString requestCloudRecognition(const QByteArray &featureBase64);

signals:
    // 云端识别结果：requestId 用于与请求配对，passed 为是否放行，name 为空表示未识别
    void cloudRecognitionResult(const QString &requestId, bool passed, const QString &name);

private:
    // 订阅云端识别响应主题
    void subscribeCloudResult();

    QMqttClient *client = nullptr;
};

#endif // MQTTUTIL_H
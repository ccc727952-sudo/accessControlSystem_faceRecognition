#ifndef MQTTUTIL_H
#define MQTTUTIL_H

#include <QObject>
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
signals:

private:
    QMqttClient *client = nullptr;
};

#endif // MQTTUTIL_H
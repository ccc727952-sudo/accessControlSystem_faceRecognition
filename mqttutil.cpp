#include "mqttutil.h"
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include "facedatalogic.h"

// ===== MQTT 连接参数（集中放在文件顶部，按自己的 EMQX 配置修改）=====
static const QString  MQTT_HOST        = "127.0.0.1";  // broker 地址
static const quint16  MQTT_PORT        = 1883; // MQTT TCP 端口
static const QString  MQTT_USER        = "rh"; // 用户名（匿名 broker 可留空）
static const QString  MQTT_PASSWORD    = "123456"; // 密码
static const QString  MQTT_CLIENT_ID   = "faceRecog-mqtt-client"; // 客户端 ID，同一 broker 内需唯一
// # 是 MQTT 多层通配符，可匹配 faceFeatures/1、faceFeatures/2 等所有子主题
static const QString  MQTT_TOPIC_FILTER = "faceFeatures/#";

// ===== 云端人脸识别主题 =====
// 请求：识别端 -> 后端，JSON { features(Base64 512 字节), timestamp }
// 响应：后端 -> 识别端，JSON { passed }
static const QString  MQTT_TOPIC_CLOUD_REQ  = "faceRecog/cloud/request";
static const QString  MQTT_TOPIC_CLOUD_RESP = "faceRecog/cloud/response";

MqttUtil::MqttUtil(QObject *parent)
    : QObject{parent}
{
    this->client = new QMqttClient;
    // 设置连接属性。注意：数据库部分不能用 Qt SQL（其内置 SQLite 禁用了 load_extension），
    // 所以本项目用原生 sqlite3 C API，详见 facedatadao.cpp。
    this->client->setHostname(MQTT_HOST);
    this->client->setPort(MQTT_PORT);
    this->client->setClientId(MQTT_CLIENT_ID);
    this->client->setUsername(MQTT_USER);
    this->client->setPassword(MQTT_PASSWORD);

    // 连接状态变化：只有真正 Connected 之后才能订阅，否则订阅会失败
    connect(this->client, &QMqttClient::stateChanged, [this](QMqttClient::ClientState state){
        if (state == QMqttClient::Connected)
        {
            qDebug() << "成功连接mqtt服务器!";
            this->subcribeFaceData();
            this->subscribeCloudResult();
        }
    });

    // 发起连接（异步，结果通过上面的 stateChanged 信号返回）
    this->client->connectToHost();
}

MqttUtil &MqttUtil::getInstance()
{
    // 局部静态变量：C++11 起线程安全，第一次调用时才构造
    static MqttUtil instance;
    return instance;
}

// 订阅消息：收到 JSON 后打印中文日志，再交给 FaceDataLogic 解析入库
void MqttUtil::subcribeFaceData()
{
    QMqttTopicFilter topicFilter(MQTT_TOPIC_FILTER);
    QMqttSubscription * sub = this->client->subscribe(topicFilter);
    // 每条消息到达时回调：payload 是 JSON 字节流
    connect(sub, &QMqttSubscription::messageReceived, [](QMqttMessage msg){
        QByteArray data = msg.payload();
        qDebug() << "接收到的消息内容:" << data;
        //这一行会替换成"解析 JSON → 写入向量库"的调用
        FaceDataLogic logic;
        logic.saveDataToDb(data);
    });
}

// 订阅云端识别结果：后端处理后把 {"passed":true} 发回来
void MqttUtil::subscribeCloudResult()
{
    QMqttSubscription *sub = this->client->subscribe(QMqttTopicFilter(MQTT_TOPIC_CLOUD_RESP));
    if (!sub) {
        qWarning() << "订阅云端识别结果主题失败:" << MQTT_TOPIC_CLOUD_RESP;
        return;
    }

    connect(sub, &QMqttSubscription::messageReceived, this, [this](QMqttMessage msg){
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(msg.payload(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "云端识别结果 JSON 解析失败:" << err.errorString();
            return;
        }

        const bool passed = doc.object().value("passed").toBool();

        qDebug() << "收到云端识别结果: passed =" << passed;
        emit this->cloudRecognitionResult(passed);
    });
}

// 发布云端识别请求：features 已是 Base64 字符串，直接塞进 JSON
bool MqttUtil::requestCloudRecognition(const QByteArray &featureBase64)
{
    if (!this->client || this->client->state() != QMqttClient::Connected) {
        qWarning() << "MQTT 未连接，无法发起云端识别";
        return false;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("features"), QString::fromLatin1(featureBase64));
    obj.insert(QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch());

    const qint32 msgId = this->client->publish(
        QMqttTopicName(MQTT_TOPIC_CLOUD_REQ),
        QJsonDocument(obj).toJson(QJsonDocument::Compact));

    if (msgId == -1) {
        qWarning() << "云端识别请求发布失败:" << MQTT_TOPIC_CLOUD_REQ;
        return false;
    }

    qDebug() << "已发送云端识别请求";
    return true;
}
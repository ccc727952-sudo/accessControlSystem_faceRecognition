/*
 * @brief MQTT 通信工具类的实现
 * 文件顶部集中定义 broker 地址、端口、账号密码与订阅主题，便于统一修改。
 * 构造时创建 QMqttClient 并异步发起连接，连接成功后在 stateChanged 回调里订阅主题；
 * 每条消息到达时把 payload 交给 FaceDataLogic::saveDataToDb() 解析入库。
*/
#include "mqttutil.h"
#include <QDebug>
#include "facedatalogic.h"

// ===== MQTT 连接参数（集中放在文件顶部，按自己的 EMQX 配置修改）=====
static const QString  MQTT_HOST        = "127.0.0.1";  // broker 地址
static const quint16  MQTT_PORT        = 1883; // MQTT TCP 端口
static const QString  MQTT_USER        = "rh"; // 用户名（匿名 broker 可留空）
static const QString  MQTT_PASSWORD    = "123456"; // 密码
static const QString  MQTT_CLIENT_ID   = "faceRecog-mqtt-client"; // 客户端 ID，同一 broker 内需唯一
// # 是 MQTT 多层通配符，可匹配 faceFeatures/1、faceFeatures/2 等所有子主题
static const QString  MQTT_TOPIC_FILTER = "faceFeatures/#";

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
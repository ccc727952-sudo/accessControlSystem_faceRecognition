/*
 * @brief 主程序入口
 * 创建 QGuiApplication 与 QQmlApplicationEngine，加载 accessControlSystem_faceRecognition
 * 模块下的 Main.qml 主窗口；启动时调用 MqttUtil::getInstance() 建立 MQTT 连接，
 * 随后进入 Qt 事件循环。
*/
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "mqttutil.h"
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("accessControlSystem_faceRecognition", "Main");
    //程序运行就要启动MQTT连接
    MqttUtil::getInstance();
    return QGuiApplication::exec();
}

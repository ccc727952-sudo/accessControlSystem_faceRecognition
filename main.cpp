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

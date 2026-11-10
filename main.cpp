#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "mqttutil.h"
#include "facerecognitionmanager.h"
#include <QQuickStyle>
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    qmlRegisterType<FaceRecognitionManager>("cn.rh",1,0,"FaceRecognitionManager");
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

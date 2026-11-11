// ===== networkconnection.cpp =====
// 用 QSettings 解析标准 INI，不用手写字符串切割。

#include "networkconnection.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

// 编译期兜底路径：由 CMakeLists.txt 的 target_compile_definitions 传入
#ifndef NETWORK_INI_PATH
#define NETWORK_INI_PATH ""
#endif

NetworkConnection::NetworkConnection(QObject *parent)
    : QObject{parent}
{
    // 路径优先级：环境变量 > 编译期宏 > exe 同级目录
    const QString env = qEnvironmentVariable("NETWORK_INI");
    if (!env.isEmpty()) {
        m_configPath = env;
    } else {
        const QString compiled = QStringLiteral(NETWORK_INI_PATH);
        if (!compiled.isEmpty())
            m_configPath = compiled;
        else
            m_configPath = QDir(QCoreApplication::applicationDirPath())
                               .absoluteFilePath(QStringLiteral("Network.ini"));
    }
}

QString NetworkConnection::configFilePath() const
{
    return m_configPath;
}

bool NetworkConnection::loadConfig()
{
    if (!QFileInfo::exists(m_configPath)) {
        qWarning() << "[NetworkConnection] 配置文件不存在:" << m_configPath;
        return false;
    }

    QSettings settings(m_configPath, QSettings::IniFormat);
    if (settings.status() != QSettings::NoError) {
        qWarning() << "[NetworkConnection] 配置文件解析失败:" << m_configPath;
        return false;
    }

    // [WiFi] 段：路由器的 SSID 和密码
    settings.beginGroup("WiFi");
    m_wifiSsid     = settings.value("ssid",     QString()).toString();
    m_wifiPassword = settings.value("password", QString()).toString();
    settings.endGroup();

    // [Network] 段：设备自身的网络名 / 密码
    settings.beginGroup("Network");
    m_networkName     = settings.value("name",     QString()).toString();
    m_networkPassword = settings.value("password", QString()).toString();
    settings.endGroup();

    return true;
}

void NetworkConnection::dump() const
{
    qDebug() << "===== 网络配置 =====";
    qDebug() << "配置文件:" << m_configPath;
    qDebug() << "[WiFi]    ssid =" << m_wifiSsid << " password =" << m_wifiPassword;
    qDebug() << "[Network] name =" << m_networkName << " password =" << m_networkPassword;
}

QString NetworkConnection::wifiSsid() const { return m_wifiSsid; }
QString NetworkConnection::wifiPassword() const { return m_wifiPassword; }
QString NetworkConnection::networkName() const { return m_networkName; }
QString NetworkConnection::networkPassword() const { return m_networkPassword; }

// ===== networkconnection.h =====
// 网络配置：读取 Network.ini 里的 WiFi / 网络参数，后续用它发起网络连接。

#ifndef NETWORKCONNECTION_H
#define NETWORKCONNECTION_H

#include <QObject>
#include <QString>

class NetworkConnection : public QObject
{
    Q_OBJECT
public:
    explicit NetworkConnection(QObject *parent = nullptr);

    // 读取 Network.ini；文件缺失或格式错误只返回 false，不抛异常、不阻断启动
    bool loadConfig();

    // 把已读取的配置打印到日志，方便确认读对了
    void dump() const;

    // 配置文件绝对路径（环境变量 NETWORK_INI > 编译期宏 > exe 同级目录）
    QString configFilePath() const;

    QString wifiSsid() const;
    QString wifiPassword() const;
    QString networkName() const;
    QString networkPassword() const;

private:
    QString m_configPath;
    QString m_wifiSsid;
    QString m_wifiPassword;
    QString m_networkName;
    QString m_networkPassword;
};

#endif // NETWORKCONNECTION_H

#include "weatherservice.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

// 编译期兜底：由 CMakeLists.txt 的 target_compile_definitions 传入
#ifndef WEATHER_IMG_DIR
#define WEATHER_IMG_DIR ""
#endif

namespace {

// 易客天气的免费测试账号（正式使用建议改成读环境变量，避免密钥进仓库）
const char *kAppId = "84979327";
const char *kAppSecret = "JlRJLFN88";

// 刷新间隔：30 分钟。免费接口每天 300 次限额，完全够用
const int kRefreshIntervalMs = 30 * 60 * 1000;

// 图片目录查找，优先级：环境变量 > 编译期宏 > exe 同级 resources/weather
QString weatherImageDir()
{
    const QString env = qEnvironmentVariable("WEATHER_IMG_DIR");
    if (!env.isEmpty())
        return QDir(env).absolutePath();

    const QString compiled = QStringLiteral(WEATHER_IMG_DIR);
    if (!compiled.isEmpty())
        return QDir(compiled).absolutePath();

    return QDir(QCoreApplication::applicationDirPath())
        .absoluteFilePath(QStringLiteral("resources/weather"));
}

// JSON 值转字符串：接口大部分字段是字符串，但个别版本会返回数字，这里两种都兼容
QString jsonText(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 10);
    return QString();
}

}

WeatherService::WeatherService(QObject *parent)
    : QObject{parent}
{
    // 网络管理器：QNetworkAccessManager 必须在有事件循环的线程（这里是主线程）创建
    m_manager = new QNetworkAccessManager(this);

    // 定时器：每 30 分钟拉一次最新天气
    m_timer = new QTimer(this);
    m_timer->setInterval(kRefreshIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &WeatherService::requestWeather);
    m_timer->start();

    // 启动后立刻请求一次，避免界面长时间显示"天气加载中..."
    requestWeather();
}

void WeatherService::refresh()
{
    requestWeather();
}

// 拼装展示文案，例如"阴 13°C"；还没有数据时给个占位提示
QString WeatherService::summary() const
{
    if (m_wea.isEmpty() && m_tem.isEmpty())
        return QStringLiteral("天气加载中...");
    return QStringLiteral("%1 %2°C").arg(m_wea, m_tem);
}

// 按 wea_img 拼出图标路径：resources/weather/<wea_img>.png
QString WeatherService::iconSource() const
{
    // 资源目录里实际存在的图标名，接口返回其他值时统一兜底成 yin.png
    static const QStringList kIcons{
        QStringLiteral("qing"), QStringLiteral("yun"), QStringLiteral("yin"),
        QStringLiteral("yu"), QStringLiteral("lei"), QStringLiteral("xue"),
        QStringLiteral("wu"), QStringLiteral("shachen"), QStringLiteral("mai"),
        QStringLiteral("bingbao"), QStringLiteral("feng"), QStringLiteral("big_rain")
    };

    QString key = m_weaImg.trimmed().toLower();
    if (!kIcons.contains(key))
        key = QStringLiteral("yin");

    // QML 的 Image.source 需要 URL，所以用 fromLocalFile 转成 file:///...
    return QUrl::fromLocalFile(
        QDir(weatherImageDir()).absoluteFilePath(key + QStringLiteral(".png"))).toString();
}

// 背景图固定为 weather_background.jpg，用 CONSTANT 属性只需取一次
QString WeatherService::backgroundSource() const
{
    return QUrl::fromLocalFile(
        QDir(weatherImageDir()).absoluteFilePath(QStringLiteral("weather_background.jpg"))).toString();
}

void WeatherService::requestWeather()
{
    // 防重入：上一次请求还没结束就不重复发
    if (m_loading)
        return;

    m_loading = true;
    m_error.clear();
    emit weatherChanged(); // 通知 QML：loading 状态变了

    // 组装请求地址：http://v1.yiketianqi.com/free/day?appid=...&appsecret=...&unescape=1
    QUrl url(QStringLiteral("http://v1.yiketianqi.com/free/day"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("appid"), QString::fromLatin1(kAppId));
    query.addQueryItem(QStringLiteral("appsecret"), QString::fromLatin1(kAppSecret));
    query.addQueryItem(QStringLiteral("unescape"), QStringLiteral("1")); // 1 = 返回中文不转义
    url.setQuery(query);

    QNetworkRequest request(url);
    // 部分服务端会校验 UA，带上浏览器的 UA 更稳
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
    // 允许跟随重定向（http -> https 或 CDN 跳转）
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    // 异步 GET：finished 信号回来时再解析，不阻塞 UI
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleReply(reply);
        reply->deleteLater(); // 必须手动释放
    });
}

void WeatherService::handleReply(QNetworkReply *reply)
{
    m_loading = false;

    // 1) 网络层错误（断网、DNS 失败、超时等）
    if (reply->error() != QNetworkReply::NoError) {
        setError(reply->errorString());
        emit weatherChanged();
        return;
    }

    // 2) JSON 解析错误
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        setError(QStringLiteral("天气数据解析失败"));
        emit weatherChanged();
        return;
    }

    // 3) 业务错误：接口失败时会返回 {"errcode": xxx, "errmsg": "..."}
    const QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("errcode"))
        && obj.value(QStringLiteral("errcode")).toInt() != 0) {
        setError(obj.value(QStringLiteral("errmsg")).toString());
        emit weatherChanged();
        return;
    }

    // 4) 正常解析：字段名与接口返回完全一致
    m_city = jsonText(obj.value(QStringLiteral("city")));
    m_wea = jsonText(obj.value(QStringLiteral("wea")));
    m_weaImg = jsonText(obj.value(QStringLiteral("wea_img")));   // 图标关键字
    m_tem = jsonText(obj.value(QStringLiteral("tem")));
    m_temDay = jsonText(obj.value(QStringLiteral("tem_day")));
    m_temNight = jsonText(obj.value(QStringLiteral("tem_night")));
    m_win = jsonText(obj.value(QStringLiteral("win")));
    m_winSpeed = jsonText(obj.value(QStringLiteral("win_speed")));
    m_humidity = jsonText(obj.value(QStringLiteral("humidity")));
    m_air = jsonText(obj.value(QStringLiteral("air")));
    m_updateTime = jsonText(obj.value(QStringLiteral("update_time")));
    m_error.clear(); // 成功后清掉旧错误

    emit weatherChanged(); // 一次性通知 QML 刷新所有绑定
}

void WeatherService::setError(const QString &msg)
{
    m_error = msg;
    qWarning() << "天气接口请求失败:" << msg;
}

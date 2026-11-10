/*
 * @brief 实时天气服务
 * 负责请求易客天气免费接口（v1.yiketianqi.com/free/day），把返回的 JSON
 * 解析成一个个 QString 属性，通过 Q_PROPERTY 暴露给 QML 直接绑定。
 * 数据每 30 分钟自动刷新一次；天气图标按接口的 wea_img 字段（qing/yun/yin...）
 * 在 resources/weather 目录下匹配同名 png。
 * 图片目录优先级：环境变量 WEATHER_IMG_DIR > 编译期宏 WEATHER_IMG_DIR > exe 同级目录，
 * 与 ModelPaths 的路径约定保持一致。
*/
#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QObject>
#include <QString>

// 前置声明：头文件里只用指针成员，不必包含完整定义，减少编译依赖
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class WeatherService : public QObject
{
    Q_OBJECT

    // ===== 以下属性全部供 QML 绑定，例如 Text { text: weatherService.city } =====
    Q_PROPERTY(QString city READ city NOTIFY weatherChanged)             // 城市名，如"成都"
    Q_PROPERTY(QString wea READ wea NOTIFY weatherChanged)               // 天气文字，如"阴"
    Q_PROPERTY(QString tem READ tem NOTIFY weatherChanged)               // 当前温度，如"13"
    Q_PROPERTY(QString temDay READ temDay NOTIFY weatherChanged)         // 白天最高温
    Q_PROPERTY(QString temNight READ temNight NOTIFY weatherChanged)     // 夜间最低温
    Q_PROPERTY(QString win READ win NOTIFY weatherChanged)               // 风向，如"北风"
    Q_PROPERTY(QString winSpeed READ winSpeed NOTIFY weatherChanged)     // 风力，如"02级"
    Q_PROPERTY(QString humidity READ humidity NOTIFY weatherChanged)     // 湿度，如"52%"
    Q_PROPERTY(QString air READ air NOTIFY weatherChanged)               // 空气质量指数
    Q_PROPERTY(QString updateTime READ updateTime NOTIFY weatherChanged) // 数据更新时间，如"18:38"
    Q_PROPERTY(QString summary READ summary NOTIFY weatherChanged)       // 拼好的展示文案："阴 13°C"
    Q_PROPERTY(QString iconSource READ iconSource NOTIFY weatherChanged) // 当前天气图标 file:// URL
    Q_PROPERTY(QString backgroundSource READ backgroundSource CONSTANT)  // 背景图 URL，运行期不变
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY weatherChanged) // 最近一次请求的错误信息
    Q_PROPERTY(bool loading READ loading NOTIFY weatherChanged)          // 是否正在请求中

public:
    explicit WeatherService(QObject *parent = nullptr);

    // ---- 各字段的只读访问器，实现直接内联，减少 .cpp 里的样板代码 ----
    QString city() const { return m_city; }
    QString wea() const { return m_wea; }
    QString tem() const { return m_tem; }
    QString temDay() const { return m_temDay; }
    QString temNight() const { return m_temNight; }
    QString win() const { return m_win; }
    QString winSpeed() const { return m_winSpeed; }
    QString humidity() const { return m_humidity; }
    QString air() const { return m_air; }
    QString updateTime() const { return m_updateTime; }
    QString errorMessage() const { return m_error; }
    bool loading() const { return m_loading; }

    // summary / iconSource / backgroundSource 需要计算，实现在 .cpp 中
    QString summary() const;
    QString iconSource() const;
    QString backgroundSource() const;

    // 供 QML 手动刷新（可绑到按钮上），不传参
    Q_INVOKABLE void refresh();

signals:
    // 统一用一个信号通知所有属性变化：QML 侧所有绑定会一起刷新
    void weatherChanged();

private:
    void requestWeather();                  // 发起 HTTP GET
    void handleReply(QNetworkReply *reply); // 解析响应并写回成员变量
    void setError(const QString &msg);      // 记录错误并打印日志

    QNetworkAccessManager *m_manager = nullptr; // 网络请求管理器（主线程使用）
    QTimer *m_timer = nullptr;                  // 定时刷新器，默认 30 分钟

    // 接口返回的原始字段，统一按 QString 保存，避免 QML 侧再转换
    QString m_city;
    QString m_wea;
    QString m_weaImg;   // 天气图标关键字，用于拼 png 文件名
    QString m_tem;
    QString m_temDay;
    QString m_temNight;
    QString m_win;
    QString m_winSpeed;
    QString m_humidity;
    QString m_air;
    QString m_updateTime;
    QString m_error;    // 最近一次失败原因，成功时清空
    bool m_loading = false; // 防重入：上一次请求没回来就不再发新请求
};

#endif // WEATHERSERVICE_H

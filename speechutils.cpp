// ===== speechutils.cpp =====
// 语音播报单例：QTextToSpeech + QEventLoop 非阻塞等待。

// speechutils.cpp
#include "speechutils.h"

#include "speechutils.h"
#include <QEventLoop>
#include <QTimer>

SpeechUtils *SpeechUtils::getInstance()
{
    static SpeechUtils instance;
    return &instance;

}

void SpeechUtils::speek(QString text)
{

    this->m_speech->say(text);


}

void SpeechUtils::speek(QString text, int times)
{

    this->m_speech->say(text);
    //等时间

    // 在qt中，如果想等待几秒后执行，但是这期间不能卡死界面，那么我们就可以使用事件循环
    QEventLoop loop;

    // 通过定时器QTimer，让事件循环在5s结束
    //QTimer::singleShot()
    QTimer timer;
    // 绑定定时任务的timeout信号，和事件循环的退出的槽函数做绑定
    connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    // 开启定时任务，5s结束
    timer.start(times);

    // 开启事件循环
    loop.exec();

}

SpeechUtils::SpeechUtils(QObject *parent)
    : QObject{parent}
{

    this->m_speech = new QTextToSpeech();
    //设置默认音量
    this->m_speech->setVolume(1.0);
}
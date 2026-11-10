// ===== speechutils.h =====
// 语音播报单例声明。

// speechutils.h
#ifndef SPEECHUTILS_H
#define SPEECHUTILS_H

#include <QObject>
#include <QTextToSpeech>

class SpeechUtils : public QObject
{
    Q_OBJECT
public:
    static SpeechUtils* getInstance();
    void speek(QString text);
    void speek(QString text,int times);
private:
    explicit SpeechUtils(QObject *parent = nullptr);
    QTextToSpeech *m_speech;
signals:
};

#endif // SPEECHUTILS_H
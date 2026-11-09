#ifndef FACEDATALOGIC_H
#define FACEDATALOGIC_H

#include <QObject>

//业务逻辑层
//把MQTT收到 JSON字节流解析成FaceData实体，在转化维DAO入库
class FaceDataLogic : public QObject
{
    Q_OBJECT
public:
    explicit FaceDataLogic(QObject *parent = nullptr);
    //保存MQTT下发的住户信息和住户人脸信息到数据库  向量数据库
    void saveDataToDb(const QByteArray& payload);
signals:
};

#endif // FACEDATALOGIC_H

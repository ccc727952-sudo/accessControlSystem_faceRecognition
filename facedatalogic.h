/*
 * @brief 人脸记录业务逻辑层
 * 位于 MQTT 通信层与数据访问层之间，负责把 MQTT 收到的 JSON 字节流解析成
 * FaceData 实体，再交给 FaceDataDao 写入向量数据库。
 * JSON 解析失败或格式不符时只打印警告并直接返回，不向上抛出异常。
*/
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

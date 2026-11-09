/*
 * @brief 人脸记录业务逻辑层的实现
 * 实现 saveDataToDb()：解析 MQTT 下发的 JSON（sn/name/phone/address/features），
 * 把 base64 编码的 features 解码为 128 维 float 的原始二进制，
 * 封装成 FaceData 对象后调用 FaceDataDao 写入数据库。
*/
#include "facedatalogic.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include "facedata.h"
#include "facedatadao.h"

FaceDataLogic::FaceDataLogic(QObject *parent)
    : QObject{parent}
{}


//保存MQTT下发的住户信息和住户人脸信息到数据库  向量数据库
void FaceDataLogic::saveDataToDb(const QByteArray& payload){
    //1.解析JSON：协议字段为sn/name/phone/address/features
    QJsonParseError err;
    QJsonDocument doc=QJsonDocument::fromJson(payload,&err);
    if(err.error!=QJsonParseError::NoError || !doc.isObject()){
        qWarning()<<"JSON解析失败"<<err.errorString();
        return;
    }
    QJsonObject obj=doc.object();
    QString sn=obj.value("sn").toString();
    QString name=obj.value("name").toString();
    QString phone=obj.value("phone").toString();
    QString address=obj.value("address").toString();

    //2.features是128个float的，QByteArray需要保存原始的二进制，要转换
    QByteArray features=QByteArray::fromBase64(obj["features"].toString().toLatin1());

    //3.封装为FaceData对象
    FaceData data(sn,name,phone,address,features);
    //4.保存  DAO
    FaceDataDao dao;
    dao.add(data);
}
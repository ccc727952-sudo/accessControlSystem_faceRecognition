#include "facedata.h"

// 实体类只有构造与 getter/setter，无业务逻辑；数据由 FaceDataDao 统一入库。
FaceData::FaceData(QObject *parent)
    : QObject{parent}
{}

int FaceData::getId() const
{
    return id;
}

void FaceData::setId(int newId)
{
    id = newId;
}

QString FaceData::getName() const
{
    return name;
}

void FaceData::setName(const QString &newName)
{
    name = newName;
}

QString FaceData::getPhone() const
{
    return phone;
}

void FaceData::setPhone(const QString &newPhone)
{
    phone = newPhone;
}

QString FaceData::getAddress() const
{
    return address;
}

void FaceData::setAddress(const QString &newAddress)
{
    address = newAddress;
}

QByteArray FaceData::getFeatures() const
{
    return features;
}

void FaceData::setFeatures(const QByteArray &newFeatures)
{
    features = newFeatures;
}

QString FaceData::getSn() const
{
    return sn;
}

void FaceData::setSn(const QString &newSn)
{
    sn = newSn;
}

// 带 id 的构造：从数据库读出的记录用
FaceData::FaceData(int id, const QString &sn, const QString &name, const QString &phone, const QString &address, const QByteArray &features) : id(id),
    sn(sn),
    name(name),
    phone(phone),
    address(address),
    features(features)
{}

// 不带 id 的构造：新增记录时用，id 由 vec0 表自动生成
FaceData::FaceData(const QString &sn, const QString &name, const QString &phone, const QString &address, const QByteArray &features) : sn(sn),
    name(name),
    phone(phone),
    address(address),
    features(features)
{}
#ifndef FACEDATA_H
#define FACEDATA_H

#include <QObject>

// FaceData：人脸记录实体类，字段与 vec0 虚拟表 t_face_data 的列一一对应。
// features 保存 128 维 float 的原始二进制（128*4 = 512 字节），
// 由 MQTT 消息里 features 字段的 base64 字符串解码而来。
class FaceData : public QObject
{
    Q_OBJECT
public:
    explicit FaceData(QObject *parent = nullptr);
    FaceData(const QString &sn, const QString &name, const QString &phone, const QString &address, const QByteArray &features);
    FaceData(int id, const QString &sn, const QString &name, const QString &phone, const QString &address, const QByteArray &features);

    int getId() const;
    void setId(int newId);
    QString getName() const;
    void setName(const QString &newName);
    QString getPhone() const;
    void setPhone(const QString &newPhone);
    QString getAddress() const;
    void setAddress(const QString &newAddress);
    QByteArray getFeatures() const;
    void setFeatures(const QByteArray &newFeatures);

    QString getSn() const;
    void setSn(const QString &newSn);

signals:

private:
    int id;
    QString sn;
    QString name;
    QString phone;
    QString address;
    QByteArray features;
};

#endif // FACEDATA_H
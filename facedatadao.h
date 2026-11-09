#ifndef FACEDATADAO_H
#define FACEDATADAO_H

#include <QObject>
#include "facedata.h"
#include "sqlite3.h"

// FaceDataDao：数据访问层（DAO）
// 关键点：Qt 内置 SQLite 被裁剪掉了 load_extension，无法加载 sqlite-vec；
// 因此项目里直接编译原生 sqlite3.c，用 C API 打开独立连接并加载 sqlite-vec.dll。
class FaceDataDao : public QObject
{
    Q_OBJECT
public:
    explicit FaceDataDao(QObject *parent = nullptr);
    ~FaceDataDao();                       // 析构里 sqlite3_close，释放原生连接句柄
    void add(const FaceData & data);      // 新增一条人脸特征记录
    void createTable();                   // 建 vec0 向量表（若不存在）
signals:

private:
    // 类似于之前 QSqlDatabase 的角色，但这是原生 sqlite3 连接句柄
    sqlite3 *m_db = nullptr;
};

#endif // FACEDATADAO_H
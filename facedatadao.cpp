#include "facedatadao.h"
#include <QDebug>
#include <QCoreApplication>

// 为什么不用 Qt SQL：Qt 自带的 SQLite 被裁剪掉了 load_extension，
// 无法加载 sqlite-vec.dll 扩展。所以本工程直接编译原生 sqlite3.c，
// 用 sqlite3_open / sqlite3_db_config / sqlite3_load_extension 这套 C API。
FaceDataDao::FaceDataDao(QObject *parent)
    : QObject{parent}
{
    // 1. 数据库与扩展都放在 exe 同级目录，避免原型里写死的绝对路径。
    //    QCoreApplication::applicationDirPath() 返回可执行文件所在目录。
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString dbPath = appDir + "/face.db";
    const QString vecDllPath = appDir + "/sqlite-vec.dll";

    int rc = sqlite3_open(dbPath.toUtf8().constData(), &m_db);
    if (rc != SQLITE_OK) {
        qDebug() << "打开数据库失败:" << sqlite3_errmsg(m_db) << "，数据库路径:" << dbPath;
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    // 2. 启用扩展加载（只开当前这条 C API 连接，不影响 Qt 内部）
    rc = sqlite3_db_config(m_db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    if (rc != SQLITE_OK) {
        qDebug() << "启用扩展加载失败:" << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    // 3. 加载 sqlite-vec.dll，显式指定入口点 sqlite3_vec_init
    char *errMsg = nullptr;
    rc = sqlite3_load_extension(m_db, vecDllPath.toUtf8().constData(),
                                "sqlite3_vec_init", &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "加载 sqlite-vec 失败:" << (errMsg ? errMsg : "unknown")
                 << "，扩展路径:" << vecDllPath;
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    qDebug() << "sqlite-vec 加载成功";

    // 建表（vec0 虚拟表，首次运行时创建）
    createTable();
}

// 析构：关闭原生 sqlite3 连接，避免句柄泄漏
FaceDataDao::~FaceDataDao()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void FaceDataDao::createTable()
{
    if (!m_db) {
        qDebug() << "建表失败: 数据库连接未打开";
        return;
    }
    // vec0 是 sqlite-vec 提供的虚拟表：
    // features float[128] 声明 128 维浮点向量；distance_metric=cosine 指定余弦距离，
    // 后续做 1:N 人脸比对时可直接用 KNN 查询，避免全表扫描算距离。
    const char *sql =
        "CREATE VIRTUAL TABLE IF NOT EXISTS t_face_data USING vec0("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sn Text,"
        "  name TEXT,"
        "  phone TEXT,"
        "  address TEXT,"
        "  features float[128] distance_metric=cosine"
        ");";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "建表失败:" << (errMsg ? errMsg : "unknown");
        if (errMsg) sqlite3_free(errMsg);
    } else {
        qDebug() << "建表成功!";
    }
}

void FaceDataDao::add(const FaceData &data)
{
    if (!m_db) {
        qDebug() << "插入失败: 数据库连接未打开";
        return;
    }
    // 用预编译语句 + 参数绑定，避免 SQL 注入；features 以 BLOB 绑定原始 512 字节
    const char *sql =
        "INSERT INTO t_face_data(sn,name, phone, address, features) "
        "VALUES (?,?, ?, ?, ?);";

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qDebug() << "prepare 失败:" << sqlite3_errmsg(m_db);
        return;
    }

    sqlite3_bind_text(stmt, 1, data.getSn().toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, data.getName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, data.getPhone().toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, data.getAddress().toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 5, data.getFeatures().constData(), data.getFeatures().size(), SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (!ok) {
        qDebug() << "插入失败:" << sqlite3_errmsg(m_db);
    } else {
        qDebug() << "新增数据成功.............";
    }

    sqlite3_finalize(stmt);
}

std::optional<FaceMatchResult> FaceDataDao::findNearest(
    const std::vector<float> &queryFeatures)
{
    if (!m_db) {
        qDebug() << "查询失败: 数据库连接未打开";
        return std::nullopt;
    }
    // 维度必须和 vec0 里声明的 float[128] 完全一致，否则 blob 长度对不上
    if (queryFeatures.size() != 128) {
        qDebug() << "查询失败: 特征维度不匹配，当前维度:" << queryFeatures.size();
        return std::nullopt;
    }

    const char *sql =
        "SELECT id, name, distance FROM t_face_data "
        "WHERE features MATCH ? AND k = 1;";   // k=1：只取最相似的一条

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qDebug() << "prepare 失败:" << sqlite3_errmsg(m_db);
        return std::nullopt;
    }

    // 把 vector<float> 按原始字节绑定到第 1 个 ?，SQLITE_TRANSIENT 让 SQLite 自己复制
    sqlite3_bind_blob(stmt, 1, queryFeatures.data(),
                      static_cast<int>(queryFeatures.size() * sizeof(float)),
                      SQLITE_TRANSIENT);

    FaceMatchResult result;
    bool found = false;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        result.id       = sqlite3_column_int(stmt, 0);
        result.name     = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        result.distance = static_cast<float>(sqlite3_column_double(stmt, 2));
        found = true;
    } else if (rc != SQLITE_DONE) {
        qDebug() << "查询执行失败:" << sqlite3_errmsg(m_db);
    }
    sqlite3_finalize(stmt);

    if (!found) {
        qDebug() << "库中无匹配记录";
        return std::nullopt;   // 空库 / 一条都没查到
    }
    return result;
}
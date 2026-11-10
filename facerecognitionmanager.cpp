// ===== facerecognitionmanager.cpp =====
// 门面类：两条 QThread + QVideoSink，把摄像头帧送进识别线程。

#include "facerecognitionmanager.h"
#include <QVideoFrameFormat>
#include <QVideoFrame>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>

// 编译期兜底路径：由 CMakeLists.txt 的 target_compile_definitions 传入
#ifndef FACE_INPUT_EXE
#define FACE_INPUT_EXE ""
#endif

FaceRecognitionManager::FaceRecognitionManager(QObject *parent)
    : QObject{parent}
{
    this->cameraWorker = new CameraWorker;
    this->cameraWorkerThread = new QThread;
    this->cameraWorker->moveToThread(this->cameraWorkerThread);

    this->recogWorker = new FaceRecognitionHandleWorker;
    this->recogWorkerThread = new QThread;
    this->recogWorker->moveToThread(this->recogWorkerThread);
    //采集->识别
    connect(this->cameraWorker,&CameraWorker::handleFrame,this->recogWorker,&FaceRecognitionHandleWorker::handleFaceRecognition);
    //识别完成->采集
    connect(this->recogWorker,&FaceRecognitionHandleWorker::frameHandleFinished,this->cameraWorker,&CameraWorker::frameHandleFinished);
    connect(this->cameraWorker,&CameraWorker::frameReady,this,&FaceRecognitionManager::showFrame);
    connect(this,&FaceRecognitionManager::notifyOpenCamera,this->cameraWorker,&CameraWorker::openCamera);
    connect(this,&FaceRecognitionManager::notifyCloseCamera,this->cameraWorker,&CameraWorker::closeCamera);
    // 摄像头真正释放完成后才启动录入程序（跨线程自动队列连接，回到 GUI 线程执行）
    connect(this->cameraWorker,&CameraWorker::cameraClosed,this,&FaceRecognitionManager::startPendingFaceInput);

    this->cameraWorkerThread->start();
    this->recogWorkerThread->start();
}

// 启动独立的人脸录入程序
// USB 摄像头是独占设备：主程序不释放，录入程序就打不开，所以流程是
// 「先关摄像头 -> 再启动录入程序 -> 录入程序退出后自动重开摄像头」。
bool FaceRecognitionManager::launchFaceInput()
{
    // 路径优先级：环境变量 > 编译期宏，方便不重新编译就换路径
    QString exePath = qEnvironmentVariable("FACE_INPUT_EXE");
    if (exePath.isEmpty())
        exePath = QStringLiteral(FACE_INPUT_EXE);

    if (exePath.isEmpty() || !QFileInfo::exists(exePath)) {
        qWarning() << "人脸录入程序不存在，请先编译 faceInput 工程:" << exePath;
        return false;
    }

    // 已经在运行就不重复启动，避免误把摄像头关掉
    if (m_faceInputProcess && m_faceInputProcess->state() != QProcess::NotRunning) {
        qWarning() << "人脸录入程序已在运行，忽略本次点击";
        return false;
    }

    // 首次调用时创建进程对象，并绑定启动/退出/启动失败的回调
    if (!m_faceInputProcess) {
        m_faceInputProcess = new QProcess(this);
        // 让子进程输出直接进当前终端，否则看不到它自己的 qDebug
        m_faceInputProcess->setProcessChannelMode(QProcess::ForwardedChannels);

        // 起来了：打印 pid，确认到底有没有真正启动
        connect(m_faceInputProcess, &QProcess::started, this, [this]() {
            qDebug() << "人脸录入程序已启动, pid =" << m_faceInputProcess->processId();
        });

        // 退出：打印退出码，这是判断闪退原因的关键
        connect(m_faceInputProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus status) {
                    if (status == QProcess::CrashExit || code != 0) {
                        qWarning() << "人脸录入程序异常退出, exitCode =" << code
                                   << "(-1073741515 一般是缺 DLL, -1073741819 一般是访问冲突)";
                    } else {
                        qDebug() << "人脸录入程序正常退出";
                    }
                    QTimer::singleShot(800, this, [this]() { emit this->notifyOpenCamera(); });
                });

        // 启动失败：打印具体原因
        connect(m_faceInputProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
            qWarning() << "人脸录入程序进程错误:" << err << m_faceInputProcess->errorString();
            if (err == QProcess::FailedToStart)
                QTimer::singleShot(800, this, [this]() { emit this->notifyOpenCamera(); });
        });
    }

    // ① 记下待启动的 exe，等摄像头线程回报 cameraClosed 后再真正启动
    m_pendingFaceInputExe = exePath;
    emit this->notifyCloseCamera();

    // ② 兜底：摄像头本来就没打开时 cameraClosed 可能不来，1.5 秒后强制启动
    QTimer::singleShot(1500, this, [this]() {
        if (!m_pendingFaceInputExe.isEmpty())
            this->startPendingFaceInput();
    });

    return true;
}

// 摄像头线程回报“设备已释放”后调用；用空标记保证只启动一次
void FaceRecognitionManager::startPendingFaceInput()
{
    if (m_pendingFaceInputExe.isEmpty())
        return;
    const QString exePath = m_pendingFaceInputExe;
    m_pendingFaceInputExe.clear();
    this->startFaceInputProcess(exePath);
}

void FaceRecognitionManager::startFaceInputProcess(const QString &exePath)
{
    m_faceInputProcess->setWorkingDirectory(QFileInfo(exePath).absolutePath());
    // 不调 waitForStarted：它会阻塞 GUI 线程，左下角时间会停
    m_faceInputProcess->start(exePath);
    qDebug() << "已发起启动人脸录入程序:" << exePath;
}

void FaceRecognitionManager::openCamera()
{
    emit this->notifyOpenCamera();
}

QVideoSink *FaceRecognitionManager::displaySink()
{
    return this->m_displaySink;
}

void FaceRecognitionManager::setDisplaySink(QVideoSink *sink)
{
    this->m_displaySink = sink;
    emit displaySinkChanged();
}

void FaceRecognitionManager::showFrame(const QImage &image)
{
    if (!m_displaySink || image.isNull()) return;

    QVideoFrameFormat fmt(image.size(), QVideoFrameFormat::Format_RGBX8888);
    QVideoFrame frame(fmt);
    if (!frame.map(QVideoFrame::WriteOnly)) return;

    QImage converted = image.convertToFormat(QImage::Format_RGBX8888);

    // 关键：单平面格式，一次性拷贝整个缓冲区
    memcpy(frame.bits(0), converted.constBits(), converted.sizeInBytes());

    frame.unmap();
    m_displaySink->setVideoFrame(frame);
}
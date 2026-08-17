#include "logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

constexpr qint64 kMaxLogFileSize = 5 * 1024 * 1024;    /**< 单日志文件最大 5MB */
constexpr int kMaxBackupCount = 5;                     /**< 备份文件数量 */

/**
 * @brief 将 Qt 消息级别转换为文本
 */
QString levelToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return "DEBUG";
    case QtInfoMsg:     return "INFO";
    case QtWarningMsg:  return "WARNING";
    case QtCriticalMsg: return "CRITICAL";
    case QtFatalMsg:    return "FATAL";
    default:            return "INFO";
    }
}

/**
 * @brief 判断级别是否应写入错误日志（CRITICAL 及以上）
 */
bool isErrorLevel(QtMsgType type)
{
    return type == QtCriticalMsg || type == QtFatalMsg;
}

} // namespace

// ============================================================
// 单例与初始化
// ============================================================

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
    if (m_errorFile.isOpen()) {
        m_errorFile.close();
    }
}

void Logger::initialize()
{
    if (m_initialized) {
        return;
    }
    m_initialized = true;

    // 日志目录：Windows 使用 %APPDATA%/MultiClipboard/logs，Linux 使用 ~/.local/share/MultiClipboard/logs
    // 注意：此处早于 QApplication 创建（applicationName 尚未设置），
    // 必须使用 GenericConfigLocation 等不依赖应用名的位置，保证路径与文档一致
    QString base;
#ifdef Q_OS_WIN
    base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
#else
    base = QDir::homePath() + "/.local/share";
#endif
    m_logDir = base + "/MultiClipboard/logs";
    QDir().mkpath(m_logDir);

    m_logFilePath = m_logDir + "/multiclipboard.log";
    m_errorLogPath = m_logDir + "/error.log";

    m_logFile.setFileName(m_logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "无法打开日志文件:" << m_logFilePath;
    }
    m_errorFile.setFileName(m_errorLogPath);
    if (!m_errorFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "无法打开错误日志文件:" << m_errorLogPath;
    }

    qInfo() << QString("%1").arg(QString("-").repeated(60));
    qInfo() << "多元剪贴板 启动";
    qInfo() << QString("日志目录: %1").arg(m_logDir);
}

// ============================================================
// 路径访问
// ============================================================

QString Logger::logDir() const
{
    return m_logDir;
}

QString Logger::errorLogPath() const
{
    return m_errorLogPath;
}

// ============================================================
// 消息处理
// ============================================================

void Logger::setupGlobalHandler()
{
    qInstallMessageHandler(messageHandler);
    qInfo() << "全局消息处理器已安装";
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Logger& logger = Logger::instance();

    // 格式：时间 [级别] [文件:行] 消息
    const QString line = QString("%1 [%2] [%3:%4] %5")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                             .arg(levelToString(type))
                             .arg(context.file != nullptr ? QFileInfo(QString::fromUtf8(context.file)).fileName()
                                                          : QStringLiteral("-"))
                             .arg(context.line)
                             .arg(msg);

    logger.writeToFile(levelToString(type), line, isErrorLevel(type));
}

void Logger::writeToFile(const QString& level, const QString& line, bool isError)
{
    rotateIfNeeded();
    const QByteArray data = (line + "\n").toUtf8();
    if (m_logFile.isOpen()) {
        m_logFile.write(data);
        m_logFile.flush();
    }
    if (isError && m_errorFile.isOpen()) {
        m_errorFile.write(data);
        m_errorFile.flush();
    }
}

void Logger::rotateIfNeeded()
{
    // 超过大小限制时轮转：删除最旧的备份，依次后移
    if (m_logFile.size() < kMaxLogFileSize) {
        return;
    }
    m_logFile.close();

    const QString oldest = QString("%1.%2").arg(m_logFilePath).arg(kMaxBackupCount);
    QFile::remove(oldest);
    for (int i = kMaxBackupCount - 1; i >= 1; --i) {
        QFile::rename(QString("%1.%2").arg(m_logFilePath).arg(i),
                      QString("%1.%2").arg(m_logFilePath).arg(i + 1));
    }
    QFile::rename(m_logFilePath, m_logFilePath + ".1");

    m_logFile.setFileName(m_logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "轮转后无法重新打开日志文件:" << m_logFilePath;
    }
}

// ============================================================
// 崩溃报告
// ============================================================

void Logger::writeCrashReport(const QString& detail)
{
    const QString path = m_logDir + QString("/crash_%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(detail.toUtf8());
        file.close();
        qInfo() << QString("崩溃报告已保存: %1").arg(path);
    }
}

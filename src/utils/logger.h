#pragma once

#include <QFile>
#include <QString>
#include <QtCore/qlogging.h>    // QtMsgType / QMessageLogContext（兼容无 QtLogging 聚合头的发行版）

/**
 * @brief 日志管理器（单例）
 *
 * 提供统一的日志管理，支持：
 * - 文件输出（持久化，用于崩溃分析）
 * - 日志文件自动轮转（保留最近 5 个日志文件，每个最大 5MB）
 * - 错误日志单独文件（error.log，仅 ERROR 及以上）
 * - 崩溃报告（crash_YYYYMMDD_HHMMSS.log）
 *
 * 通过 qInstallMessageHandler 捕获全部 qDebug/qInfo/qWarning/qCritical 消息。
 */
class Logger
{
public:
    /**
     * @brief 获取全局唯一实例
     */
    static Logger& instance();

    /**
     * @brief 初始化日志系统（幂等）
     */
    void initialize();

    /**
     * @brief 获取日志目录路径
     */
    QString logDir() const;

    /**
     * @brief 获取错误日志文件路径
     */
    QString errorLogPath() const;

    /**
     * @brief 安装全局消息处理器（捕获 Qt 消息并写入日志文件）
     */
    void setupGlobalHandler();

    /**
     * @brief 写出崩溃报告文件
     * @param detail 崩溃详情文本
     */
    void writeCrashReport(const QString& detail);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Qt 全局消息处理回调
     */
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    /**
     * @brief 写入日志文件（自动轮转）
     * @param level 级别文本（如 "INFO"）
     * @param line 完整日志行
     * @param isError 是否同时写入错误日志
     */
    void writeToFile(const QString& level, const QString& line, bool isError);

    /**
     * @brief 按大小轮转当前日志文件（超过 5MB 时重命名）
     */
    void rotateIfNeeded();

    QString m_logDir;           /**< 日志目录路径 */
    QString m_logFilePath;      /**< 主日志文件路径 */
    QString m_errorLogPath;     /**< 错误日志文件路径 */
    QFile m_logFile;            /**< 主日志文件 */
    QFile m_errorFile;          /**< 错误日志文件 */
    bool m_initialized = false; /**< 是否已初始化 */
};

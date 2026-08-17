#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <memory>

class ConfigManager;
class PlatformInterface;

/**
 * @brief 剪贴板管理器
 *
 * 负责监控剪贴板变化、获取剪贴板内容，并提供内容变化通知。
 * 使用平台抽象层实现跨平台支持。
 */
class ClipboardManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 配置管理器实例，为空时内部创建
     * @param parent 父对象
     */
    explicit ClipboardManager(ConfigManager* config = nullptr, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~ClipboardManager() override;

    /**
     * @brief 剪贴板内容变化信号，携带新的文本内容
     */
    Q_SIGNAL void clipboardChanged(const QString& text);

    /**
     * @brief 启动剪贴板监控（按配置间隔轮询）
     */
    void startMonitoring();

    /**
     * @brief 停止剪贴板监控
     */
    void stopMonitoring();

    /**
     * @brief 获取剪贴板文本内容
     * @return 剪贴板文本，无文本时返回空字符串
     */
    QString getText();

    /**
     * @brief 设置剪贴板文本内容
     * @param text 要设置的文本
     * @return 是否成功
     */
    bool setText(const QString& text);

    /**
     * @brief 检查剪贴板是否有文本内容
     * @return 是否有文本内容
     */
    bool isTextAvailable();

    /**
     * @brief 获取剪贴板历史记录（当前返回空列表，保留接口）
     * @return 历史记录列表
     */
    QStringList getHistory();

    /**
     * @brief 清空剪贴板
     */
    void clear();

    /**
     * @brief 设置监控间隔
     * @param interval 间隔时间（毫秒）
     */
    void setMonitorInterval(int interval);

    /**
     * @brief 停止监控并清理资源
     */
    void stop();

    /**
     * @brief 获取最近一次记录到的剪贴板文本
     */
    QString lastText() const { return m_lastText; }

    /**
     * @brief 设置最近一次记录到的剪贴板文本（供 DragManager 同步使用）
     */
    void setLastText(const QString& text) { m_lastText = text; }

private:
    /**
     * @brief 检查剪贴板内容是否变化（定时器回调）
     */
    void checkClipboard();

    /**
     * @brief 检查剪贴板当前内容是否为非文本类型（文件、图片等）
     * @return true 表示是非文本内容，应忽略
     */
    bool isNonTextContent();

    ConfigManager* m_config = nullptr;                          /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;                 /**< 自有的配置管理器（config 为空时使用） */
    std::shared_ptr<PlatformInterface> m_platform;              /**< 平台适配器 */
    QJsonObject m_clipboardConfig;                              /**< 剪贴板配置 */
    QTimer* m_monitorTimer = nullptr;                           /**< 监控定时器 */
    QString m_lastText;                                         /**< 最近一次记录到的剪贴板文本 */
    qint64 m_lastCheckTime = 0;                                 /**< 上次检查时间戳（毫秒） */
};

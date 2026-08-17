#pragma once

#include <QObject>
#include <QHash>
#include <QPair>
#include <functional>
#include <memory>

class ConfigManager;
class PlatformInterface;

/**
 * @brief 全局快捷键管理器
 *
 * 负责全局快捷键的注册、监听和处理。
 * 使用平台抽象层实现跨平台支持。
 */
class HotkeyManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 配置管理器实例，为空时内部创建
     * @param parent 父对象
     */
    explicit HotkeyManager(ConfigManager* config = nullptr, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~HotkeyManager() override;

    /**
     * @brief 注册所有配置的快捷键，并安装消息过滤器
     */
    void registerAllHotkeys();

    /**
     * @brief 重新加载所有快捷键
     */
    void reloadHotkeys();

    /**
     * @brief 注册单个快捷键
     * @param shortcutStr 快捷键字符串（如 "Ctrl+Shift+T"）
     * @param callback 回调函数
     * @return 注册的快捷键 ID，失败返回 0
     */
    int registerHotkey(const QString& shortcutStr, const std::function<void()>& callback);

    /**
     * @brief 注销单个快捷键
     * @param hotkeyId 快捷键 ID
     * @return 是否注销成功
     */
    bool unregisterHotkey(int hotkeyId);

    /**
     * @brief 注销所有快捷键
     */
    void unregisterAllHotkeys();

    /**
     * @brief 设置回调函数
     * @param toggleAlwaysOnTop 切换窗口置顶
     * @param toggleWindow 切换窗口显示/隐藏
     * @param clearAll 清空所有条目
     * @param copyAll 复制所有条目
     * @param pastePlain 纯文本粘贴
     */
    void setCallbacks(const std::function<void()>& toggleAlwaysOnTop,
                      const std::function<void()>& toggleWindow,
                      const std::function<void()>& clearAll,
                      const std::function<void()>& copyAll,
                      const std::function<void()>& pastePlain);

    /**
     * @brief 处理热键消息
     * @param hotkeyId 热键 ID
     * @return true 表示已处理，false 表示未找到对应的热键
     */
    bool handleHotkey(int hotkeyId);

    /**
     * @brief 停止快捷键监听
     */
    void stop();

private:
    /**
     * @brief 解析快捷键字符串
     * @param shortcutStr 快捷键字符串（如 "Ctrl+Shift+T"）
     * @return (modifiers, vkCode) 对，解析失败返回 (0, 0)
     */
    QPair<quint32, quint32> parseShortcut(const QString& shortcutStr);

    ConfigManager* m_config = nullptr;                          /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;                 /**< 自有的配置管理器（config 为空时使用） */
    std::shared_ptr<PlatformInterface> m_platform;              /**< 平台适配器 */
    QHash<int, std::function<void()>> m_hotkeys;                /**< 热键 ID -> 回调 */
    QHash<QString, int> m_registeredShortcuts;                  /**< 快捷键名 -> 热键 ID */
    int m_hotkeyIdCounter = 100;                                /**< 热键 ID 计数器 */

    // 回调函数
    std::function<void()> m_toggleAlwaysOnTopCb;                /**< 切换窗口置顶回调 */
    std::function<void()> m_toggleWindowCb;                     /**< 切换窗口回调 */
    std::function<void()> m_clearAllCb;                         /**< 清空条目回调 */
    std::function<void()> m_copyAllCb;                          /**< 复制全部回调 */
    std::function<void()> m_pastePlainCb;                       /**< 纯文本粘贴回调 */
};

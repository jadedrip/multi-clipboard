#pragma once

#include <QObject>
#include <QString>
#include <memory>

class ConfigManager;
class PlatformInterface;
class ClipboardManager;
class QWidget;
struct Item;

/**
 * @brief 拖拽管理器
 *
 * 负责处理条目拖拽操作，实现双路径策略：
 * - 路径 A：Qt QDrag 原生拖拽
 * - 路径 B：剪贴板写入 + 模拟鼠标点击 + 模拟 Ctrl+V
 * 当路径 A 失败时自动切换到路径 B。
 * 使用平台抽象层实现跨平台支持。
 */
class DragManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 配置管理器实例，为空时内部创建
     * @param clipboardManager 剪贴板管理器实例（用于同步 last_text，避免误触发列表刷新）
     * @param parent 父对象
     */
    explicit DragManager(ConfigManager* config = nullptr,
                         ClipboardManager* clipboardManager = nullptr,
                         QObject* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~DragManager() override;

    /**
     * @brief 启动拖拽操作
     *
     * 优先使用 QDrag 原生拖拽（路径 A），失败时自动回退到路径 B。
     *
     * @param widget 发起拖拽的 Qt 部件
     * @param item 要拖拽的条目
     * @return true 表示拖拽完成（内容已被外部接受），false 表示取消
     */
    bool startDrag(QWidget* widget, const Item& item);

    /**
     * @brief 直接复制到剪贴板并模拟粘贴
     * @param item 要粘贴的条目
     * @return 是否成功
     */
    bool copyToClipboardAndPaste(const Item& item);

    /**
     * @brief 仅复制到剪贴板（不粘贴），通过 ClipboardManager 保持同步
     * @param item 要复制的条目
     * @return 是否成功
     */
    bool copyOnly(const Item& item);

    /**
     * @brief 直接复制文本到剪贴板
     * @param text 要复制的文本
     * @return 是否成功
     */
    bool copyText(const QString& text);

private:
    /**
     * @brief 执行备选路径（路径 B）
     *
     * 步骤：获取鼠标位置窗口句柄 -> 激活目标窗口 -> 模拟鼠标左键单击 ->
     * 将内容写入系统剪贴板 -> 模拟 Ctrl+V 粘贴。
     *
     * @param item 要粘贴的条目
     */
    void fallbackToPlanB(const Item& item);

    /**
     * @brief 写入剪贴板（通过 ClipboardManager 保持 last_text 同步）
     * @param text 要写入的文本
     */
    void setClipboardText(const QString& text);

    ConfigManager* m_config = nullptr;                          /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;                 /**< 自有的配置管理器（config 为空时使用） */
    ClipboardManager* m_clipboardManager = nullptr;             /**< 剪贴板管理器指针 */
    std::shared_ptr<PlatformInterface> m_platform;              /**< 平台适配器 */
    Item* m_currentItem = nullptr;                              /**< 当前拖拽的条目 */
};

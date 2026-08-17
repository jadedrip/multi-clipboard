#pragma once

#include <QFrame>
#include <QPoint>
#include <QVector>
#include <memory>

#include "core/item.h"

class ConfigManager;
class DragManager;
class QCheckBox;
class QLabel;
class ElideLabel;

/**
 * @brief 条目卡片组件
 *
 * 负责显示单个条目卡片，处理拖拽、双击复制、右键菜单等交互。
 * 拖拽与双击由 Qt 原生事件机制区分，无需额外定时器。
 */
class ItemWidget : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief 拖拽判断阈值（像素）
     */
    static constexpr int kDragThreshold = 5;

    /**
     * @brief 构造函数
     * @param item 条目数据
     * @param config 配置管理器实例
     * @param dragManager 拖拽管理器实例
     * @param parent 父部件
     */
    explicit ItemWidget(const Item& item,
                        ConfigManager* config = nullptr,
                        DragManager* dragManager = nullptr,
                        QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~ItemWidget() override;

    // ==================== 信号 ====================

    /**
     * @brief 条目标记为已使用信号
     */
    Q_SIGNAL void itemUsed(Item* item);

    /**
     * @brief 条目标记为未使用信号
     */
    Q_SIGNAL void itemUnused(Item* item);

    /**
     * @brief 条目复制到剪贴板信号
     */
    Q_SIGNAL void itemCopied(Item* item);

    /**
     * @brief 条目持久化状态变化信号
     */
    Q_SIGNAL void itemPersistentChanged(Item* item, bool persistent);

    /**
     * @brief 设置主题
     * @param theme 主题名称，"light" 或 "dark"
     */
    void setTheme(const QString& theme);

    /**
     * @brief 设置搜索匹配状态
     * @param match true 表示匹配搜索词（正常显示），false 表示不匹配（淡化显示）
     */
    void setSearchMatch(bool match);

    /**
     * @brief 更新状态显示（根据 used 状态重新应用样式）
     */
    void updateStatusDisplay();

    /**
     * @brief 设置条目使用状态
     * @param used 是否已使用
     */
    void setUsed(bool used);

    /**
     * @brief 设置条目数据
     * @param item 条目数据
     */
    void setItem(const Item& item);

    /**
     * @brief 获取当前条目数据的指针（供 MainWindow 使用）
     */
    Item* item() { return &m_item; }

    /**
     * @brief 获取当前条目数据的指针（只读）
     */
    const Item* item() const { return &m_item; }

protected:
    /**
     * @brief 鼠标按下事件：记录按下位置，供后续移动判断使用
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标移动事件：左键按住且移动超过阈值时启动拖拽
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标双击事件：复制到剪贴板
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * @brief 右键菜单事件：标记使用状态、复制、纯文本粘贴
     */
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    /**
     * @brief 初始化界面
     */
    void initUi();

    /**
     * @brief 更新 tooltip，鼠标悬停时显示完整文本内容
     */
    void updateTooltip();

    /**
     * @brief 获取当前主题的样式字符串
     * @param key 样式键名
     */
    QString styleKey(const QString& key) const;

    /**
     * @brief 根据使用状态和是否为原始条目应用完整样式
     * @param isUsed 是否已使用
     */
    void applyStyle(bool isUsed);

    /**
     * @brief 应用未使用样式
     */
    void applyNormalStyle();

    /**
     * @brief 应用已使用样式（淡绿背景 + 删除线）
     */
    void applyUsedStyle();

    /**
     * @brief 应用未分隔原始条目样式（紫色背景）
     * @param isUsed 是否已使用
     */
    void applyRawStyle(bool isUsed);

    /**
     * @brief 双击处理：复制到剪贴板
     */
    void onDoubleClick();

    /**
     * @brief 高亮闪烁效果（短暂蓝色高亮后恢复）
     */
    void flashHighlight();

    /**
     * @brief 恢复闪烁前的样式
     */
    void restoreFlashStyle();

    /**
     * @brief 启动拖拽操作（仅拖拽成功后才标注为已使用）
     */
    void startDrag();

    /**
     * @brief 复制条目内容到剪贴板
     */
    void copyToClipboard();

    /**
     * @brief 纯文本粘贴：复制条目内容并模拟 Ctrl+V
     */
    void pastePlainText();

    /**
     * @brief 持久化复选框状态变化处理
     */
    void onPersistentChanged(int state);

    Item m_item;                                            /**< 条目数据 */
    ConfigManager* m_config = nullptr;                      /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;             /**< 自有的配置管理器（config 为空时使用） */
    DragManager* m_dragManager = nullptr;                   /**< 拖拽管理器指针 */
    std::unique_ptr<DragManager> m_ownDragManager;          /**< 自有的拖拽管理器（dragManager 为空时使用） */

    QCheckBox* m_persistentCheckbox = nullptr;              /**< 持久化复选框 */
    QLabel* m_indexLabel = nullptr;                         /**< 序号标签 */
    ElideLabel* m_contentLabel = nullptr;                   /**< 内容标签（自动截断） */
    QLabel* m_statusLabel = nullptr;                        /**< 状态标签（√ 标记） */

    int m_itemHeight = 36;                                  /**< 条目高度 */
    QString m_currentTheme = "light";                       /**< 当前主题 */
    bool m_isDragging = false;                              /**< 是否正在拖拽 */
    QPoint m_pressPos;                                      /**< 鼠标按下位置 */
};

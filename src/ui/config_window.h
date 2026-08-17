#pragma once

#include <QDialog>
#include <QHash>
#include <memory>

class ConfigManager;
class QVBoxLayout;
class QGroupBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QCheckBox;
class HotkeyEditWidget;

/**
 * @brief 配置窗口
 *
 * 提供热键配置、窗口设置等应用设置功能，支持亮色和暗色主题切换。
 */
class ConfigWindow : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 配置管理器实例
     * @param parent 父部件
     */
    explicit ConfigWindow(ConfigManager* config = nullptr, QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~ConfigWindow() override;

    /**
     * @brief 热键配置变化信号
     */
    Q_SIGNAL void hotkeysChanged();

    /**
     * @brief 设置主题
     * @param themeName 主题名称，"light" 或 "dark"
     */
    void setTheme(const QString& themeName);

protected:
    /**
     * @brief 窗口显示事件：激活窗口
     */
    void showEvent(QShowEvent* event) override;

private:
    /**
     * @brief 初始化界面
     */
    void initUi();

    /**
     * @brief 应用主题样式
     */
    void applyTheme();

    /**
     * @brief 初始化快捷键配置分区
     * @param parentLayout 父布局
     */
    void initHotkeySection(QVBoxLayout* parentLayout);

    /**
     * @brief 初始化窗口设置分区
     * @param parentLayout 父布局
     */
    void initWindowSection(QVBoxLayout* parentLayout);

    /**
     * @brief 初始化按钮分区
     * @param parentLayout 父布局
     */
    void initButtonSection(QVBoxLayout* parentLayout);

    /**
     * @brief 重置为默认快捷键
     */
    void onReset();

    /**
     * @brief 确定按钮：保存配置并发出 hotkeysChanged 信号
     */
    void onOk();

    ConfigManager* m_config = nullptr;                              /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;                     /**< 自有的配置管理器（config 为空时使用） */
    QString m_currentTheme = "light";                               /**< 当前主题 */

    QGroupBox* m_groupBox = nullptr;                                /**< 快捷键分区组框 */
    QTableWidget* m_table = nullptr;                                /**< 快捷键表格 */
    QHash<QString, HotkeyEditWidget*> m_hotkeyEdits;                /**< 快捷键名 -> 编辑控件 */
    QLabel* m_hintLabel = nullptr;                                  /**< 提示标签 */
    QCheckBox* m_autoPopupCheckbox = nullptr;                       /**< 自动弹出复选框 */
    QPushButton* m_resetButton = nullptr;                           /**< 重置按钮 */
    QPushButton* m_cancelButton = nullptr;                          /**< 取消按钮 */
    QPushButton* m_okButton = nullptr;                              /**< 确定按钮 */
};

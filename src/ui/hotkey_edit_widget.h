#pragma once

#include <QLineEdit>

/**
 * @brief 热键编辑控件
 *
 * 只读输入框，聚焦后按下组合键即可捕获并显示快捷键文本。
 */
class HotkeyEditWidget : public QLineEdit
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父部件
     */
    explicit HotkeyEditWidget(QWidget* parent = nullptr);

    /**
     * @brief 快捷键设置信号，携带快捷键字符串（如 "Ctrl+Shift+T"）
     */
    Q_SIGNAL void hotkeySet(const QString& hotkey);

    /**
     * @brief 设置主题
     * @param themeName 主题名称，"light" 或 "dark"
     */
    void setTheme(const QString& themeName);

protected:
    /**
     * @brief 按键事件：捕获修饰键 + 主键组合
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    /**
     * @brief 获取按键名称（Qt 键码 -> 显示名，如 "A"、"F1"、"SPACE"）
     * @param key Qt 键码
     * @return 按键名称，不支持时返回空字符串
     */
    QString getKeyName(int key) const;
};

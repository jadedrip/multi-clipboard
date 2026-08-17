#pragma once

#include <QLabel>

/**
 * @brief 自动截断文本的标签控件
 *
 * 在 paintEvent 中绘制文本时自动根据可用宽度截断（带省略号）。
 * 多行文本仅显示第一行，并在末尾追加 "..." 提示。
 * 不向 QLabel 传递文本内容，避免 sizeHint 影响布局收缩。
 */
class ElideLabel : public QLabel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param text 完整文本
     * @param parent 父部件
     */
    explicit ElideLabel(const QString& text = QString(), QWidget* parent = nullptr);

    /**
     * @brief 设置完整文本（截断仅在绘制时进行）
     * @param text 完整文本
     */
    void setFullText(const QString& text);

    /**
     * @brief 获取完整文本
     */
    QString fullText() const { return m_fullText; }

    /**
     * @brief 获取首行文本（多行文本只取第一行）
     * @return 首行文本
     */
    QString displayText() const;

    /**
     * @brief 返回最小尺寸提示，允许标签缩小到 0 宽度
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief 绘制事件：用 QFontMetrics::elidedText 自动截断文本
     */
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_fullText;     /**< 完整文本 */
};

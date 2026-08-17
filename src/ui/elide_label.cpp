#include "elide_label.h"

#include <QPainter>
#include <QFontMetrics>
#include <QSize>
#include <QSizePolicy>

/**
 * @brief 构造函数
 * @param text 完整文本
 * @param parent 父部件
 */
ElideLabel::ElideLabel(const QString& text, QWidget* parent)
    : QLabel(QString(), parent)
    , m_fullText(text)
{
    setWordWrap(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

/**
 * @brief 设置完整文本（截断仅在绘制时进行）
 * @param text 完整文本
 */
void ElideLabel::setFullText(const QString& text)
{
    m_fullText = text;
    update();
}

/**
 * @brief 获取首行文本（多行文本只取第一行）
 * @return 首行文本
 */
QString ElideLabel::displayText() const
{
    if (m_fullText.contains('\n')) {
        return m_fullText.section('\n', 0, 0);
    }
    return m_fullText;
}

/**
 * @brief 返回最小尺寸提示，允许标签缩小到 0 宽度
 */
QSize ElideLabel::minimumSizeHint() const
{
    return QSize(0, QLabel::minimumSizeHint().height());
}

/**
 * @brief 绘制事件：用 QFontMetrics::elidedText 自动截断文本
 */
void ElideLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setFont(font());
    painter.setPen(palette().color(QPalette::WindowText));

    QRect rect = this->rect();
    QString display = displayText();

    // 多行文本在第一行末尾添加 ... 提示
    if (m_fullText.contains('\n')) {
        display += QStringLiteral("...");
    }

    // 根据实际绘制区域宽度截断文本
    QFontMetrics fm = fontMetrics();
    QString elided = fm.elidedText(display, Qt::ElideRight, rect.width());
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, elided);
}

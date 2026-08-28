#include "window_drag_filter.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

#include "item_widget.h"

/**
 * @brief 构造函数：安装事件过滤器到 watched 上
 * @param watched 被监视的部件
 * @param targetWindow 要移动的目标窗口
 * @param parent QObject 父对象
 */
WindowDragFilter::WindowDragFilter(QWidget* watched, QWidget* targetWindow, QObject* parent)
    : QObject(parent)
    , m_targetWindow(targetWindow)
{
    if (watched != nullptr) {
        watched->installEventFilter(this);
    }
}

/**
 * @brief 事件过滤：处理左键按下、移动、释放，实现窗口拖动
 *
 * 只消费左键相关的鼠标事件，其他事件一律放行，不影响既有交互。
 *
 * @param watched 被监视对象
 * @param event 事件
 * @return true 表示事件已处理，false 表示继续传播
 */
bool WindowDragFilter::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        // 左键按下：记录起始位置，等待后续移动判断
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // 按下位置落在条目卡片上时不触发窗体拖拽，
            // 避免拖动条目时窗体随之移动（条目拖拽由 ItemWidget 自行处理）
            if (isOnItemWidget(watched, mouseEvent->position().toPoint())) {
                return false; // 放行，不消费事件
            }
            m_pressGlobalPos = mouseEvent->globalPosition().toPoint();
            m_pressWindowPos = m_targetWindow->pos();
            m_dragging = false;
            return true; // 消费按下事件，使本部件成为鼠标抓取者
        }
        break;
    }
    case QEvent::MouseMove: {
        // 按住左键移动：超过阈值后进入拖拽，随鼠标位移移动窗口
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!(mouseEvent->buttons() & Qt::LeftButton)) {
            break;
        }
        const QPoint currentPos = mouseEvent->globalPosition().toPoint();
        if (!m_dragging) {
            const int distance = (currentPos - m_pressGlobalPos).manhattanLength();
            if (distance < m_dragThreshold) {
                break; // 移动距离不足阈值，继续观察
            }
            m_dragging = true;
        }
        if (m_targetWindow != nullptr) {
            // 目标位置 = 按下时窗口位置 + 鼠标位移
            m_targetWindow->move(m_pressWindowPos + (currentPos - m_pressGlobalPos));
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        // 左键释放：结束拖拽
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_dragging = false;
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

/**
 * @brief 判断鼠标位置是否落在条目卡片（ItemWidget）上
 *
 * 从按下位置的最深层子部件沿父链向上查找，命中条目卡片即认为在条目上，
 * 此时不触发窗体拖拽，避免与条目自身的拖拽逻辑冲突。
 *
 * @param watched 被监视部件
 * @param pos 被监视部件内的坐标
 * @return true 表示落在条目卡片上
 */
bool WindowDragFilter::isOnItemWidget(QObject* watched, const QPoint& pos) const
{
    auto* widget = qobject_cast<QWidget*>(watched);
    if (widget == nullptr) {
        return false;
    }
    QWidget* child = widget->childAt(pos);
    while (child != nullptr && child != widget) {
        if (qobject_cast<ItemWidget*>(child) != nullptr) {
            return true;
        }
        child = child->parentWidget();
    }
    return false;
}

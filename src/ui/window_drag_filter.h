#pragma once

#include <QObject>
#include <QPoint>

class QEvent;
class QWidget;

/**
 * @brief 窗口拖拽移动过滤器
 *
 * 安装到指定部件上，实现"按住鼠标左键并拖动该部件时移动整个窗口"。
 * 典型用途：无标题栏或标题栏过窄的窗口，通过列表空白区域拖动移动窗体。
 *
 * 工作原理：
 * 1. 左键按下时记录鼠标全局坐标与目标窗口位置，并消费事件使被监视部件
 *    成为鼠标抓取者（保证拖动期间持续收到移动事件）；
 * 2. 按住左键移动超过阈值后进入拖拽状态，目标位置 = 按下时窗口位置 + 鼠标位移，
 *    调用 move() 实时移动窗口；
 * 3. 左键释放后结束拖拽。
 *
 * 点击条目卡片等子控件时，事件由子控件自行消费，不会传播到过滤器，
 * 因此条目自身的拖拽逻辑与窗体移动互不干扰。
 */
class WindowDragFilter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数（自动将过滤器安装到 watched 上）
     * @param watched 被监视的部件（鼠标事件来源，如列表滚动容器的内容部件）
     * @param targetWindow 要移动的目标窗口（一般为顶层窗口）
     * @param parent QObject 父对象
     */
    explicit WindowDragFilter(QWidget* watched, QWidget* targetWindow, QObject* parent = nullptr);

protected:
    /**
     * @brief 事件过滤：处理左键按下、移动、释放，实现窗口拖动
     * @param watched 被监视对象
     * @param event 事件
     * @return true 表示事件已处理，false 表示继续传播
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* m_targetWindow = nullptr;  /**< 要移动的目标窗口 */
    QPoint m_pressGlobalPos;            /**< 按下时鼠标的全局坐标 */
    QPoint m_pressWindowPos;            /**< 按下时目标窗口的位置 */
    bool m_dragging = false;            /**< 是否处于拖拽移动状态 */
    int m_dragThreshold = 4;            /**< 触发拖拽的最小移动距离（像素） */
};

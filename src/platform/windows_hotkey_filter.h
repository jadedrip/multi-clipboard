#pragma once

#include <QAbstractNativeEventFilter>
#include <functional>

/**
 * @brief Windows 原生事件过滤器
 *
 * 用于处理 WM_HOTKEY 热键消息，将热键事件分发到回调函数。
 */
class WindowsHotkeyFilter : public QAbstractNativeEventFilter
{
public:
    /**
     * @brief 构造函数
     * @param callback 热键回调函数，接收热键 ID，返回是否已处理
     */
    explicit WindowsHotkeyFilter(const std::function<bool(int)>& callback);

    /**
     * @brief 过滤并处理 Windows 原生事件
     * @param eventType 事件类型
     * @param message 消息指针
     * @param result 处理结果
     * @return true 表示事件已处理
     */
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    std::function<bool(int)> m_callback;    /**< 热键回调 */
};

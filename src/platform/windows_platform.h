#pragma once

#include <QHash>
#include <QPair>
#include <functional>

#include "platform_interface.h"

class WindowsHotkeyFilter;

/**
 * @brief Windows 平台适配器实现
 *
 * 实现 Windows 平台的具体 API：
 * - RegisterHotKey/UnregisterHotKey 全局热键
 * - SendInput 鼠标/键盘模拟
 * - WindowFromPoint 窗口操作
 */
class WindowsPlatform : public PlatformInterface
{
public:
    WindowsPlatform();
    ~WindowsPlatform() override;

    // 平台信息
    QString getPlatformName() const override;

    // 热键管理
    bool registerHotkey(int hotkeyId, quint32 modifiers, quint32 key) override;
    bool unregisterHotkey(int hotkeyId) override;
    void unregisterAllHotkeys() override;
    bool installHotkeyListener(const std::function<bool(int)>& callback) override;
    void removeHotkeyListener() override;

    // 鼠标/键盘模拟
    QPoint getCursorPosition() override;
    bool setCursorPosition(int x, int y) override;
    bool simulateMouseClick(int x, int y) override;
    bool simulateKeyPress(const QVector<quint32>& keyCodes) override;
    bool simulateCtrlV() override;

    // 窗口操作
    WId getWindowAtPosition(int x, int y) override;
    bool setForegroundWindow(WId hwnd) override;

    // 剪贴板增强
    bool isFileCopyContent(const QMimeData* mimeData) override;

    // 资源清理
    void cleanup() override;

private:
    WindowsHotkeyFilter* m_hotkeyFilter = nullptr;                  /**< 热键事件过滤器 */
    QHash<int, QPair<quint32, quint32>> m_registeredHotkeys;        /**< 已注册热键表（id -> (modifiers, key)） */
};

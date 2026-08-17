#include "drag_manager.h"

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QMimeData>
#include <QThread>
#include <QWidget>

#include "core/clipboard_manager.h"
#include "core/item.h"
#include "platform/platform_factory.h"
#include "utils/config_manager.h"

// ============================================================
// 构造与析构
// ============================================================

DragManager::DragManager(ConfigManager* config, ClipboardManager* clipboardManager, QObject* parent)
    : QObject(parent)
{
    if (config != nullptr) {
        m_config = config;
    } else {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    }

    m_clipboardManager = clipboardManager;
    m_platform = PlatformFactory::platform();
}

DragManager::~DragManager() = default;

// ============================================================
// 拖拽操作
// ============================================================

bool DragManager::startDrag(QWidget* widget, const Item& item)
{
    m_currentItem = const_cast<Item*>(&item);

    // 路径 A：Qt QDrag 原生拖拽
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(item.content);

    QDrag* drag = new QDrag(widget);
    drag->setMimeData(mimeData);
    drag->setHotSpot(widget->rect().center());

    const Qt::DropAction result = drag->exec(Qt::CopyAction);

    if (result == Qt::CopyAction) {
        return true;
    }

    // 路径 A 失败时自动切换到路径 B
    if (result != Qt::CopyAction) {
        fallbackToPlanB(item);
    }

    return false;
}

bool DragManager::copyToClipboardAndPaste(const Item& item)
{
    const QPoint pos = m_platform->getCursorPosition();
    const WId hwnd = m_platform->getWindowAtPosition(pos.x(), pos.y());

    if (hwnd) {
        m_platform->setForegroundWindow(hwnd);
        QThread::msleep(100);
    }

    setClipboardText(item.content);
    QThread::msleep(50);
    m_platform->simulateCtrlV();
    return true;
}

bool DragManager::copyOnly(const Item& item)
{
    setClipboardText(item.content);
    qDebug() << QString("已复制到剪贴板: %1").arg(item.content.left(30));
    return true;
}

bool DragManager::copyText(const QString& text)
{
    setClipboardText(text);
    qDebug() << QString("已复制文本到剪贴板: %1").arg(text.left(30));
    return true;
}

// ============================================================
// 私有方法
// ============================================================

void DragManager::fallbackToPlanB(const Item& item)
{
    // 路径 B：剪贴板写入 + 模拟鼠标点击 + 模拟 Ctrl+V
    const QPoint pos = m_platform->getCursorPosition();
    const WId hwnd = m_platform->getWindowAtPosition(pos.x(), pos.y());

    if (hwnd) {
        m_platform->setForegroundWindow(hwnd);
        QThread::msleep(100);
    }

    m_platform->simulateMouseClick(pos.x(), pos.y());
    QThread::msleep(100);

    setClipboardText(item.content);
    QThread::msleep(50);

    m_platform->simulateCtrlV();
}

void DragManager::setClipboardText(const QString& text)
{
    // 通过 ClipboardManager 写入，保持 last_text 同步，避免误触发列表刷新
    if (m_clipboardManager != nullptr) {
        m_clipboardManager->setText(text);
    } else {
        QApplication::clipboard()->setText(text);
    }
}

#include "clipboard_manager.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QMimeData>

#include "platform/platform_factory.h"
#include "utils/config_manager.h"

// ============================================================
// 构造与析构
// ============================================================

ClipboardManager::ClipboardManager(ConfigManager* config, QObject* parent)
    : QObject(parent)
{
    if (config != nullptr) {
        m_config = config;
    } else {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    }

    m_clipboardConfig = m_config->getClipboardConfig();
    m_platform = PlatformFactory::platform();

    // 创建监控定时器（默认不启动）
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &ClipboardManager::checkClipboard);
}

ClipboardManager::~ClipboardManager() = default;

// ============================================================
// 监控控制
// ============================================================

void ClipboardManager::startMonitoring()
{
    const int interval = m_clipboardConfig.value("monitor_interval").toInt(500);
    m_monitorTimer->start(interval);
    qInfo() << QString("剪贴板监控已启动，间隔 %1ms").arg(interval);
}

void ClipboardManager::stopMonitoring()
{
    m_monitorTimer->stop();
    qDebug() << "剪贴板监控已停止";
}

void ClipboardManager::setMonitorInterval(int interval)
{
    m_monitorTimer->stop();
    m_monitorTimer->start(interval);
    qDebug() << QString("监控间隔已更新: %1ms").arg(interval);
}

void ClipboardManager::stop()
{
    stopMonitoring();
    qInfo() << "ClipboardManager 已停止";
}

// ============================================================
// 剪贴板检查
// ============================================================

void ClipboardManager::checkClipboard()
{
    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const int debounceDelay = m_clipboardConfig.value("debounce_delay").toInt(300);

    // 防抖：两次检查间隔小于防抖时间则跳过
    if (currentTime - m_lastCheckTime < debounceDelay) {
        return;
    }

    // 跳过非文本内容（文件、图片等）
    if (isNonTextContent()) {
        return;
    }

    const QString text = getText();
    if (!text.isEmpty() && text != m_lastText) {
        m_lastText = text;
        qDebug() << QString("剪贴板内容变化，长度: %1").arg(text.length());
        emit clipboardChanged(text);
    }

    m_lastCheckTime = currentTime;
}

bool ClipboardManager::isNonTextContent()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mime = clipboard->mimeData();
    if (mime == nullptr) {
        return false;
    }
    // 使用平台抽象层检查文件复制格式
    if (m_platform->isFileCopyContent(mime)) {
        return true;
    }
    // 图片数据
    if (mime->hasImage()) {
        return true;
    }
    return false;
}

// ============================================================
// 剪贴板读写
// ============================================================

QString ClipboardManager::getText()
{
    return QApplication::clipboard()->text();
}

bool ClipboardManager::setText(const QString& text)
{
    QApplication::clipboard()->setText(text);
    m_lastText = text;
    qDebug() << QString("剪贴板文本已更新，长度: %1").arg(text.length());
    return true;
}

bool ClipboardManager::isTextAvailable()
{
    const QString text = getText();
    return !text.trimmed().isEmpty();
}

QStringList ClipboardManager::getHistory()
{
    // 历史记录功能暂未实现，保留接口返回空列表
    return {};
}

void ClipboardManager::clear()
{
    QApplication::clipboard()->clear();
    m_lastText.clear();
    qDebug() << "剪贴板已清空";
}

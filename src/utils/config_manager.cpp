#include "config_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>

namespace {

/**
 * @brief 构建默认配置
 */
QJsonObject buildDefaultConfig()
{
    QJsonObject window;
    window["width"] = 320;
    window["height"] = 480;
    window["x"] = 100;
    window["y"] = 100;
    window["always_on_top"] = true;
    window["auto_popup"] = true;
    window["auto_popup_min_items"] = 3; // 解析出的非常驻条目数小于等于该值时，不自动弹出窗口

    QJsonObject clipboard;
    clipboard["monitor_interval"] = 500;
    clipboard["debounce_delay"] = 300;

    QJsonObject parsing;
    parsing["split_mode"] = "smart";
    parsing["single_column_delimiter"] = "\\n";
    parsing["single_row_delimiter"] = "\\t";
    parsing["strip_whitespace"] = true;
    parsing["remove_empty_lines"] = true;
    parsing["remove_duplicates"] = false;
    parsing["enable_split_limits"] = true;
    parsing["max_split_count"] = 10;
    parsing["max_item_length"] = 100;

    QJsonObject ui;
    ui["item_height"] = 36;
    ui["font_size"] = 10;
    ui["theme"] = "light";
    ui["opacity"] = 100;                        // 窗口不透明度百分比（30~100，100 为完全不透明）
    ui["mark_used_after_double_click"] = true;

    QJsonObject shortcuts;
    shortcuts["toggle_always_on_top"] = "Ctrl+Shift+T";
    shortcuts["toggle_window"] = "Ctrl+Shift+M";
    shortcuts["clear_all"] = "Ctrl+Shift+X";
    shortcuts["copy_all"] = "Ctrl+Shift+C";
    shortcuts["paste_plain"] = "Ctrl+Shift+V";

    QJsonObject persistent;
    persistent["items"] = QJsonArray();

    QJsonObject root;
    root["window"] = window;
    root["clipboard"] = clipboard;
    root["parsing"] = parsing;
    root["ui"] = ui;
    root["shortcuts"] = shortcuts;
    root["persistent"] = persistent;
    return root;
}

/**
 * @brief 获取默认配置（静态局部变量，仅构建一次）
 */
const QJsonObject& defaultConfig()
{
    static const QJsonObject config = buildDefaultConfig();
    return config;
}

/**
 * @brief 递归合并目标与源配置（源覆盖目标）
 */
void mergeDict(QJsonObject& target, const QJsonObject& source)
{
    for (auto it = source.begin(); it != source.end(); ++it) {
        if (target.contains(it.key()) && target.value(it.key()).isObject() && it.value().isObject()) {
            QJsonObject sub = target.value(it.key()).toObject();
            mergeDict(sub, it.value().toObject());
            target[it.key()] = sub;
        } else {
            target[it.key()] = it.value();
        }
    }
}

/**
 * @brief 递归设置点分路径上的配置值
 */
void setNested(QJsonObject& obj, const QStringList& keys, int index, const QJsonValue& value)
{
    if (index == keys.size() - 1) {
        obj[keys[index]] = value;
        return;
    }
    QJsonObject child = obj.value(keys[index]).toObject();
    setNested(child, keys, index + 1, value);
    obj[keys[index]] = child;
}

} // namespace

// ============================================================
// 构造与加载
// ============================================================

ConfigManager::ConfigManager(const QString& configPath)
{
    if (configPath.isEmpty()) {
        // 默认使用用户配置目录（Windows: %APPDATA%/MultiClipboard，Linux: ~/.config/MultiClipboard），
        // 避免写入程序所在目录（可能为只读的安装目录）导致配置保存失败
        const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        m_configPath = baseDir + QStringLiteral("/MultiClipboard/config.json");
    } else {
        m_configPath = configPath;
    }
    // 确保配置目录存在，避免首次保存失败
    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    m_config = loadConfig();
}

QJsonObject ConfigManager::loadConfig()
{
    QFile file(m_configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        file.close();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            return mergeConfig(doc.object());
        }
        return defaultConfig();
    }
    return defaultConfig();
}

QJsonObject ConfigManager::mergeConfig(const QJsonObject& loaded) const
{
    QJsonObject merged = defaultConfig();
    mergeDict(merged, loaded);
    return merged;
}

// ============================================================
// 读写配置
// ============================================================

void ConfigManager::saveConfig()
{
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QJsonDocument doc(m_config);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        if (file.error() != QFileDevice::NoError) {
            qWarning() << QStringLiteral("配置写入失败: %1，错误: %2")
                              .arg(m_configPath, file.errorString());
        }
    } else {
        // 写入失败（如沙箱/只读目录限制）时记录日志，方便定位配置无法持久化的问题
        qWarning() << QStringLiteral("配置保存失败，无法打开配置文件: %1，错误: %2")
                          .arg(m_configPath, file.errorString());
    }
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue) const
{
    const QStringList keys = key.split('.');
    QJsonValue value = m_config;
    for (const QString& k : keys) {
        if (value.isObject() && value.toObject().contains(k)) {
            value = value.toObject().value(k);
        } else {
            return defaultValue;
        }
    }
    return value.toVariant();
}

void ConfigManager::set(const QString& key, const QVariant& value)
{
    const QStringList keys = key.split('.');
    if (keys.isEmpty()) {
        return;
    }
    QJsonObject root = m_config;
    setNested(root, keys, 0, QJsonValue::fromVariant(value));
    m_config = root;
}

// ============================================================
// 分区配置
// ============================================================

QJsonObject ConfigManager::getWindowConfig() const
{
    return m_config.value("window").toObject(defaultConfig().value("window").toObject());
}

QJsonObject ConfigManager::getClipboardConfig() const
{
    return m_config.value("clipboard").toObject(defaultConfig().value("clipboard").toObject());
}

QJsonObject ConfigManager::getParsingConfig() const
{
    return m_config.value("parsing").toObject(defaultConfig().value("parsing").toObject());
}

QJsonObject ConfigManager::getUiConfig() const
{
    return m_config.value("ui").toObject(defaultConfig().value("ui").toObject());
}

QJsonObject ConfigManager::getShortcutConfig() const
{
    return m_config.value("shortcuts").toObject(defaultConfig().value("shortcuts").toObject());
}

// ============================================================
// 窗口配置
// ============================================================

void ConfigManager::updateWindowPosition(int x, int y)
{
    QJsonObject window = getWindowConfig();
    window["x"] = x;
    window["y"] = y;
    m_config["window"] = window;
    saveConfig();
}

void ConfigManager::updateWindowSize(int width, int height)
{
    QJsonObject window = getWindowConfig();
    window["width"] = width;
    window["height"] = height;
    m_config["window"] = window;
    saveConfig();
}

bool ConfigManager::toggleAlwaysOnTop()
{
    QJsonObject window = getWindowConfig();
    const bool current = window.value("always_on_top").toBool(true);
    window["always_on_top"] = !current;
    m_config["window"] = window;
    saveConfig();
    return !current;
}

// ============================================================
// 持久化条目
// ============================================================

QJsonArray ConfigManager::getPersistentItems() const
{
    return m_config.value("persistent").toObject().value("items").toArray();
}

void ConfigManager::savePersistentItems(const QJsonArray& items)
{
    QJsonObject persistent = m_config.value("persistent").toObject();
    persistent["items"] = items;
    m_config["persistent"] = persistent;
    saveConfig();
}

void ConfigManager::addPersistentItem(const QString& content)
{
    const QJsonArray items = getPersistentItems();
    for (const QJsonValue& value : items) {
        if (value.toObject().value("content").toString() == content) {
            return; // 已存在，忽略
        }
    }
    QJsonArray newItems = items;
    QJsonObject item;
    item["content"] = content;
    newItems.append(item);
    savePersistentItems(newItems);
}

void ConfigManager::removePersistentItem(const QString& content)
{
    const QJsonArray items = getPersistentItems();
    QJsonArray filtered;
    for (const QJsonValue& value : items) {
        if (value.toObject().value("content").toString() != content) {
            filtered.append(value);
        }
    }
    savePersistentItems(filtered);
}

void ConfigManager::setPersistentItemNote(const QString& content, const QString& note)
{
    const QJsonArray items = getPersistentItems();
    QJsonArray newItems;
    bool found = false;
    for (const QJsonValue& value : items) {
        QJsonObject item = value.toObject();
        if (item.value("content").toString() == content) {
            if (note.isEmpty()) {
                item.remove("note"); // 空备注移除备注键
            } else {
                item["note"] = note;
            }
            found = true;
        }
        newItems.append(item);
    }
    if (found) {
        savePersistentItems(newItems);
    }
}

#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariant>

/**
 * @brief 配置管理器
 *
 * 负责应用配置的加载、保存和管理。
 * 内部使用 QJsonObject 保存配置，支持点分隔键访问和默认配置合并。
 */
class ConfigManager
{
public:
    /**
     * @brief 构造函数
     * @param configPath 配置文件路径，为空时使用用户配置目录下的 config.json（Windows: %APPDATA%/MultiClipboard）
     */
    explicit ConfigManager(const QString& configPath = QString());

    /**
     * @brief 获取配置值
     * @param key 配置键，支持点分隔（如 "window.width"）
     * @param defaultValue 默认值
     * @return 配置值
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief 设置配置值
     * @param key 配置键，支持点分隔（如 "window.width"）
     * @param value 配置值
     */
    void set(const QString& key, const QVariant& value);

    /**
     * @brief 保存配置到文件（UTF-8 编码，缩进 2 空格）
     */
    void saveConfig();

    /**
     * @brief 获取窗口配置
     */
    QJsonObject getWindowConfig() const;

    /**
     * @brief 获取剪贴板配置
     */
    QJsonObject getClipboardConfig() const;

    /**
     * @brief 获取解析配置
     */
    QJsonObject getParsingConfig() const;

    /**
     * @brief 获取界面配置
     */
    QJsonObject getUiConfig() const;

    /**
     * @brief 获取快捷键配置
     */
    QJsonObject getShortcutConfig() const;

    /**
     * @brief 更新窗口位置配置并保存
     * @param x 窗口 X 坐标
     * @param y 窗口 Y 坐标
     */
    void updateWindowPosition(int x, int y);

    /**
     * @brief 更新窗口大小配置并保存
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    void updateWindowSize(int width, int height);

    /**
     * @brief 切换窗口置顶状态并保存
     * @return 新的置顶状态
     */
    bool toggleAlwaysOnTop();

    /**
     * @brief 获取持久化条目列表
     * @return 持久化条目列表（[{content: str}, ...]）
     */
    QJsonArray getPersistentItems() const;

    /**
     * @brief 保存持久化条目列表
     * @param items 持久化条目列表
     */
    void savePersistentItems(const QJsonArray& items);

    /**
     * @brief 添加持久化条目（内容已存在时忽略）
     * @param content 条目内容
     */
    void addPersistentItem(const QString& content);

    /**
     * @brief 移除持久化条目
     * @param content 条目内容
     */
    void removePersistentItem(const QString& content);

    /**
     * @brief 设置持久化条目的备注（内容不存在时忽略；空备注移除备注键）
     * @param content 条目内容
     * @param note 备注文本
     */
    void setPersistentItemNote(const QString& content, const QString& note);

    /**
     * @brief 获取配置文件路径
     */
    QString configPath() const { return m_configPath; }

private:
    /**
     * @brief 加载配置文件并合并默认配置
     * @return 合并后的配置对象
     */
    QJsonObject loadConfig();

    /**
     * @brief 递归合并加载的配置与默认配置
     * @param loaded 加载的配置
     * @return 合并后的配置对象
     */
    QJsonObject mergeConfig(const QJsonObject& loaded) const;

    QString m_configPath;       /**< 配置文件路径 */
    QJsonObject m_config;       /**< 当前配置对象 */
};

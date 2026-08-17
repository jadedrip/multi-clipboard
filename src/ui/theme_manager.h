#pragma once

#include <QString>
#include <QHash>

/**
 * @brief 主题颜色集合
 *
 * 使用键值对表示主题颜色变量，键名统一维护，便于对照修改。
 */
using ThemeColors = QHash<QString, QString>;

/**
 * @brief 主题管理器
 *
 * 负责管理应用的亮色和暗色主题样式，所有颜色变量统一在此定义。
 */
namespace ThemeManager {

/**
 * @brief 获取主界面主题颜色
 * @param themeName 主题名称，"dark" 或 "light"
 * @return 主题颜色键值对
 */
ThemeColors getTheme(const QString& themeName);

/**
 * @brief 获取配置窗口主题颜色
 * @param themeName 主题名称，"dark" 或 "light"
 * @return 主题颜色键值对
 */
ThemeColors getConfigTheme(const QString& themeName);

} // namespace ThemeManager

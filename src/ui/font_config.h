#pragma once

#include <QString>
#include <QStringList>

class QApplication;

/**
 * @brief 跨平台字体配置
 *
 * 提供跨平台的字体配置，确保中文在不同操作系统下正确显示。
 */
namespace FontConfig {

/**
 * @brief 获取当前平台可用的中文字体族列表（按优先级排序）
 * @return 字体族名称列表
 */
QStringList getChineseFontFamilies();

/**
 * @brief 获取适用于 QSS 的 font-family 字符串（含回退链）
 * @return CSS font-family 值
 */
QString getCssFontFamily();

/**
 * @brief 获取默认字体大小
 * @return 字体大小（像素）
 */
int getDefaultFontSize();

/**
 * @brief 设置应用的默认字体（选择第一个可用的中文字体）
 * @param app QApplication 实例
 */
void setupApplicationFont(QApplication* app);

} // namespace FontConfig

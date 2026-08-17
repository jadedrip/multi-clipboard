#include "font_config.h"

#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>

#include <utility>

// ============================================================
// 字体族
// ============================================================

QStringList FontConfig::getChineseFontFamilies()
{
#ifdef Q_OS_WIN
    // Windows 平台常见中文字体
    return {
        QStringLiteral("Microsoft YaHei"),      // 微软雅黑（优先）
        QStringLiteral("微软雅黑"),
        QStringLiteral("SimSun"),               // 宋体
        QStringLiteral("宋体"),
        QStringLiteral("SimHei"),               // 黑体
        QStringLiteral("黑体"),
        QStringLiteral("Microsoft JhengHei"),   // 微软正黑体
        QStringLiteral("FangSong"),             // 仿宋
        QStringLiteral("KaiTi")                 // 楷体
    };
#elif defined(Q_OS_MAC)
    // macOS 平台常见中文字体
    return {
        QStringLiteral("PingFang SC"),          // 苹方（简体）
        QStringLiteral("Hiragino Sans GB"),     // 冬青黑体
        QStringLiteral("STHeiti"),              // 华文黑体
        QStringLiteral("Songti SC"),            // 宋体
        QStringLiteral("Arial Unicode MS")
    };
#else
    // Linux 平台常见中文字体
    return {
        QStringLiteral("Noto Sans CJK SC"),     // Noto 思源黑体（简体）
        QStringLiteral("Noto Sans CJK"),
        QStringLiteral("WenQuanYi Micro Hei"),  // 文泉驿微米黑
        QStringLiteral("WenQuanYi Zen Hei"),    // 文泉驿正黑
        QStringLiteral("Source Han Sans SC"),   // 思源黑体
        QStringLiteral("Source Han Sans CN"),
        QStringLiteral("AR PL UMing CN"),       // 文鼎 PL 宋体
        QStringLiteral("DejaVu Sans")           // 回退字体
    };
#endif
}

QString FontConfig::getCssFontFamily()
{
    QStringList families = getChineseFontFamilies();
    // 添加通用回退
    families << QStringLiteral("sans-serif") << QStringLiteral("Serif");

    // 为包含空格的字体添加引号
    QStringList quoted;
    for (const QString& family : std::as_const(families)) {
        if (family.contains(' ')) {
            quoted << QString("\"%1\"").arg(family);
        } else {
            quoted << family;
        }
    }
    return quoted.join(", ");
}

int FontConfig::getDefaultFontSize()
{
#ifdef Q_OS_WIN
    return 10;  // Windows 下使用较小的字体
#elif defined(Q_OS_MAC)
    return 11;  // macOS 下字体渲染更好
#else
    return 10;  // Linux 下使用较小的字体
#endif
}

void FontConfig::setupApplicationFont(QApplication* app)
{
    const QStringList families = getChineseFontFamilies();

    // 获取系统可用字体列表
    const QStringList availableFamilies = QFontDatabase::families();

    // 查找第一个可用的中文字体
    QString selectedFamily;
    for (const QString& family : families) {
        if (availableFamilies.contains(family)) {
            selectedFamily = family;
            qInfo() << QString("选择中文字体: %1").arg(family);
            break;
        }
    }

    QFont font;
    if (!selectedFamily.isEmpty()) {
        font = QFont(selectedFamily, getDefaultFontSize());
    } else {
        // 未找到中文字体，使用系统默认字体
        qWarning() << "未找到中文字体，使用系统默认字体";
        font.setPointSize(getDefaultFontSize());
    }

    app->setFont(font);
    qInfo() << QString("应用字体设置完成: family=%1, size=%2").arg(font.family()).arg(font.pointSize());
}

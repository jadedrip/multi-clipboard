#include <QtTest/QtTest>

#include "core/content_parser.h"
#include "utils/config_manager.h"

/**
 * @brief 内容解析器单元测试
 *
 * 覆盖内容解析器的全部核心用例。
 * 注意：未分隔的原始条目（raw）位于列表首位。
 */
class TestContentParser : public QObject
{
    Q_OBJECT

private slots:
    // ==================== 智能模式 ====================

    void testParseSmartModeWithTabs()
    {
        // 智能模式解析带制表符的文本（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\tc\nd\te\tf");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 7);
        // 原始条目位于首位
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        // 分割条目
        QCOMPARE(items[1].content, QStringLiteral("a"));
        QCOMPARE(items[2].content, QStringLiteral("b"));
        QCOMPARE(items[3].content, QStringLiteral("c"));
        QCOMPARE(items[4].content, QStringLiteral("d"));
    }

    void testParseSmartModeWithoutTabs()
    {
        // 智能模式解析不带制表符的文本（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\nline2\nline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    // ==================== 行尾兼容性 ====================

    void testParseWindowsLineEndings()
    {
        // Windows \r\n 换行：原始条目应规范化行尾，条目内容不残留 \r
        ContentParser parser;
        const QString text = QStringLiteral("line1\r\nline2\r\nline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        // 原始条目行尾已规范化为 \n
        QCOMPARE(items[0].content, QStringLiteral("line1\nline2\nline3"));
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    void testParseOldMacLineEndings()
    {
        // 旧 Mac \r 换行：同样按 \n 切分
        ContentParser parser;
        const QString text = QStringLiteral("line1\rline2\rline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QCOMPARE(items[0].content, QStringLiteral("line1\nline2\nline3"));
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    void testParseExcelWindowsLineEndings()
    {
        // Excel 复制的 Windows 文本：\r\n 分行、\t 分列，内容不残留 \r
        ContentParser parser;
        const QString text = QStringLiteral("姓名\t年龄\r\n张三\t25\r\n李四\t30");

        const QVector<Item> items = parser.parseFromExcel(text);

        QCOMPARE(items.size(), 6);
        QCOMPARE(items[0].content, QStringLiteral("姓名"));
        QCOMPARE(items[1].content, QStringLiteral("张三"));
        QCOMPARE(items[2].content, QStringLiteral("李四"));
        QCOMPARE(items[3].content, QStringLiteral("年龄"));
        QCOMPARE(items[4].content, QStringLiteral("25"));
        QCOMPARE(items[5].content, QStringLiteral("30"));
    }

    // ==================== 指定切分模式 ====================

    void testParseSingleColumnMode()
    {
        // 单列模式解析（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\nline2\nline3");

        const QVector<Item> items = parser.parse(text, SplitMode::SingleColumn);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
    }

    void testParseSingleRowMode()
    {
        // 单行模式解析（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\tc");

        const QVector<Item> items = parser.parse(text, SplitMode::SingleRow);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        QCOMPARE(items[1].content, QStringLiteral("a"));
        QCOMPARE(items[2].content, QStringLiteral("b"));
        QCOMPARE(items[3].content, QStringLiteral("c"));
    }

    // ==================== 后处理 ====================

    void testStripWhitespace()
    {
        // 去除空白字符
        ContentParser parser;
        const QString text = QStringLiteral("  line1  \n  line2  ");

        const QVector<Item> items = parser.parse(text);

        // 原始条目 + 2 个分割条目
        QCOMPARE(items.size(), 3);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
    }

    void testRemoveEmptyLines()
    {
        // 移除空行（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\n\nline2\n\n\nline3");

        const QVector<Item> items = parser.parse(text);

        // 原始条目 + 3 个分割条目
        QCOMPARE(items.size(), 4);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    // ==================== 边界情况 ====================

    void testParseEmptyText()
    {
        // 解析空文本
        ContentParser parser;
        const QVector<Item> items = parser.parse(QString());

        QCOMPARE(items.size(), 0);
    }

    // ==================== Excel 格式 ====================

    void testParseExcelFormat()
    {
        // 解析 Excel 格式文本（按列优先顺序输出）
        ContentParser parser;
        const QString text = QStringLiteral("姓名\t年龄\t城市\n张三\t25\t北京\n李四\t30\t上海");

        const QVector<Item> items = parser.parseFromExcel(text);

        QCOMPARE(items.size(), 9);
        QCOMPARE(items[0].content, QStringLiteral("姓名"));
        QCOMPARE(items[1].content, QStringLiteral("张三"));
        QCOMPARE(items[2].content, QStringLiteral("李四"));
        QCOMPARE(items[3].content, QStringLiteral("年龄"));
        QCOMPARE(items[4].content, QStringLiteral("25"));
        QCOMPARE(items[5].content, QStringLiteral("30"));
    }
};

QTEST_MAIN(TestContentParser)
#include "test_content_parser.moc"

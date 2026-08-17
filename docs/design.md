# 多元剪贴板 C++20 设计文档

## 1. 目标

设计并实现「多元剪贴板」原生应用，基于 **C++20 + Qt6**，提供以下功能：

- 智能内容切分（smart / single_column / single_row / Excel 格式）
- 条目拖拽复制（QDrag 原生 + 剪贴板+模拟按键回退双路径）
- 实时搜索过滤（匹配高亮、不匹配淡化）
- 明暗主题切换
- 全局快捷键（RegisterHotKey + 原生事件过滤器）
- 系统托盘常驻
- 持久化条目（复选框固定）
- 纯文本粘贴
- 单实例运行检测
- 日志系统（文件轮转 + 错误日志 + 崩溃报告）

## 2. 技术选型

| 项目 | 选择 | 说明 |
| :--- | :--- | :--- |
| 语言标准 | C++20 | 用户指定 |
| GUI 框架 | Qt 6 (qtbase) | 用户规则要求 Qt6 |
| 构建系统 | xmake | 用户规则要求 C++ 工程优先使用 xmake |
| 依赖管理 | vcpkg | qtbase 通过 vcpkg 安装，路径取自环境变量 `vcpkg_root` |
| 平台 | Windows 10/11 为主，Linux 适配保留接口 | 开发环境为 Windows |

## 3. 目录结构

```
├── xmake.lua                     # 构建脚本
├── docs/
│   └── design.md                 # 本设计文档
├── src/
│   ├── main.cpp                  # 主程序入口（单实例检测、日志、App 初始化）
│   ├── core/                     # 核心模块
│   │   ├── item.h                # 条目数据结构
│   │   ├── content_parser.h      # 内容解析器（接口声明）
│   │   ├── content_parser.cpp    # 内容解析器实现
│   │   ├── clipboard_manager.h   # 剪贴板管理器（接口声明）
│   │   ├── clipboard_manager.cpp # 剪贴板管理器实现
│   │   ├── drag_manager.h        # 拖拽管理器（接口声明）
│   │   └── drag_manager.cpp      # 拖拽管理器实现
│   ├── platform/                 # 平台抽象层
│   │   ├── platform_interface.h  # 平台抽象接口（纯虚类）
│   │   ├── platform_factory.h    # 平台工厂（接口声明）
│   │   ├── platform_factory.cpp  # 平台工厂实现
│   │   ├── windows_platform.h    # Windows 平台实现（接口声明）
│   │   ├── windows_platform.cpp  # Windows 平台实现
│   │   ├── windows_hotkey_filter.h # Windows 热键事件过滤器（接口声明）
│   │   ├── windows_hotkey_filter.cpp # Windows 热键事件过滤器实现
│   │   ├── linux_platform.h      # Linux 平台实现（接口声明）
│   │   └── linux_platform.cpp    # Linux 平台实现（条件编译）
│   ├── ui/                       # UI 模块（仅负责界面与交互，业务逻辑在 core 层）
│   │   ├── main_window.h         # 主窗口（接口声明）
│   │   ├── main_window.cpp       # 主窗口实现
│   │   ├── item_widget.h         # 条目卡片组件（接口声明）
│   │   ├── item_widget.cpp       # 条目卡片组件实现
│   │   ├── elide_label.h         # 自动截断文本标签（接口声明）
│   │   ├── elide_label.cpp       # 自动截断文本标签实现
│   │   ├── config_window.h       # 配置窗口（接口声明）
│   │   ├── config_window.cpp     # 配置窗口实现
│   │   ├── hotkey_edit_widget.h  # 热键编辑控件（接口声明）
│   │   ├── hotkey_edit_widget.cpp# 热键编辑控件实现
│   │   ├── theme_manager.h       # 主题管理器（接口声明）
│   │   ├── theme_manager.cpp     # 主题管理器实现（颜色常量 + 样式生成）
│   │   ├── font_config.h         # 字体配置（接口声明）
│   │   └── font_config.cpp       # 字体配置实现
│   └── utils/                    # 工具模块
│       ├── config_manager.h      # 配置管理器（接口声明）
│       ├── config_manager.cpp    # 配置管理器实现
│       ├── hotkey_manager.h      # 全局热键管理器（接口声明）
│       ├── hotkey_manager.cpp    # 全局热键管理器实现
│       ├── logger.h              # 日志管理器（接口声明）
│       └── logger.cpp            # 日志管理器实现
├── resources/
│   ├── resources.qrc             # Qt 资源文件
│   └── icon.ico                  # 应用图标
└── test/
    └── test_content_parser.cpp   # 内容解析器单元测试（Qt Test）
```

## 4. 模块接口设计

### 4.1 Item（条目数据结构）— `core/item.h`

使用普通结构体 + 成员变量定义条目：

```cpp
struct Item {
    QString id;              // 条目唯一标识
    QString content;         // 条目内容
    int index = 0;           // 显示顺序索引
    bool used = false;       // 是否已使用
    double usedTime = 0.0;   // 使用时间戳
    int usageOrder = -1;     // 使用顺序
    bool persistent = false; // 是否持久化（复选框固定）
    bool raw = false;        // 是否为未分隔的原始条目
};
```

### 4.2 ContentParser（内容解析器）— `core/content_parser.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `enum class SplitMode { Smart, SingleColumn, SingleRow }` | 切分模式 |
| `QVector<Item> parse(const QString& text, SplitMode mode = SplitMode::Smart)` | 解析文本为条目列表（含原始条目） |
| `QVector<Item> parseFromExcel(const QString& text)` | 解析 Excel 复制文本（按列优先） |

私有方法：`parseRaw`、`parseSmart`、`parseSingleColumn`、`parseSingleRow`、`resolveDelimiter`、`applyPostProcessing`、`stripWhitespace`、`removeEmptyLines`、`removeDuplicates`、`shouldSkipSplit`。

### 4.3 ClipboardManager（剪贴板管理器）— `core/clipboard_manager.h/.cpp`

继承 `QObject`，负责监控剪贴板变化：

| 成员 | 说明 |
| :--- | :--- |
| `Q_SIGNAL void clipboardChanged(const QString& text)` | 剪贴板内容变化信号 |
| `void startMonitoring()` / `void stopMonitoring()` | 启动/停止监控 |
| `QString getText()` | 获取剪贴板文本 |
| `bool setText(const QString& text)` | 写入剪贴板文本并同步 `lastText` |
| `void clear()` | 清空剪贴板 |
| `bool isTextAvailable()` | 剪贴板是否有文本 |
| `void setMonitorInterval(int ms)` | 设置监控间隔 |
| `void stop()` | 停止监控并清理 |

私有：`QTimer* m_monitorTimer`、`QString m_lastText`、`qint64 m_lastCheckTime`、平台适配器指针。

### 4.4 DragManager（拖拽管理器）— `core/drag_manager.h/.cpp`

继承 `QObject`，负责条目拖拽与复制粘贴：

| 方法 | 说明 |
| :--- | :--- |
| `bool startDrag(QWidget* widget, const Item& item)` | 启动拖拽（路径 A），失败回退路径 B |
| `bool copyToClipboardAndPaste(const Item& item)` | 复制到剪贴板并模拟粘贴 |
| `bool copyOnly(const Item& item)` | 仅复制到剪贴板 |
| `bool copyText(const QString& text)` | 直接复制文本 |

### 4.5 平台抽象层 — `platform/`

`PlatformInterface` 为纯虚基类，定义：

```cpp
class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;
    virtual QString getPlatformName() const = 0;
    virtual bool registerHotkey(int hotkeyId, quint32 modifiers, quint32 key) = 0;
    virtual bool unregisterHotkey(int hotkeyId) = 0;
    virtual void unregisterAllHotkeys() = 0;
    virtual bool installHotkeyListener(std::function<bool(int)> callback) = 0;
    virtual void removeHotkeyListener() = 0;
    virtual QPoint getCursorPosition() = 0;
    virtual bool setCursorPosition(int x, int y) = 0;
    virtual bool simulateMouseClick(int x = -1, int y = -1) = 0;
    virtual bool simulateKeyPress(const QVector<quint32>& keyCodes) = 0;
    virtual bool simulateCtrlV() = 0;
    virtual HWND/WId getWindowAtPosition(int x, int y) = 0;
    virtual bool setForegroundWindow(WId hwnd) = 0;
    virtual bool isFileCopyContent(const QMimeData* mimeData) = 0;
    virtual void cleanup() = 0;
};
```

- `WindowsPlatform`：完整实现（RegisterHotKey、SendInput、WindowFromPoint、QAbstractNativeEventFilter 子类 `WindowsHotkeyFilter`）。
- `LinuxPlatform`：基于 X11 的实现，使用 `#ifdef Q_OS_LINUX` 条件编译，Windows 下不参与编译。
- `PlatformFactory`：单例工厂，Windows 下返回 `WindowsPlatform`，Linux 下返回 `LinuxPlatform`。

### 4.6 ConfigManager（配置管理器）— `utils/config_manager.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `explicit ConfigManager(const QString& configPath = {})` | 加载配置（默认 `%APPDATA%/MultiClipboard/config.json`，Windows） |
| `QVariant get(const QString& key, const QVariant& def = {}) const` | 点分键取值 |
| `void set(const QString& key, const QVariant& value)` | 点分键设值 |
| `void saveConfig()` | 保存到文件 |
| `QJsonObject getWindowConfig()` 等 | 各分区配置 |
| `void updateWindowPosition(int x, int y)` / `updateWindowSize(int w, int h)` | 更新窗口位置/大小 |
| `bool toggleAlwaysOnTop()` | 切换置顶并返回新状态 |
| `QJsonArray getPersistentItems()` / `addPersistentItem(content)` / `removePersistentItem(content)` | 持久化条目管理 |

实现要点：
- 内部使用 `QJsonObject` 保存配置，支持嵌套合并（默认配置 + 用户配置）。
- 默认配置内置在代码中，用户配置缺失的项自动使用默认值。

### 4.7 HotkeyManager（热键管理器）— `utils/hotkey_manager.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `void registerAllHotkeys()` | 注册所有配置快捷键 |
| `void reloadHotkeys()` | 重新加载 |
| `int registerHotkey(const QString& shortcutStr, std::function<void()> cb)` | 注册单个热键 |
| `void setCallbacks(std::function<void()> toggleAlwaysOnTop, ...)` | 设置回调 |
| `bool handleHotkey(int hotkeyId)` | 热键分发 |
| `void stop()` | 停止监听 |

`parseShortcut` 解析 `Ctrl+Shift+T` 为 (modifiers, vkCode)，内置 Windows 虚拟键码映射表。

### 4.8 Logger（日志管理器）— `utils/logger.h/.cpp`

单例（`Q_GLOBAL_STATIC`），功能：
- 文件轮转日志 `multiclipboard.log`（5MB × 5 份）
- 错误日志 `error.log`（仅 ERROR 及以上）
- 崩溃报告 `crash_YYYYMMDD_HHMMSS.log`
- 日志目录：`%APPDATA%/MultiClipboard/logs`（Windows）
- 安装 `qInstallMessageHandler` 捕获 Qt 与全局消息

### 4.9 ThemeManager（主题管理器）— `ui/theme_manager.h/.cpp`

- 定义 `ThemeColors`（QHash<QString, QString> 键值对）颜色变量
- `const ThemeColors& lightTheme()` / `darkTheme()`
- `const ThemeColors& configLightTheme()` / `configDarkTheme()`
- 提供 `getTheme(name)` / `getConfigTheme(name)`

### 4.10 FontConfig（字体配置）— `ui/font_config.h/.cpp`

- `QStringList getChineseFontFamilies()`：按平台返回中文字体列表
- `QString getCssFontFamily()`：生成 QSS `font-family` 回退链
- `int getDefaultFontSize()`
- `void setupApplicationFont(QApplication* app)`：设置应用默认字体

### 4.11 主窗口 — `ui/main_window.h/.cpp`

继承 `QMainWindow`，主要功能：
- 工具栏（搜索框，条目 > 1 时显示）
- 滚动区域条目列表（`ItemWidget` 数组）
- 状态栏（已使用计数 / 搜索结果数）
- 系统托盘（置顶切换、显示、配置、主题切换、退出）
- 剪贴板监控信号连接（多行自动弹出、单行后台更新）
- 持久化条目加载/合并/排序（persistent 优先、raw 其次、index 最后）
- 窗口自适应高度（最多 8 行）
- `closeEvent` 隐藏到托盘、保存窗口位置/大小
- 热键回调（toggleWindow / toggleAlwaysOnTop / clearAll / copyAll / pastePlain）

信号：`windowClosed`、`showRequested`。

### 4.12 条目组件 — `ui/item_widget.h/.cpp` + `ui/elide_label.h/.cpp`

`ItemWidget` 继承 `QFrame`：
- 布局：持久化复选框 + 序号标签 + 内容标签（ElideLabel）+ 状态标签
- 信号：`itemUsed(Item*)`、`itemUnused(Item*)`、`itemCopied(Item*)`、`itemPersistentChanged(Item*, bool)`
- 鼠标事件：按下记录位置、移动超过 5px 触发拖拽、双击复制、右键菜单
- 样式：normal / used / raw / flash / search_dimmed 五态 × 明暗两主题

`ElideLabel` 继承 `QLabel`：paintEvent 中用 `QFontMetrics::elidedText` 截断文本，多行只显示首行加 `...`。

### 4.13 配置窗口 — `ui/config_window.h/.cpp` + `ui/hotkey_edit_widget.h/.cpp`

`ConfigWindow` 继承 `QDialog`：
- 快捷键配置表格（5 行 2 列：功能描述 + HotkeyEditWidget）
- 窗口设置（自动弹出复选框）
- 按钮：重置默认 / 取消 / 确定
- 信号：`hotkeysChanged`
- 保存时写回 `shortcuts.*` 与 `window.auto_popup`

`HotkeyEditWidget` 继承 `QLineEdit`：只读、点击后按下组合键捕获显示，信号 `hotkeySet(QString)`。

## 5. 关键信号/槽流程

```
用户复制文本
   └─> QClipboard::dataChanged / 定时轮询
        └─> ClipboardManager::_checkClipboard（防抖、过滤文件/图片）
             └─> signal clipboardChanged(text)
                  └─> MainWindow::onClipboardChanged
                       ├─> ContentParser::parse(text) -> items
                       ├─> 合并持久化条目、查重
                       ├─> _applySearchFilter -> _displayItems（重建 ItemWidget）
                       ├─> 多行 -> 托盘弹出窗口；单行 -> 后台更新
                       └─> 状态栏更新

拖拽条目
   └─> ItemWidget::mouseMoveEvent（超过 5px 阈值）
        └─> DragManager::startDrag
             ├─> 路径 A：QDrag::exec 成功 -> setUsed(true)
             └─> 失败回退路径 B：模拟点击 + 写剪贴板 + Ctrl+V

全局热键
   └─> Windows 消息 WM_HOTKEY
        └─> WindowsHotkeyFilter::nativeEventFilter
             └─> HotkeyManager::handleHotkey(id) -> 回调
```

## 6. 构建配置（xmake.lua）

```lua
set_project("multiclipboard")
set_version("1.0.0")
set_languages("c++20")
add_defines("NOMINMAX")                       -- Windows 下避免 min/max 宏冲突
add_includedirs("src", "src/core", "src/utils", "src/ui", "src/platform")

target("multiclipboard")
    set_kind("binary")
    add_rules("qt.widgetapp")                 -- Qt Widgets 应用（自动 moc/rcc）
    add_files("src/**.cpp", "src/**.h")
    add_files("resources/resources.qrc")
    if is_plat("windows") then
        add_syslinks("user32", "shell32")
        add_files("resources/app.rc")         -- 图标与版本信息
    end

target("test_content_parser")
    set_kind("binary")
    add_rules("qt.console")
    add_files("test/test_content_parser.cpp", {rules = "qt.moc"})
    add_files("src/core/content_parser.cpp")
    add_files("src/utils/config_manager.cpp")
    add_links("Qt6Test")
```

**Qt SDK 定位**：xmake 的 `qt.widgetapp` 规则通过查找 qmake 定位 Qt SDK，有以下两种方式：

- **方式一（推荐）**：将 Qt 工具目录加入 PATH，xmake 自动从 PATH 中发现 qmake：

  ```powershell
  $env:PATH = "<vcpkg>\installed\x64-windows\tools\Qt6\bin;" + $env:PATH
  xmake f -y -m release
  ```

- **方式二（显式指定）**：`xmake f -y -m release --qt="<vcpkg>\installed\x64-windows\tools\Qt6"`

注意：xmake 检测到 qmake 后通过 `qmake -query` 获取头文件、库、工具等全部路径，因此 vcpkg 的 Qt 布局（`include/Qt6`、`tools/Qt6/bin`）可正确适配。vcpkg 的 qtbase 默认仅含 release 库，**必须使用 `-m release` 构建**。

## 7. 测试计划

使用 Qt Test 框架编写 `test/test_content_parser.cpp`，覆盖内容解析器全部用例：
- 智能模式带制表符 / 不带制表符
- 单列模式、单行模式
- 去除空白、移除空行
- 空文本
- Excel 格式（按列优先）
- 切分限制（超过 max_split_count 或 max_item_length 时不拆分）

## 8. 风险与扩展点

- **Qt 获取**：依赖 vcpkg 安装 qtbase，首次构建耗时较长。
- **Linux 平台**：Windows 环境无法验证 Linux 代码路径，Linux 实现使用条件编译，需在 Linux 环境编译验证。
- **拖拽回退路径**：路径 B（模拟点击 + Ctrl+V）依赖 Windows API。
- **扩展点**：平台抽象层可继续扩展 macOS 实现；主题颜色集中管理便于新增主题。

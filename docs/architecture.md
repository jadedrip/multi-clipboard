# 架构说明

本文档介绍多元剪贴板（C++20 / Qt6 版）的软件架构：分层结构、模块职责、关键流程与设计约定。完整设计细节见 [design.md](design.md)。

## 总体分层

```
┌─────────────────────────────────────────────────────────┐
│  main.cpp（程序入口）                                      │
│  单实例检测 → 日志初始化 → 应用初始化 → 主窗口创建 → 事件循环   │
└─────────────────────────────────────────────────────────┘
              │
┌─────────────▼──────────────┐     ┌──────────────────────────┐
│  ui/（表现层）              │     │  utils/（通用工具）         │
│  MainWindow                │     │  ConfigManager            │
│  ItemWidget / ElideLabel   │◄───►│  HotkeyManager            │
│  ConfigWindow / HotkeyEdit │ 信号 │  Logger                   │
│  ThemeManager / FontConfig │      └──────────────────────────┘
└─────────────┬──────────────┘
              │ 信号 / 槽（前后端分离）
┌─────────────▼──────────────┐
│  core/（业务层，无 UI 依赖）  │
│  ContentParser             │
│  ClipboardManager          │
│  DragManager               │
└─────────────┬──────────────┘
              │ 平台接口（虚函数）
┌─────────────▼──────────────┐
│  platform/（平台适配层）     │
│  PlatformInterface（抽象）  │
│  PlatformFactory（单例工厂）│
│  WindowsPlatform / LinuxPlatform
└────────────────────────────┘
```

**分层原则**：

- `ui/` 只负责界面与交互，不写业务逻辑；与 `core/` 通过 Qt 信号 / 槽通信
- `core/` 不依赖任何 UI 类型，可独立单元测试
- `platform/` 通过纯虚接口隔离平台差异，新增平台（如 macOS）只需实现 `PlatformInterface`
- `utils/` 为无平台依赖（或依赖 Qt）的通用能力，供各层复用

## 模块职责

### core/（核心业务）

| 模块 | 职责 |
| :--- | :--- |
| `item.h` | 条目数据结构：`id / content / index / used / usedTime / usageOrder / persistent / raw` |
| `content_parser` | 文本切分：智能 / 单列 / 单行 / Excel 四种模式，去空白、去空行、去重、长度限制 |
| `clipboard_manager` | 剪贴板监控：`QTimer` 轮询 + 300ms 防抖 + 文件 / 图片内容过滤，`setText` 同步内部状态避免误触发 |
| `drag_manager` | 拖拽与复制：`QDrag` 优先（路径 A），失败回退"写剪贴板 + 模拟点击 + Ctrl+V"（路径 B） |

### platform/（平台抽象）

| 模块 | 职责 |
| :--- | :--- |
| `platform_interface.h` | 纯虚接口：热键注册、模拟鼠标 / 键盘、取光标位置、取窗口句柄、文件复制内容判断等 |
| `platform_factory` | 单例工厂（`std::call_once`），按操作系统创建对应实现 |
| `windows_platform` | Windows 实现：`RegisterHotKey` + `QAbstractNativeEventFilter`（`WM_HOTKEY`）、`SendInput`、`WindowFromPoint`；`WindowsHotkeyFilter` 为原生事件过滤器 |
| `linux_platform` | Linux（X11/XTest）实现，`#ifdef Q_OS_LINUX` 条件编译，Windows 下为空实现 |

### utils/（工具）

| 模块 | 职责 |
| :--- | :--- |
| `config_manager` | 配置读写：`QJsonObject` 嵌套结构 + 点分键访问（`get` / `set`），默认配置递归合并 |
| `hotkey_manager` | 全局热键：快捷键字符串解析（`Ctrl+Shift+T` → 修饰键 + 虚拟键码）、注册、分发回调 |
| `logger` | 日志：`qInstallMessageHandler` 全局捕获，5MB × 5 文件轮转，`error.log` 错误日志，崩溃报告 |

### ui/（表现层）

| 模块 | 职责 |
| :--- | :--- |
| `main_window` | 主窗口：工具栏 / 搜索框、条目列表、状态栏、系统托盘、主题应用、热键回调、窗口自适应 |
| `item_widget` | 条目卡片：复选框 + 序号 + 内容（ElideLabel）+ 状态；双击复制、拖拽、右键菜单、五种视觉状态 |
| `elide_label` | 自动截断标签：`QFontMetrics::elidedText`，多行只显示首行加 `...` |
| `config_window` | 配置对话框：快捷键表格、自动弹出开关、重置 / 确定 |
| `hotkey_edit_widget` | 热键捕获控件：点击后捕获组合键并显示 |
| `theme_manager` | 主题：light / dark 两套颜色表 + QSS 生成 |
| `font_config` | 字体：按平台选择中文字体族，生成 QSS `font-family` 回退链 |

## 关键流程

### 剪贴板监控

```
复制文本
  └─> QTimer 轮询 → ClipboardManager::checkClipboard
       ├─ 防抖（300ms 间隔）
       ├─ 过滤：空文本 / 文件复制 / 图片
       └─ signal clipboardChanged(text)
            └─> MainWindow::onClipboardChanged
                 ├─ ContentParser::parse → 条目列表
                 ├─ 合并持久化条目（QSet 去重 + stable_sort）
                 ├─ 查重（isSameContent）
                 ├─ applySearchFilter → displayItems（重建卡片）
                 ├─ 多行 → 托盘弹出窗口；单行 → 后台更新
                 └─ 状态栏更新
```

### 拖拽

```
ItemWidget::mouseMoveEvent（移动超过 5px）
  └─> DragManager::startDrag
       ├─ 路径 A：QDrag::exec → 成功后标记已使用
       └─ 路径 B（A 失败）：写剪贴板 + simulateMouseClick + simulateCtrlV
```

### 全局热键

```
WM_HOTKEY 系统消息
  └─> WindowsHotkeyFilter::nativeEventFilter（QAbstractNativeEventFilter）
       └─> HotkeyManager::handleHotkey(id) → 对应回调（std::function）
            ├─ 切换窗口 / 置顶 / 清空 / 复制全部 / 纯文本粘贴
            └─ 信号或直接调用 MainWindow 公开方法
```

## 线程模型

- 单线程事件循环：所有逻辑运行在 GUI 线程，无自定义工作线程
- 剪贴板轮询使用 `QTimer`（异步），避免阻塞事件循环
- 拖拽、模拟按键等同步操作短时完成，不产生可感知卡顿

## 配置结构

`config.json` 使用嵌套 JSON，结构与内置默认配置保持一致：

```json
{
  "window": { "always_on_top": true, "auto_popup": true },
  "parsing": {
    "split_mode": "smart",
    "strip_whitespace": true,
    "remove_empty_lines": true,
    "remove_duplicates": true,
    "max_split_count": 10,
    "max_item_length": 100
  },
  "ui": { "theme": "dark", "mark_used_after_double_click": true },
  "shortcuts": {
    "toggle_window": "Ctrl+Shift+M",
    "toggle_always_on_top": "Ctrl+Shift+T",
    "clear_all": "Ctrl+Shift+X",
    "copy_all": "Ctrl+Shift+C",
    "paste_plain": "Ctrl+Shift+V"
  }
}
```

## 设计约定

- 每个类独立一个文件，文件名与类名一致
- 中文注释，Javadoc 风格（`/** */`）
- UI 与业务分离：UI 通过信号 / 槽调用核心层，禁止在 UI 中直接操作平台 API
- 平台差异通过 `PlatformInterface` 抽象，业务代码不出现平台宏
- 新功能优先在 `core/` 实现并配单元测试，再接入 UI

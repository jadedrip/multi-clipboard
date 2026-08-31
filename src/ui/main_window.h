#pragma once

#include <QJsonArray>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QVector>
#include <memory>

class ConfigManager;
class ContentParser;
class ClipboardManager;
class DragManager;
class HotkeyManager;
class ItemWidget;
class ConfigWindow;
struct Item;

class QFrame;
class QLineEdit;
class QStatusBar;
class QLabel;
class QScrollArea;
class QVBoxLayout;
class QMenu;
class QAction;
class QSlider;
class QTimer;
class QPushButton;

/**
 * @brief 主窗口
 *
 * 负责整个应用的界面布局、交互管理、系统托盘和剪贴板监控。
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  /**
   * @brief 构造函数
   * @param config 配置管理器实例
   * @param parent 父部件
   */
  explicit MainWindow(ConfigManager *config = nullptr,
                      QWidget *parent = nullptr);

  /**
   * @brief 析构函数（释放非 QObject 的成员）
   */
  ~MainWindow() override;

  /**
   * @brief 窗口关闭信号
   */
  Q_SIGNAL void windowClosed();

  /**
   * @brief 外部请求显示窗口信号（单实例检测用）
   */
  Q_SIGNAL void showRequested();

  /**
   * @brief 显示窗口（在构造函数完成后调用）
   */
  void showWindow();

  // ==================== 热键操作（供 HotkeyManager 调用） ====================

  /**
   * @brief 切换窗口显示/隐藏
   */
  void toggleWindow();

  /**
   * @brief 切换窗口置顶状态
   */
  void toggleAlwaysOnTop();

  /**
   * @brief 清空所有条目
   */
  void clearAllItems();

  /**
   * @brief 复制所有条目
   */
  void copyAllItems();

  /**
   * @brief 纯文本粘贴：获取剪贴板内容，去除格式后重新写入并粘贴
   */
  void pastePlainText();

protected:
  /**
   * @brief 关闭事件：最小化到托盘并保存窗口位置/大小
   */
  void closeEvent(QCloseEvent *event) override;

  /**
   * @brief 窗口显示事件
   */
  void showEvent(QShowEvent *event) override;

private:
  // ==================== 初始化 ====================

  /**
   * @brief 初始化窗口属性
   */
  void initProperties();

  /**
   * @brief 初始化核心组件（解析器、剪贴板、拖拽、热键管理器）
   */
  void initCoreComponents();

  /**
   * @brief 初始化界面布局
   */
  void initUi();

  /**
   * @brief 初始化工具栏（搜索框）
   * @param parentLayout 父布局
   */
  void initToolbar(QVBoxLayout *parentLayout);

  /**
   * @brief 初始化内容区域（滚动条目列表）
   * @param parentLayout 父布局
   */
  void initContentArea(QVBoxLayout *parentLayout);

  /**
   * @brief 初始化底部多行表格翻页条（◀ 第 X/Y ▶）
   * @param parentLayout 父布局
   */
  void initPagerBar(QVBoxLayout *parentLayout);

  /**
   * @brief 初始化状态栏
   */
  void initStatusBar();

  /**
   * @brief 初始化底部透明度控制条（滑动条 + 百分比标签）
   * @param parentLayout 父布局
   */
  void initOpacityControl(QVBoxLayout *parentLayout);

  /**
   * @brief 应用窗口不透明度
   * @param percent 不透明度百分比（30~100）
   */
  void applyOpacity(int percent);

  /**
   * @brief 初始化系统托盘
   */
  void initSystemTray();

  /**
   * @brief 应用窗口标志（移除最大/最小化按钮，按需置顶）
   */
  void applyWindowFlags();

  /**
   * @brief 设置搜索框可见性，同时折叠/展开工具栏
   * @param visible 是否可见
   */
  void setSearchVisible(bool visible);

  // ==================== 剪贴板事件 ====================

  /**
   * @brief 剪贴板内容变化处理：多行自动弹出，单行后台更新
   * @param text 剪贴板新文本
   */
  void onClipboardChanged(const QString &text);

  /**
   * @brief 处理多行表格内容：进入表格翻页浏览模式
   * @param tableRows 表格逐行条目
   */
  void handleTableContent(const QVector<QVector<Item>> &tableRows);

  /**
   * @brief 比较新旧条目是否内容一致
   * @param newItems 新条目列表
   * @return 是否一致
   */
  bool isSameContent(const QVector<Item> &newItems);

  /**
   * @brief 判断是否自动弹出窗口
   * @param items 解析合并后的条目列表
   * @return 是否弹出（开关开启 + 距上次关闭超过 10 秒 + 非常驻条目数超过阈值）
   */
  bool shouldAutoPopup(const QVector<Item> &items);

  /**
   * @brief 延迟启动剪贴板监控
   */
  void startClipboardMonitoring();

  // ==================== 搜索过滤 ====================

  /**
   * @brief 搜索文本变化处理：实时过滤条目列表
   * @param text 搜索文本
   */
  void onSearchChanged(const QString &text);

  /**
   * @brief 应用搜索过滤器：不隐藏条目，匹配的高亮，不匹配的淡化
   */
  void applySearchFilter();

  // ==================== 条目显示 ====================

  /**
   * @brief 显示条目列表
   */
  void displayItems();

  /**
   * @brief 条目标记为已使用处理
   * @param item 条目
   */
  void onItemUsed(Item *item);

  /**
   * @brief 条目标记为未使用处理
   * @param item 条目
   */
  void onItemUnused(Item *item);

  /**
   * @brief 条目复制处理
   * @param item 条目
   */
  void onItemCopied(Item *item);

  /**
   * @brief 翻页条上一行处理：切换当前显示的表格行
   */
  void onPrevPage();

  /**
   * @brief 翻页条下一行处理：切换当前显示的表格行
   */
  void onNextPage();

  /**
   * @brief 翻页条滑块变化处理：切换当前显示的表格行
   * @param value 表格行索引
   */
  void onPagerSliderChanged(int value);

  /**
   * @brief 进入指定表格行：显示该行切分后的条目并刷新列表
   * @param rowIndex 表格行索引（0 起）
   */
  void enterTableRow(int rowIndex);

  /**
   * @brief 比较新表格与当前表格内容是否一致（避免无谓刷新丢失翻页位置）
   * @param newRows 新表格逐行条目
   * @return 是否一致
   */
  bool isSameTableContent(const QVector<QVector<Item>> &newRows) const;

  /**
   * @brief 更新翻页条状态：按当前表格行刷新标签与显隐
   */
  void updatePagerState();

  /**
   * @brief 更新已使用计数器显示
   */
  void updateUsedCounter();

  /**
   * @brief 更新状态栏标签
   */
  void updateStatusLabel();

  // ==================== 持久化条目管理 ====================

  /**
   * @brief 加载持久化条目
   */
  void loadPersistentItems();

  /**
   * @brief 合并持久化条目到新条目列表，确保 persistent 条目放最前面
   * @param newItems 新条目列表
   * @return 合并后的条目列表
   */
  QVector<Item> mergePersistentItems(const QVector<Item> &newItems);

  /**
   * @brief 条目持久化状态变化处理
   * @param item 条目
   * @param persistent 是否持久化
   */
  void onItemPersistentChanged(Item *item, bool persistent);

  /**
   * @brief 条目备注变化处理：同步内存数据源并保存到配置
   * @param item 条目
   */
  void onItemNoteRequested(Item *item);

  /**
   * @brief 条目强制解析处理：绕过切分限制解析并替换当前列表
   * @param item 条目
   */
  void onItemForceParseRequested(Item *item);

  /**
   * @brief 条目删除处理：从内存列表删除并刷新，常驻条目同步删除配置
   * @param item 条目
   */
  void onItemDeleteRequested(Item *item);

  /**
   * @brief 删除所有已复制条目：删除所有非常驻且已复制的条目并刷新
   */
  void onDeleteCopiedRequested();

  // ==================== 配置窗口 ====================

  /**
   * @brief 显示配置窗口
   */
  void showConfigWindow();

  /**
   * @brief 热键配置变化处理
   */
  void onHotkeysChanged();

  /**
   * @brief 窗口置顶实时预览处理（不落盘）
   * @param on 是否置顶
   */
  void onAlwaysOnTopPreview(bool on);

  /**
   * @brief 配置窗口关闭后，按配置重新应用窗口置顶与透明度
   *        确定 = 应用新值，取消 = 恢复原状
   */
  void syncConfigWindowSettings();

  // ==================== 主题 ====================

  /**
   * @brief 切换明暗主题
   */
  void toggleTheme();

  /**
   * @brief 应用主题样式
   */
  void applyTheme();

  // ==================== 窗口操作 ====================

  /**
   * @brief 设置窗口置顶
   * @param onTop 是否置顶
   */
  void setAlwaysOnTop(bool onTop);

  /**
   * @brief 更新窗口标题显示置顶状态
   */
  void updateOnTopUi();

  /**
   * @brief 自动调整窗口高度适配条目数（最多 8 行）
   */
  void autoFitWindow();

  /**
   * @brief 从托盘恢复显示
   */
  void showFromTray();

  /**
   * @brief 收到外部显示请求
   */
  void onShowRequested();

  /**
   * @brief 托盘图标激活处理
   * @param reason 激活原因
   */
  void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

  /**
   * @brief 完全退出应用
   */
  void quitApplication();

  // ==================== 成员变量 ====================

  ConfigManager *m_config = nullptr; /**< 配置管理器指针 */
  std::unique_ptr<ConfigManager>
      m_ownConfig; /**< 自有的配置管理器（config 为空时使用） */

  ContentParser *m_contentParser = nullptr;       /**< 内容解析器 */
  ClipboardManager *m_clipboardManager = nullptr; /**< 剪贴板管理器 */
  DragManager *m_dragManager = nullptr;           /**< 拖拽管理器 */
  HotkeyManager *m_hotkeyManager = nullptr;       /**< 热键管理器 */

  // UI 控件
  QFrame *m_toolbar = nullptr;           /**< 工具栏容器 */
  QLineEdit *m_searchEdit = nullptr;     /**< 搜索框 */
  QScrollArea *m_scrollArea = nullptr;   /**< 滚动区域 */
  QVBoxLayout *m_itemsLayout = nullptr;  /**< 条目布局 */
  QStatusBar *m_statusBar = nullptr;     /**< 状态栏 */
  QLabel *m_statusLabel = nullptr;       /**< 状态标签 */
  QFrame *m_opacityBar = nullptr;        /**< 透明度控制条容器 */
  QSlider *m_opacitySlider = nullptr;    /**< 透明度滑动条（30~100） */
  QLabel *m_opacityValueLabel = nullptr; /**< 透明度百分比标签 */
  QTimer *m_opacitySaveTimer = nullptr;  /**< 透明度配置防抖保存定时器 */

  QFrame *m_pagerBar = nullptr;         /**< 多行表格翻页条容器 */
  QPushButton *m_prevPageBtn = nullptr; /**< 翻页条上一行按钮 */
  QPushButton *m_nextPageBtn = nullptr; /**< 翻页条下一行按钮 */
  QSlider *m_pagerSlider = nullptr;     /**< 翻页条行切换滑块（占满中间） */
  QLabel *m_pagerLabel = nullptr;       /**< 翻页条标签（第 X/Y） */

  QSystemTrayIcon *m_trayIcon = nullptr;  /**< 托盘图标 */
  QAction *m_trayPinAction = nullptr;     /**< 托盘置顶菜单项 */
  QAction *m_trayThemeAction = nullptr;   /**< 托盘主题菜单项 */
  ConfigWindow *m_configWindow = nullptr; /**< 配置窗口 */

  // 数据状态
  QVector<Item> m_items;               /**< 当前显示的条目（过滤后） */
  QVector<Item> m_allItems;            /**< 全部条目（过滤前） */
  QVector<Item> m_persistentItems;     /**< 常驻条目运行时缓存（启动加载 + 运行中勾选/取消） */
  QVector<ItemWidget *> m_itemWidgets; /**< 条目组件列表 */
  QJsonArray m_persistentItemsData;    /**< 持久化条目数据 */
  QString m_searchText;                /**< 当前搜索文本 */
  QVector<QVector<Item>> m_tableRows;  /**< 多行表格逐行条目（翻页浏览用） */
  int m_activeTableRowIndex = -1;      /**< 当前显示的表格行索引（-1 表示非表格模式） */

  // 窗口状态
  bool m_isAlwaysOnTop = true; /**< 是否置顶 */
  bool m_autoPopup = true;     /**< 剪贴板变化时自动弹出 */
  int m_autoPopupMinItems =
      3;                            /**<
                                       自动弹出最小条目数阈值（非常驻条目数小于等于该值时仅后台更新，不弹出）
                                     */
  double m_lastCloseTime = 0;       /**< 上次关闭时间戳（秒） */
  int m_usedCounter = 0;            /**< 已使用计数 */
  QString m_currentTheme = "light"; /**< 当前主题 */
  bool m_hotkeysRegistered = false; /**< 热键是否已注册 */
};

#!/bin/bash
# =============================================================
# 多元剪贴板 Linux 打包脚本
#
# 功能：使用 linuxdeploy + linuxdeploy-plugin-qt 将 release 构建
#       打包为自包含的 AppImage（无需目标机安装 Qt6）
# 用法：bash scripts/pack_linux.sh [版本号]
#       版本号缺省时自动从 xmake.lua 读取
# 产物：dist/multiclipboard-<版本>-linux-x86_64.AppImage
# 环境：需要 xmake、Qt6（qmake6）、squashfs-tools
#       工具首次运行时自动下载到 $LINUXDEPLOY_HOME（默认 ~/ldt），
#       下载失败时会给出手动下载指引
# =============================================================
set -e

# ---------- 路径定位 ----------
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")

# ---------- 1. 确定版本号 ----------
VERSION="${1:-}"
if [ -z "$VERSION" ]; then
    VERSION=$(grep -oP 'set_version\("\K[^"]+' "$PROJECT_ROOT/xmake.lua")
fi
echo "==> 打包版本: $VERSION"

# ---------- 2. 工具检查与准备 ----------
# linuxdeploy 工具解压目录（可用环境变量 LINUXDEPLOY_HOME 覆盖，默认 ~/ldt）
LINUXDEPLOY_HOME="${LINUXDEPLOY_HOME:-$HOME/ldt}"
mkdir -p "$LINUXDEPLOY_HOME"

# 解压 AppImage 需要 unsquashfs
if ! command -v unsquashfs >/dev/null 2>&1; then
    echo "==> 缺少 squashfs-tools，请先安装: sudo apt-get install -y squashfs-tools"
    exit 1
fi

# 下载并解压 linuxdeploy 相关 AppImage 工具
# 参数：$1=工具名  $2=下载地址  $3=解压目录名
download_and_extract() {
    local name="$1" url="$2" outdir="$3"
    local appimage="$LINUXDEPLOY_HOME/$name.AppImage"
    if [ ! -d "$LINUXDEPLOY_HOME/$outdir/usr/bin" ]; then
        if [ ! -f "$appimage" ]; then
            echo "==> 下载 $name ..."
            curl -L --retry 3 --retry-all-errors -o "$appimage" "$url" || {
                echo "!! 自动下载失败，请手动下载后放入: $appimage"
                echo "   下载地址: $url"
                exit 1
            }
        fi
        echo "==> 解压 $name ..."
        # 定位 AppImage 内嵌 squashfs 的偏移并解压（避免 WSL 无 FUSE 的限制）
        local offset
        offset=$(grep -abo 'hsqs' "$appimage" | tail -1 | cut -d: -f1)
        unsquashfs -offset "$offset" -d "$LINUXDEPLOY_HOME/$outdir" "$appimage" >/dev/null
        chmod +x "$LINUXDEPLOY_HOME/$outdir/usr/bin"/* 2>/dev/null || true
    fi
}

download_and_extract "linuxdeploy" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "linuxdeploy"
download_and_extract "linuxdeploy-plugin-qt" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "plugin-qt"

# ---------- 3. WSL 环境隔离 ----------
# 避免 linuxdeploy 遍历到 Windows 侧 PATH（/mnt/c/...）引发权限错误
case ":$PATH:" in
    *":/mnt/"*)
        echo "==> 检测到 WSL 环境，清理 PATH 中的 Windows 目录"
        export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
        ;;
esac

# ---------- 4. 检查 Qt6 与编译工具 ----------
if ! command -v qmake6 >/dev/null 2>&1; then
    echo "==> 缺少 qmake6，请先安装: sudo apt-get install -y qt6-base-dev"
    exit 1
fi
export QMAKE=$(command -v qmake6)

if ! command -v xmake >/dev/null 2>&1; then
    echo "==> 缺少 xmake，请参考 README 安装"
    exit 1
fi

# ---------- 5. 编译 release ----------
cd "$PROJECT_ROOT"
xmake f -y -m release
xmake -y
BINARY="$PROJECT_ROOT/build/linux/x86_64/release/multiclipboard"
if [ ! -f "$BINARY" ]; then
    echo "!! 未找到构建产物: $BINARY"
    exit 1
fi

# ---------- 6. 收集 Qt 依赖生成 AppDir ----------
# 将解压后的插件与主程序加入 PATH，供 linuxdeploy 调用
export PATH="$LINUXDEPLOY_HOME/plugin-qt/usr/bin:$LINUXDEPLOY_HOME/linuxdeploy/usr/bin:$PATH"
export LD_LIBRARY_PATH="$LINUXDEPLOY_HOME/plugin-qt/usr/lib:$LD_LIBRARY_PATH"

APPDIR="$LINUXDEPLOY_HOME/mc-AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"

echo "==> linuxdeploy 收集 Qt 依赖（生成 AppDir）..."
"$LINUXDEPLOY_HOME/linuxdeploy/AppRun" \
    --appdir "$APPDIR" \
    --executable "$BINARY" \
    --plugin qt 2>&1 | tail -5

# ---------- 7. 补充 desktop 与图标（AppImage 规范要求） ----------
mkdir -p "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
cat > "$APPDIR/usr/share/applications/multiclipboard.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=多元剪贴板
Comment=跨平台剪贴板管理工具
Exec=multiclipboard
Icon=multiclipboard
Terminal=false
Categories=Utility;
EOF
if [ -f "$PROJECT_ROOT/resources/icons/icon_256x256.png" ]; then
    cp "$PROJECT_ROOT/resources/icons/icon_256x256.png" \
        "$APPDIR/usr/share/icons/hicolor/256x256/apps/multiclipboard.png"
fi

# ---------- 8. AppImage runtime 准备 ----------
# linuxdeploy-plugin-appimage 需要 type2-runtime；提前下载缓存，避免生成时网络失败
RUNTIME_FILE="$LINUXDEPLOY_HOME/runtime-x86_64"
if [ ! -f "$RUNTIME_FILE" ]; then
    echo "==> 下载 AppImage runtime ..."
    curl -L --retry 3 --retry-all-errors -o "$RUNTIME_FILE" \
        "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64" || {
        echo "!! runtime 自动下载失败，请手动下载后放入: $RUNTIME_FILE"
        echo "   下载地址: https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64"
        exit 1
    }
fi
export LINUXDEPLOY_APPIMAGE_RUNTIME_FILE="$RUNTIME_FILE"

# ---------- 9. 生成 AppImage ----------
# 产物生成于当前工作目录（项目根），文件名取自 desktop 的 Name
echo "==> 生成 AppImage ..."
cd "$PROJECT_ROOT"
"$LINUXDEPLOY_HOME/linuxdeploy/AppRun" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/multiclipboard" \
    --plugin qt --output appimage 2>&1 | tail -3

# ---------- 10. 规范命名并输出到 dist ----------
OUTPUT="$PROJECT_ROOT/dist/multiclipboard-$VERSION-linux-x86_64.AppImage"
mkdir -p "$PROJECT_ROOT/dist"
# 查找刚生成的 AppImage（比 AppDir 更新）并移动到 dist
find "$PROJECT_ROOT" -maxdepth 1 -name '*.AppImage' -newer "$APPDIR" \
    -exec mv {} "$OUTPUT" \; 2>/dev/null || true
if [ ! -f "$OUTPUT" ]; then
    echo "!! 未找到生成的 AppImage，请检查上面输出"
    exit 1
fi

echo ""
echo "==> 打包完成: $OUTPUT"
ls -lh "$OUTPUT"

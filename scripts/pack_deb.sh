#!/bin/bash
# =============================================================
# 多元剪贴板 Linux deb 打包脚本
#
# 功能：使用 linuxdeploy + linuxdeploy-plugin-qt 收集 Qt6 运行库，
#       生成自包含的 deb 安装包（目标 UOS 20 / Debian 10，无需预装 Qt6）
# 用法：bash scripts/pack_deb.sh [版本号]
#       版本号缺省时自动从 xmake.lua 读取
# 产物：dist/linux/multiclipboard_<版本>_amd64.deb
# 环境：需要 xmake、Qt6（qmake6）、dpkg-deb、squashfs-tools
#       工具首次运行时自动下载到 $LINUXDEPLOY_HOME（默认 ~/ldt），
#       下载失败时会给出手动下载指引
#
# 包结构（自包含，库与主程序同目录 /usr/lib/multiclipboard）：
#   /usr/bin/multiclipboard           启动脚本（设置 LD_LIBRARY_PATH / QT_PLUGIN_PATH）
#   /usr/lib/multiclipboard/multiclipboard  主程序（RUNPATH=$ORIGIN）
#   /usr/lib/multiclipboard/*.so*     Qt6 及其依赖库（含 libstdc++/libgcc_s 兜底）
#   /usr/lib/multiclipboard/plugins/  Qt 插件（platforms/imageformats/...）
#   /usr/share/applications/multiclipboard.desktop
#   /usr/share/icons/hicolor/.../multiclipboard.png
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
ARCH="amd64"
PKG_NAME="multiclipboard"
echo "==> 打包版本: $VERSION (架构: $ARCH)"

# ---------- 2. 工具检查与准备 ----------
# linuxdeploy 工具解压目录（可用环境变量 LINUXDEPLOY_HOME 覆盖，默认 ~/ldt）
LINUXDEPLOY_HOME="${LINUXDEPLOY_HOME:-$HOME/ldt}"
mkdir -p "$LINUXDEPLOY_HOME"

if ! command -v unsquashfs >/dev/null 2>&1; then
    echo "==> 缺少 squashfs-tools，请先安装: sudo apt-get install -y squashfs-tools"
    exit 1
fi

# 下载并解压 linuxdeploy 相关 AppImage 工具（逻辑与 pack_linux.sh 保持一致）
# 参数：$1=工具名  $2=下载地址  $3=解压目录名
download_and_extract() {
    local name="$1" url="$2" outdir="$3"
    local appimage="$LINUXDEPLOY_HOME/$name.AppImage"
    if [ ! -d "$LINUXDEPLOY_HOME/$outdir/usr/bin" ]; then
        if [ ! -f "$appimage" ]; then
            echo "==> 下载 $name ..."
            # 注：低版本 curl（如 Debian 10 的 7.64）不支持 --retry-all-errors，故只使用 --retry
            curl -L --retry 3 -o "$appimage" "$url" || {
                echo "!! 自动下载失败，请手动下载后放入: $appimage"
                echo "   下载地址: $url"
                exit 1
            }
        fi
        echo "==> 解压 $name ..."
        # 定位 AppImage 内嵌 squashfs 的偏移，切出后再用 unsquashfs 解压
        # （避免 WSL 无 FUSE；兼容不支持 -offset 参数的低版本 squashfs-tools，如 Debian 10 的 4.3）
        local offset
        offset=$(grep -abo 'hsqs' "$appimage" | tail -1 | cut -d: -f1)
        dd if="$appimage" of="$LINUXDEPLOY_HOME/$name.fs" bs=1 skip="$offset" 2>/dev/null
        unsquashfs -d "$LINUXDEPLOY_HOME/$outdir" "$LINUXDEPLOY_HOME/$name.fs" >/dev/null
        rm -f "$LINUXDEPLOY_HOME/$name.fs"
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
        # 注意：xmake 官方脚本安装在 ~/.local/bin（XMAKE_ROOTDIR），且必须排在
        # /usr/local/bin 之前，否则 /usr/local/bin/xmake 引导器（缺主程序）被优先命中而崩溃（exit 255）
        export PATH=$HOME/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
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

# ---------- 7. 补充 desktop 与图标（复用 AppImage 相同的规范文件） ----------
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

# ---------- 8. 组装 deb 包根目录 ----------
PKGROOT="$LINUXDEPLOY_HOME/mc-deb-root"
rm -rf "$PKGROOT"
mkdir -p "$PKGROOT/DEBIAN"
mkdir -p "$PKGROOT/usr/bin"
mkdir -p "$PKGROOT/usr/lib/$PKG_NAME"
mkdir -p "$PKGROOT/usr/share/applications"
LIBDIR="$PKGROOT/usr/lib/$PKG_NAME"

# 8.1 主程序：放入库目录并修正 RUNPATH（与库同目录，使用 $ORIGIN）
BIN_INSTALL="$LIBDIR/$PKG_NAME"
cp -a "$APPDIR/usr/bin/$PKG_NAME" "$BIN_INSTALL"
"$LINUXDEPLOY_HOME/linuxdeploy/usr/bin/patchelf" --set-rpath '$ORIGIN' "$BIN_INSTALL"
echo "==> 主程序已放入 $LIBDIR 并修正 RUNPATH=\$ORIGIN"

# 8.2 Qt 及依赖库（linuxdeploy 已收集，整体拷贝）
cp -a "$APPDIR/usr/lib/." "$LIBDIR/"
# 清理 linuxdeploy 附带拷贝的文档，避免污染包
rm -rf "$LIBDIR/doc" 2>/dev/null || true

# 8.2.1 校正系统库：linuxdeploy 收集的部分系统库（如 libXau/libXtst/libXext）
#       可能是带 debug 信息的异常副本，统一替换为 Debian 10 系统的真实库
#       （系统同名库版本一致且 UOS 20 兼容；无同名则不处理，如 Qt 自带 ICU56）
SYS_LIBDIR=/usr/lib/x86_64-linux-gnu
CORRECTED=0
for f in "$LIBDIR"/*.so*; do
    [ -f "$f" ] || continue
    base=$(basename "$f")
    if [ -e "$SYS_LIBDIR/$base" ] && ! cmp -s "$SYS_LIBDIR/$base" "$f"; then
        cp -f --remove-destination "$SYS_LIBDIR/$base" "$f"
        echo "  校正库: $base"
        CORRECTED=$((CORRECTED + 1))
    fi
done
[ "$CORRECTED" -gt 0 ] && echo "==> 共校正系统库 $CORRECTED 个"

# 8.3 Qt 插件目录
cp -a "$APPDIR/usr/plugins" "$LIBDIR/plugins"
# 附带 offscreen 平台插件：headless 冒烟验证 / 无显示环境兜底
if [ -f /opt/qt-6.2.4/plugins/platforms/libqoffscreen.so ]; then
    cp -a /opt/qt-6.2.4/plugins/platforms/libqoffscreen.so "$LIBDIR/plugins/platforms/"
    echo "==> 已附带 offscreen 平台插件"
fi

# 8.4 qt.conf：以主程序所在目录为前缀查找插件
cat > "$LIBDIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
EOF

# 8.5 兜底补 libstdc++ / libgcc_s（与主程序同目录，RUNPATH 优先加载，
#     保证 UOS 自带 libstdc++ 版本略低时仍可运行）
if [ -e /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ]; then
    cp -a /usr/lib/x86_64-linux-gnu/libstdc++.so.6* "$LIBDIR/"
fi
if [ -e /usr/lib/x86_64-linux-gnu/libgcc_s.so.1 ]; then
    cp -a /usr/lib/x86_64-linux-gnu/libgcc_s.so.1 "$LIBDIR/"
fi

# 8.6 启动脚本（/usr/bin/multiclipboard -> wrapper）
cat > "$PKGROOT/usr/bin/$PKG_NAME" <<EOF
#!/bin/sh
# $PKG_NAME 启动器：指向自包含运行库目录
APP_DIR=/usr/lib/$PKG_NAME
export QT_PLUGIN_PATH="\$APP_DIR/plugins"
export LD_LIBRARY_PATH="\$APP_DIR\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
exec "\$APP_DIR/$PKG_NAME" "\$@"
EOF
chmod 0755 "$PKGROOT/usr/bin/$PKG_NAME"

# 8.7 桌面入口与图标
cp -a "$APPDIR/usr/share/applications/." "$PKGROOT/usr/share/applications/"
if [ -d "$APPDIR/usr/share/icons" ]; then
    cp -a "$APPDIR/usr/share/icons" "$PKGROOT/usr/share/"
fi
if [ -d "$APPDIR/usr/share/pixmaps" ]; then
    cp -a "$APPDIR/usr/share/pixmaps" "$PKGROOT/usr/share/"
fi

# ---------- 9. 生成 DEBIAN 控制文件 ----------
# 自包含 Qt 库；Depends 声明主程序运行时解析到系统（未打进包）的全部库，
# 均为 UOS 20 / Debian 10 桌面基础包（dpkg 安装时会自动校验补齐）
cat > "$PKGROOT/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCH
Maintainer: wangchen <wangchen@localhost>
Depends: libc6 (>= 2.28), libstdc++6, libx11-6, libxcb1, libxtst6,
 libfontconfig1, libfreetype6, libgl1, libegl1, libglvnd0,
 libexpat1, libuuid1, zlib1g
Description: 跨平台剪贴板管理工具（多元剪贴板）
 支持多行/单行剪贴板内容管理、常驻条目、备注、强制解析
 等功能的剪贴板增强工具。本包内置 Qt6 运行库，无需系统
 额外安装 Qt 环境即可运行。
EOF

# md5 校验清单（dpkg 安装校验用）
(cd "$PKGROOT" && find . -type f ! -path './DEBIAN/*' -exec md5sum {} \; | sed 's|^\./||' > DEBIAN/md5sums)

# ---------- 10. dpkg-deb 打包 ----------
OUTDIR="$PROJECT_ROOT/dist/linux"
mkdir -p "$OUTDIR"
DEB_FILE="$OUTDIR/${PKG_NAME}_${VERSION}_${ARCH}.deb"
rm -f "$DEB_FILE"
dpkg-deb --build --root-owner-group "$PKGROOT" "$DEB_FILE" 2>&1 | tail -2

if [ ! -f "$DEB_FILE" ]; then
    echo "!! dpkg-deb 打包失败"
    exit 1
fi

# ---------- 11. 校验 ----------
echo ""
echo "==> 校验: 包信息 (dpkg-deb -I)"
dpkg-deb -I "$DEB_FILE" | sed -n '1,20p'

echo ""
echo "==> 校验: 包内容清单（前 20 项）"
dpkg-deb -c "$DEB_FILE" 2>/dev/null | head -20

echo ""
echo "==> 校验: GLIBC / GLIBCXX 符号需求（目标 UOS 20 = glibc 2.28 / libstdc++ 3.4.25）"
# 判断版本 v 是否 <= 上限 cap（版本号比较，空版本视为满足）
check_le() {
    local v="$1" cap="$2"
    [ -z "$v" ] && return 0
    [ "$(printf '%s\n%s' "$v" "$cap" | sort -V | tail -1)" = "$cap" ]
}
MAX_GLIBC=0
MAX_GLIBCXX=0
while IFS= read -r f; do
    rel=${f#*"$PKGROOT/"}
    if file "$f" 2>/dev/null | grep -q ELF; then
        # 只统计"版本需求段"（.gnu.version_r），排除库自身"提供的版本"（.gnu.version_d）
        ver_needs=$(readelf -V "$f" 2>/dev/null | awk '/Version needs section/{need=1; next} /Version definition section/{need=0} need {print}')
        g=$(echo "$ver_needs" | grep -oE 'GLIBC_[0-9]+\.[0-9]+' | sort -V | tail -1)
        x=$(echo "$ver_needs" | grep -oE 'GLIBCXX_[0-9.]+' | sort -V | tail -1)
        if ! check_le "${g#GLIBC_}" "2.28"; then
            echo "!! 超 GLIBC 2.28: $rel ($g)"; MAX_GLIBC=1
        fi
        if ! check_le "${x#GLIBCXX_}" "3.4.25"; then
            echo "!! 超 GLIBCXX 3.4.25: $rel ($x)"; MAX_GLIBCXX=1
        fi
    fi
done < <(find "$PKGROOT" -type f \( -name "*.so*" -o -path "*/usr/lib/$PKG_NAME/$PKG_NAME" -o -path "*/plugins/*" \))
if [ "$MAX_GLIBC" = "0" ] && [ "$MAX_GLIBCXX" = "0" ]; then
    echo "OK: 包内 ELF 均满足 GLIBC<=2.28 / GLIBCXX<=3.4.25（兼容 UOS 20）"
fi

echo ""
echo "==> 校验: 动态链接完整性（包内库能否解析主程序全部依赖）"
smoke_bin="$PKGROOT/usr/lib/$PKG_NAME/$PKG_NAME"
MISSING=$(LD_LIBRARY_PATH="$PKGROOT/usr/lib/$PKG_NAME" ldd "$smoke_bin" 2>/dev/null | grep -c "not found" || true)
if [ "$MISSING" = "0" ]; then
    echo "OK: 主程序全部依赖可在包内解析（无 not found）"
else
    echo "!! 存在 $MISSING 个未解析依赖:"
    LD_LIBRARY_PATH="$PKGROOT/usr/lib/$PKG_NAME" ldd "$smoke_bin" 2>/dev/null | grep "not found" || true
fi

echo ""
echo "==> 校验: 冒烟测试（offscreen 平台，运行 5 秒确认可正常启动）"
rc=0
LD_LIBRARY_PATH="$PKGROOT/usr/lib/$PKG_NAME" QT_QPA_PLATFORM=offscreen \
    timeout 5 "$smoke_bin" >/dev/null 2>&1 || rc=$?
if [ "$rc" = "124" ]; then
    echo "OK: 主程序成功启动并持续运行（5 秒后由 timeout 终止）"
else
    echo "!! 冒烟测试提前退出 (exit=$rc)，错误信息："
    LD_LIBRARY_PATH="$PKGROOT/usr/lib/$PKG_NAME" QT_QPA_PLATFORM=offscreen "$smoke_bin" 2>&1 | tail -5 || true
fi

echo ""
echo "==> 打包完成: $DEB_FILE"
ls -lh "$DEB_FILE"

@echo off
echo =================================
echo      关机猫 编译脚本
echo =================================
echo.

echo [1/4] 检查编译环境...
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到 GCC 编译器
    echo    请确保已安装 MinGW-w64 并添加到 PATH
    pause
    exit /b 1
)
echo ✅ GCC 编译器: 已找到

echo.
echo [2/4] 检查图标配置...
if exist "cat_icon.ico" (
    echo ✅ 应用图标: cat_icon.ico 已找到
    set HAS_MAIN_ICON=1
) else (
    echo ⚠️  应用图标: cat_icon.ico 未找到
    set HAS_MAIN_ICON=0
)

if exist "cat_tray.ico" (
    echo ✅ 托盘图标: cat_tray.ico 已找到
    set HAS_TRAY_ICON=1
) else (
    echo ⚠️  托盘图标: cat_tray.ico 未找到
    set HAS_TRAY_ICON=0
)

echo.
echo [3/4] 编译资源文件...
windres resource.rc -o resource.o
if %errorlevel% neq 0 (
    echo ❌ 资源编译失败
    pause
    exit /b 1
)
echo ✅ 资源编译完成

echo.
echo [4/4] 编译主程序（体积优化版）...
gcc -o shutdown_cat.exe shutdown_cat.c resource.o -lcomctl32 -luxtheme -lshell32 -mwindows -municode -Os -s -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Wl,--gc-sections -fmerge-all-constants -fno-ident
if %errorlevel% neq 0 (
    echo ❌ 程序编译失败
    pause
    exit /b 1
)

echo ✅ 程序编译完成
echo.
echo =================================
echo      编译完成! ✨
echo =================================
for %%F in (shutdown_cat.exe) do echo 文件大小: %%~zF 字节
echo 输出文件: shutdown_cat.exe
echo.
pause 
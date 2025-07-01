# 关机猫 Makefile
# Shutdown Cat - Timer shutdown tool for Windows

CC = gcc
WINDRES = windres
CFLAGS = -mwindows -municode -Os -s -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Wl,--gc-sections -fmerge-all-constants -fno-ident
LIBS = -lcomctl32 -luxtheme -lshell32

# 输出文件
TARGET = shutdown_cat.exe
RESOURCE_OBJ = resource.o
SOURCE = shutdown_cat.c
RESOURCE_RC = resource.rc

# 默认目标：编译优化版本
all: $(TARGET)

# 编译优化版本（推荐）
$(TARGET): $(SOURCE) $(RESOURCE_OBJ)
	@echo "编译关机猫..."
	$(CC) -o $(TARGET) $(SOURCE) $(RESOURCE_OBJ) $(CFLAGS) $(LIBS)
	@echo "✅ 编译完成: $(TARGET)"

# 编译资源文件
$(RESOURCE_OBJ): $(RESOURCE_RC)
	@echo "编译资源文件..."
	$(WINDRES) $(RESOURCE_RC) -o $(RESOURCE_OBJ)

# 编译调试版本
debug: $(SOURCE) $(RESOURCE_OBJ)
	@echo "编译调试版本..."
	$(CC) -o shutdown_cat_debug.exe $(SOURCE) $(RESOURCE_OBJ) -mwindows -municode $(LIBS) -g
	@echo "✅ 调试版本编译完成: shutdown_cat_debug.exe"

# 清理编译文件
clean:
	@echo "清理编译文件..."
	@if exist $(TARGET) del $(TARGET)
	@if exist shutdown_cat_debug.exe del shutdown_cat_debug.exe
	@if exist $(RESOURCE_OBJ) del $(RESOURCE_OBJ)
	@echo "✅ 清理完成"

# 重新编译
rebuild: clean all

# 安装（复制到系统目录，需要管理员权限）
install: $(TARGET)
	@echo "安装关机猫到 C:\Program Files\ShutdownCat\"
	@if not exist "C:\Program Files\ShutdownCat" mkdir "C:\Program Files\ShutdownCat"
	copy $(TARGET) "C:\Program Files\ShutdownCat\"
	copy cat_icon.ico "C:\Program Files\ShutdownCat\"
	copy cat_tray.ico "C:\Program Files\ShutdownCat\"
	@echo "✅ 安装完成"

# 卸载
uninstall:
	@echo "卸载关机猫..."
	@if exist "C:\Program Files\ShutdownCat" rmdir /s /q "C:\Program Files\ShutdownCat"
	@echo "✅ 卸载完成"

# 显示帮助
help:
	@echo "关机猫 Makefile 使用说明:"
	@echo ""
	@echo "make           - 编译优化版本（推荐）"
	@echo "make debug     - 编译调试版本"
	@echo "make clean     - 清理编译文件"
	@echo "make rebuild   - 重新编译"
	@echo "make install   - 安装到系统目录（需要管理员权限）"
	@echo "make uninstall - 从系统卸载"
	@echo "make help      - 显示此帮助信息"

.PHONY: all debug clean rebuild install uninstall help 
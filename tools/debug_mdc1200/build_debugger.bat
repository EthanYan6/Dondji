@echo off
chcp 65001 >nul
echo ============================================
echo MDC1200 调试工具 - 打包脚本
echo ============================================
echo.

REM 检查 Python 是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未检测到 Python，请先安装 Python 3.8+
    pause
    exit /b 1
)

echo [1/4] 安装依赖包...
pip install numpy matplotlib scipy sounddevice pyinstaller -i https://pypi.tuna.tsinghua.edu.cn/simple

if errorlevel 1 (
    echo [错误] 依赖包安装失败
    pause
    exit /b 1
)

echo.
echo [2/4] 检查文件...
if not exist "mdc1200_debugger.py" (
    echo [错误] 找不到 mdc1200_debugger.py
    pause
    exit /b 1
)

echo.
echo [3/4] 开始打包 EXE...
REM 使用 PyInstaller 打包
pyinstaller --onefile ^
    --windowed ^
    --name "MDC1200 调试工具" ^
    --add-data "mdc1200_debugger.py;." ^
    --icon=NONE ^
    --hidden-import=numpy ^
    --hidden-import=matplotlib ^
    --hidden-import=scipy ^
    --hidden-import=sounddevice ^
    mdc1200_debugger.py

if errorlevel 1 (
    echo [错误] 打包失败
    pause
    exit /b 1
)

echo.
echo [4/4] 清理临时文件...
if exist "build" rmdir /s /q "build"
if exist "MDC1200 调试工具.spec" del "MDC1200 调试工具.spec"

echo.
echo ============================================
echo 打包完成!
echo ============================================
echo.
echo 可执行文件位置：dist\MDC1200 调试工具.exe
echo.
echo 使用方法:
echo   1. 直接双击运行 MDC1200 调试工具.exe
echo   2. 或命令行：MDC1200 调试工具.exe audio.wav
echo.
echo 功能说明:
echo   - 录制音频信号
echo   - 加载 WAV 文件分析
echo   - 频谱分析
echo   - 频偏测量
echo   - 波形显示
echo   - 生成分析报告
echo.
echo ============================================

pause

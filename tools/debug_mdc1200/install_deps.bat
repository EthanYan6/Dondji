@echo off
chcp 65001 >nul
echo.
echo ============================================
echo MDC1200 调试工具 - 依赖安装
echo ============================================
echo.

REM 检查 Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未检测到 Python
    echo.
    echo 请先安装 Python 3.8 或更高版本
    echo 下载地址：https://www.python.org/downloads/
    echo.
    echo 安装时请勾选 "Add Python to PATH"
    echo.
    pause
    exit /b 1
)

echo 检测到 Python:
python --version
echo.

echo 正在安装依赖包...
echo.
echo 使用镜像源加速：https://pypi.tuna.tsinghua.edu.cn/simple
echo.

pip install numpy matplotlib scipy sounddevice pyinstaller -i https://pypi.tuna.tsinghua.edu.cn/simple

if errorlevel 1 (
    echo.
    echo [错误] 依赖包安装失败
    echo.
    echo 请检查网络连接，或尝试手动安装:
    echo   pip install numpy
    echo   pip install matplotlib
    echo   pip install scipy
    echo   pip install sounddevice
    echo   pip install pyinstaller
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo 安装完成!
echo ============================================
echo.
echo 已安装的包:
echo   ✓ numpy      - 数值计算
echo   ✓ matplotlib - 绘图
echo   ✓ scipy      - 科学计算
echo   ✓ sounddevice - 音频录制
echo   ✓ pyinstaller - 打包 EXE
echo.
echo 下一步:
echo   1. 运行 run_debugger.bat 启动工具
echo   2. 或运行 build_debugger.bat 打包成 EXE
echo.
echo ============================================
echo.
pause

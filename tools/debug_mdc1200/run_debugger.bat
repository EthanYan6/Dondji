@echo off
chcp 65001 >nul
echo.
echo ============================================
echo MDC1200 调试工具 - 快速启动菜单
echo ============================================
echo.
echo 请选择操作:
echo.
echo [1] 打包成 EXE 可执行文件
echo [2] 直接运行 Python 调试工具 (GUI)
echo [3] 运行快速测试 (命令行)
echo [4] 查看调试指南
echo [5] 查看固件修改建议
echo [6] 检查环境 (Python 和依赖)
echo [0] 退出
echo.
set /p choice=请输入选项 (0-6): 

if "%choice%"=="1" goto BUILD
if "%choice%"=="2" goto RUN_GUI
if "%choice%"=="3" goto RUN_QUICK
if "%choice%"=="4" goto VIEW_GUIDE
if "%choice%"=="5" goto VIEW_PATCH
if "%choice%"=="6" goto CHECK_ENV
if "%choice%"=="0" goto END
goto MENU

:MENU
cls
goto MENU

:BUILD
cls
echo.
echo ============================================
echo 正在打包 EXE...
echo ============================================
echo.
call build_debugger.bat
goto END

:RUN_GUI
cls
echo.
echo ============================================
echo 启动 MDC1200 调试工具 (GUI)
echo ============================================
echo.
python mdc1200_debugger.py
if errorlevel 1 (
    echo.
    echo [错误] 启动失败，可能是 Python 或依赖未安装
    echo 请先运行选项 [6] 检查环境
    echo.
    pause
)
goto END

:RUN_QUICK
cls
echo.
echo ============================================
echo MDC1200 快速测试
echo ============================================
echo.
echo 请输入要测试的 WAV 文件路径
echo 或直接拖拽文件到此窗口
echo.
set /p wavfile=文件路径：
if exist "%wavfile%" (
    python quick_test.py "%wavfile%"
) else (
    echo [错误] 文件不存在：%wavfile%
)
goto END

:VIEW_GUIDE
cls
echo.
echo ============================================
echo MDC1200 调试指南
echo ============================================
echo.
if exist "README_MDC1200_DEBUGGER.md" (
    type README_MDC1200_DEBUGGER.md | more
) else (
    echo [错误] 找不到 README_MDC1200_DEBUGGER.md
)
goto END

:VIEW_PATCH
cls
echo.
echo ============================================
echo MDC1200 固件修改指南
echo ============================================
echo.
if exist "MDC1200_PATCH_GUIDE.md" (
    type MDC1200_PATCH_GUIDE.md | more
) else (
    echo [错误] 找不到 MDC1200_PATCH_GUIDE.md
)
goto END

:CHECK_ENV
cls
echo.
echo ============================================
echo 检查环境
echo ============================================
echo.

echo [1/3] 检查 Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未安装 Python
    echo 请先安装 Python 3.8 或更高版本
    echo 下载地址：https://www.python.org/downloads/
    goto ENV_ERROR
) else (
    python --version
    echo [✓] Python 已安装
)
echo.

echo [2/3] 检查必要依赖...
python -c "import numpy" >nul 2>&1
if errorlevel 1 (
    echo [!] numpy 未安装
) else (
    echo [✓] numpy 已安装
)

python -c "import matplotlib" >nul 2>&1
if errorlevel 1 (
    echo [!] matplotlib 未安装
) else (
    echo [✓] matplotlib 已安装
)

python -c "import scipy" >nul 2>&1
if errorlevel 1 (
    echo [!] scipy 未安装
) else (
    echo [✓] scipy 已安装
)

python -c "import sounddevice" >nul 2>&1
if errorlevel 1 (
    echo [!] sounddevice 未安装
) else (
    echo [✓] sounddevice 已安装
)
echo.

echo [3/3] 检查 PyInstaller...
python -c "import PyInstaller" >nul 2>&1
if errorlevel 1 (
    echo [!] PyInstaller 未安装
) else (
    echo [✓] PyInstaller 已安装
)
echo.

echo ============================================
echo 环境检查完成
echo ============================================
echo.
echo 如果有 [!] 标记的项目，请运行以下命令安装:
echo.
echo pip install numpy matplotlib scipy sounddevice pyinstaller
echo.
echo 或使用国内镜像:
echo pip install numpy matplotlib scipy sounddevice pyinstaller -i https://pypi.tuna.tsinghua.edu.cn/simple
echo.

goto END

:ENV_ERROR
echo.
pause
goto END

:END
echo.
echo ============================================
echo 感谢使用 MDC1200 调试工具
echo ============================================
echo.
pause

@echo off
setlocal enabledelayedexpansion

chcp 65001 >nul

:: 定义文件大小阈值（0.5MB = 524288字节）
set "maxSize=524288"

:: 创建/清空 .gitignore 文件
echo. > .gitignore

:: 递归遍历当前目录及其子目录中的所有文件
for /r %%f in (*) do (
    :: 获取文件相对于当前目录的路径
    set "filePath=%%f"
    set "relativePath=!filePath:%cd%\=!"

    :: 忽略 .git 目录及其子目录中的文件
    if /i "!relativePath:~0,4!" neq ".git" (
        :: 获取文件大小
        set "size=%%~zf"

        :: 如果文件大小超过指定阈值，将其路径写入 .gitignore
        if !size! gtr %maxSize% (
            :: 替换路径中的反斜杠为斜杠，并将路径写入 .gitignore
            set "relativePath=!relativePath:\=/!"
            echo !relativePath! >> .gitignore
        )
    )
)

echo 完成，已将超过 0.5MB 的文件路径写入 .gitignore
pause

@echo off
setlocal enabledelayedexpansion

rem 设置文件大小阈值，1MB = 1048576字节
set "threshold=524288"

rem 如果.gitignore文件不存在，创建一个新的
if not exist .gitignore (
    echo # Auto-generated .gitignore for files larger than 1MB> .gitignore
    echo. >> .gitignore
)

rem 遍历当前文件夹及其子文件夹中的所有文件
for /r %%f in (*) do (
    rem 获取文件大小
    set "size=%%~zf"
    
    rem 判断文件大小是否超过阈值
    if !size! gtr %threshold% (
        rem 获取文件的相对路径并写入.gitignore
        set "relativePath=%%f"
        set "relativePath=!relativePath:%cd%\=!"
        echo !relativePath!>> .gitignore
    )
)

echo Done! Files larger than 1MB have been added to .gitignore.
pause
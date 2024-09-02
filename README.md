# AutoOperateLib

**AutoOperateLib** 是一个功能强大、简单易用的C++库，专为Windows平台上的智能自动化操作设计。无论是键盘、鼠标的模拟操作，还是屏幕捕获、窗口管理等任务，**AutoOperateLib** 都能为您提供直观且高效的API，帮助您轻松构建复杂的自动化工具。

## 为什么选择 AutoOperateLib？

- **全面性**：涵盖从键鼠录制到窗口管理、进程操作等多种自动化功能，一站式解决您的自动化需求。
- **高性能**：利用多线程技术，确保复杂操作不会阻塞主线程，提供流畅的用户体验。
- **易集成**：简洁的API设计，使得库的集成和使用变得极其简单，降低开发者的学习曲线。
- **高度可定制**：丰富的参数设置和选项，允许用户根据实际需求进行细粒度的控制。

## 功能概览

**AutoOperateLib** 提供以下核心功能：

### 键盘和鼠标操作
- **操作录制与回放**：可以录制用户的键盘和鼠标操作并保存为文件，稍后可以轻松加载并回放。
- **模拟输入**：自动化模拟键盘按键和鼠标点击事件。

### 屏幕操作
- **屏幕截图**：支持区域截屏及全屏截图，并将图像保存或复制到剪贴板。
- **图像识别**：基于模板匹配的图像查找功能，帮助您在屏幕上定位特定图像。

### 窗口管理
- **窗口操作**：支持获取、移动、最小化、最大化窗口，甚至将窗口置顶或隐藏。
- **多窗口处理**：批量获取当前所有打开的窗口，并对其进行统一管理。

### 进程管理
- **启动与终止进程**：轻松启动、管理和终止系统进程。
- **管理员权限检测与提升**：判断程序是否以管理员模式运行，并支持重新启动以获取管理员权限。

### 文件与目录操作
- **文件管理**：支持创建、删除文件和目录，并检测其存在性。
- **INI文件解析**：读取和解析INI配置文件，支持多节内容的读取。

## 快速上手

通过以下简单步骤，您可以立即开始使用**AutoOperateLib**：
```sh
git clone https://github.com/RayShark0605/AutoOperateLib
```
打开Visual Studio编译项目即可生成**AutoOperateLib.lib**库文件。

## 使用案例
- **键鼠操作录制与回放**

以下示例展示了如何录制键盘和鼠标操作，并将其保存到文件中，随后加载并回放这些操作。
```cpp
#include "AutoOperateLib.h"

int main()
{
    // 创建操作记录器并开始录制
    AO_ActionRecorder recorder;
    recorder.StartRecording();
    WaitForS(10); // 录制10秒
    recorder.StopRecording();

    // 获取录制的操作记录并保存到文件
    auto records = recorder.GetRecords();
    SaveRecordsToTxtFile(records, "records.txt");

    // 加载并回放操作
    auto loadedRecords = LoadRecordsFromTxtFile("records.txt");
    AO_ActionSimulator simulator;
    simulator.Start(loadedRecords);
    simulator.WaitForEnd();

    return 0;
}
```
- **屏幕截图与剪贴板操作**

该示例展示了如何截取屏幕特定区域并保存到文件，同时将内容复制到剪贴板。
```cpp
#include "AutoOperateLib.h"

int main()
{
    // 定义截图区域
    AO_Rect rect(0, 0, 1920, 1080);

    // 截取屏幕并保存到文件
    CaptureScreenToFile(rect, "D:/screenshot.bmp");

    // 将截图内容复制到剪贴板
    CaptureScreenToClipboard(rect);

    return 0;
}
```
- **操作窗口**

该示例展示了如何获取所有打开的窗口，并将指定窗口置于屏幕中央。
```cpp
#include "AutoOperateLib.h"

int main()
{
    // 获取所有窗口
    auto windows = GetAllOpenWindows();

    if (!windows.empty())
    {
        // 选择第一个窗口并移动到屏幕中央
        AO_Window& window = windows[0];
        MoveAndResizeWindow(window, 480, 270, 960, 540);
        BringWindowToFront(window);
    }

    return 0;
}
```
- **启动并终止进程**

该示例展示了如何启动并终止一个进程。
```cpp
#include "AutoOperateLib.h"

int main()
{
    std::string appPath = "C:\\Program Files\\ExampleApp\\example.exe";
    
    if (StartProcess(appPath))
    {
        std::cout << "进程启动成功" << std::endl;
    }

    if (TerminateProcess("example.exe"))
    {
        std::cout << "进程终止成功" << std::endl;
    }

    return 0;
}
```

## 贡献
欢迎众多开发者为**AutoOperateLib** 贡献代码！如果您发现问题或希望添加新功能，请提交Issue或Pull Request。

## 许可证
**AutoOperateLib** 遵循 MIT 许可证。
# AutoOperateLib

**AutoOperateLib** 是一个功能强大且易于使用的C++库，旨在帮助开发者实现Windows平台上的智能自动化操作。无论是进行键盘、鼠标操作模拟，还是捕获屏幕、操作窗口，**AutoOperateLib** 都能为您提供简单而高效的API，使得开发复杂的自动化工具变得前所未有的轻松。

## 概述

**AutoOperateLib** 提供了一系列易于调用的API，这些API旨在帮助开发者快速实现以下自动化功能：

- **键盘和鼠标操作的录制与回放**：轻松记录并重现用户的键盘和鼠标操作。
- **屏幕截图与剪贴板操作**：截取屏幕区域或整个显示器，并轻松管理剪贴板内容。
- **窗口操作**：获取、移动、最小化、最大化窗口，甚至将窗口置顶或发送到后台。
- **系统信息获取**：例如显示器信息获取、高精度计时等。

## 特点

- **简洁易用**：每个功能都提供了清晰的API调用方式，便于开发者快速上手。
- **高效**：利用多线程技术，确保在执行复杂操作时不会阻塞主线程。
- **灵活**：丰富的操作选项和细粒度控制，满足各种自动化需求。

## 编译

**AutoOperateLib** 是一个纯C++库，只需打开原工程中的`AutoOperateLib.sln`并编译，即可得到`AutoOperateLib.lib`。

```sh
git clone https://github.com/RayShark0605/AutoOperateLib
```

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

## 贡献
欢迎众多开发者为**AutoOperateLib** 贡献代码！如果您发现问题或希望添加新功能，请提交Issue或Pull Request。

## 许可证
**AutoOperateLib** 遵循 MIT 许可证。
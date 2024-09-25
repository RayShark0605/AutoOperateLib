# AutoOperateLib

**AutoOperateLib** 是一个功能强大、简单易用的C++库，专为Windows平台上的智能自动化操作设计。无论是键盘、鼠标的模拟操作，还是屏幕捕获、窗口管理、文字识别、模版匹配等任务，**AutoOperateLib** 都能为您提供直观且高效的API，帮助您轻松构建复杂的自动化工具。

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

### 图像模版匹配
- **查找图片**：支持从全屏或指定区域中查找匹配目标图片。

### OCR文字识别
- **调用API**：支持百度、阿里巴巴、腾讯的文字识别OCR API。
- **简单调用**：无需关心API内部实现，只需通过`IsScreenExistsWords`一个函数就能非常方便地判断当前屏幕的目标区域是否存在某些文字，并且得到这些文字出现的位置。

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
- **区域找色**

该示例展示了如何在屏幕的某个区域内寻找颜色
```cpp
#include "Base.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    // 定义搜索区域 (从左上角坐标(100, 100)到右下角坐标(500, 500)的区域)
    const AO_Rect searchArea(100, 100, 400, 400);
    const AO_Color targetColor(255, 0, 0); // 定义目标颜色

    // 返回找到颜色的位置
    AO_Point foundPosition;
    // 在特定区域内找颜色，要求相似度大于0.9
    if (ScreenFindColor(targetColor, searchArea, foundPosition, 0.9))
    {
        std::cout << "在区域中找到颜色！位置: (" << foundPosition.x << ", " << foundPosition.y << ")" << std::endl;
    }
    else
    {
        std::cout << "未在区域中找到匹配的颜色。" << std::endl;
    }

    return 0;
}
```

- **屏幕绘制矩形**

该示例展示了如何在屏幕上绘制空心矩形，用来框选出重点区域
```cpp
#include "Base.h"
#include <iostream>

using namespace std;


int main(int argc, char* argv[])
{
    // 定义搜索区域 (从左上角坐标(100, 100)到右下角坐标(500, 500)的区域)
    const AO_Rect area(100, 100, 400, 400);
    DrawRectangleOnScreen(area, 3000, false); // 在屏幕上绘制空心矩形，持续3秒
    WaitForS(5);

    return 0;
}
```


- **图像匹配**

可以在屏幕截图中查找目标图像，支持设定匹配精度，快速实现视觉自动化。
```cpp
#include "CV.h"

using namespace std;
int main(int argc, char* argv[])
{
    const vector<AO_MonitorInfo> monitorsInfo = GetMonitorsInfo(); // 获取当前所有屏幕信息
    if (monitorsInfo.empty())
    {
        return 0;
    }
    const cv::Mat monitorScreenImage = CaptureScreenToCvMat(monitorsInfo[0]); // 截取第一个屏幕的内容
    const cv::Mat targetImage = cv::imread("target_image.jpg"); // 加载模版

    AO_Rect matchedRect;
    const bool found = FindImage(monitorScreenImage, targetImage, matchedRect); // 模版匹配
    if (found)
    {
        // 执行自动化操作
    }

    return 0;
}
```

- **OCR功能**

集成了OCR识别模块，支持对屏幕中的文字进行识别，极大拓展了文本处理的场景。（注意：使用此功能必须先自行申请百度、阿里或腾讯的**OCR API**，并且将相关**API Key**填入**ocr_config.ini**）
```cpp
#include "CV.h"

using namespace std;
int main(int argc, char* argv[])
{
    const vector<AO_MonitorInfo> monitorsInfo = GetMonitorsInfo(); // 获取当前所有屏幕信息
    if (monitorsInfo.empty())
    {
        return 0;
    }

    const vector<string> targetWords = { "原神", "绝区零", "启动" }; // 从第一个屏幕所在的内容中查找这三个词语
    AO_Rect wordPosition; // 返回的wordPosition表示以上三个词语中，识别结果中置信度最高的那个词语所在的位置
    const bool isFind = AO_OCR::IsScreenExistsWords(targetWords, wordPosition, monitorsInfo[0].GetRect());
    if (isFind)
    {
        // 执行自动化操作
    }

    return 0;
}
```

## 示例APP介绍
为了帮助用户快速上手，在**SampleApps**中提供了三个示例App，演示了**AutoOperateLib**的核心功能。

- **OperationRecorder**

OperationRecorder是一个强大的键鼠录制工具，能够精确记录用户的键盘和鼠标操作，并支持后续回放。这款工具特别适合需要重复执行某些操作的场景，比如测试自动化或工作流程自动化。

![1](https://github.com/user-attachments/assets/6e570d6b-24a8-4a43-b6d2-21b7e9015155)
![2](https://github.com/user-attachments/assets/dc77c51f-9e3e-46de-9ff7-3fe759b2b558)

主要特点：

录制并保存操作为脚本文件，便于随时回放操作。

支持对录制的操作进行过滤与编辑，确保操作的准确性。

轻松集成到任何自动化项目中，简化工作流程。

主要代码文件：`SampleApps\OperationRecorder\MainWindow.cpp`

- **QMouseClicker**

QMouseClicker是一个简单高效的工具，用于实时显示鼠标指针的当前位置及其对应的像素信息，特别适合需要精确点击的场景。此外，用户还可以通过快捷键启动连续快速的左键单击操作，适用于需要高频点击的应用场景，如游戏或自动化任务。

![3](https://github.com/user-attachments/assets/4710c8ac-80db-4b0a-8013-9e24b5d274b7)

主要特点：

实时显示鼠标指针坐标和屏幕像素颜色，适合调试和开发。

支持通过自定义快捷键触发鼠标连点，极大提升操作效率。

主要代码文件：`SampleApps\QMouseClicker\MainWindow.cpp`


- **WindowShift**

WindowShift是一款极具实用性的“老板键”工具。通过配置文件的设定，用户可以使用快捷键快速隐藏指定窗口并将某个窗口置顶。这款工具特别适合在多任务场景中快速隐藏敏感窗口，保护隐私，恢复时也十分方便。

![4](https://github.com/user-attachments/assets/6a9ddcdb-ae8d-442f-9674-139078834697)

主要特点：

快捷键一键隐藏和恢复窗口，保护您的隐私。

支持配置多个窗口和快捷键，自定义程度高，适合多种使用场景。

主要代码文件：`SampleApps\WindowShift\WindowShift.cpp`

## 贡献
欢迎众多开发者为**AutoOperateLib** 贡献代码！如果您发现问题或希望添加新功能，请提交Issue或Pull Request。

## 许可证
**AutoOperateLib** 遵循 MIT 许可证。

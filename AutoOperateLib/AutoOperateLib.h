#pragma once

#include <windows.h>
#include <thread>
#include <vector>
#include <string>
#include <condition_variable>

// 二维整数点
struct AO_Point
{
    int x = 0;
    int y = 0;
    AO_Point();
    AO_Point(int x, int y);
    AO_Point(POINT pt);
    POINT GetWin32Point() const;
};

// 矩形。注意，外部对成员变量的修改需要自行确保合理性
struct AO_Rect
{
    AO_Point leftTop;
    AO_Point rightBottom;
    int width = 0;
    int height = 0;
    AO_Rect();
    AO_Rect(const AO_Point& leftTop, const AO_Point& rightBottom);
    AO_Rect(const AO_Point& leftTop, int width, int height);
    AO_Rect(int leftTopX, int leftTopY, int width, int height);
    AO_Rect(const RECT& rect);
    RECT GetWin32Rect() const;
};

// 显示器信息
struct AO_MonitorInfo
{
    std::string deviceName = ""; // 操作系统内部命名
    AO_Point leftTop;            // 显示器左上角的全局坐标
    int width = 0;               // 显示器分辨率宽
    int height = 0;              // 显示器分辨率高
    AO_Rect GetRect() const;
};

// 获取显示器信息
std::vector<AO_MonitorInfo> GetMonitorsInfo();

// 高精度计时器
class AO_Timer
{
public:
    AO_Timer();
    void Start();    // 开始
    void Stop();     // 停止，不清除已经记录的数据
    void Pause();    // 暂停
    void Resume();   // 从暂停状态恢复
    void Restart();  // 重新开始
    void Reset();    // 停止并重设，这会清除已经记录的数据
    double ElapsedMilliseconds() const;
    double ElapsedSeconds() const;
    double ElapsedMinutes() const;

private:
    bool isRunning = false;   // 当前是否处于运行状态
    bool isPaused = false;    // 当前是否处于暂停状态
    LARGE_INTEGER startTime;
    LARGE_INTEGER pauseTime;
    LARGE_INTEGER frequency;  // 基于CPU支持的高精度时间戳计数器频率
    long long pausedDuration;
};

// 获取当前系统时间并输出为字符串。字符串格式为“2024-08-16_11-08-20-256”格式
std::string GetCurrentTimeAsString();

// 获取当前系统时间并输出为正整数。该正整数表示从1601年1月1日零时以来经过的毫秒数
unsigned long long GetCurrentTimeAsNum();

// 等待
void WaitForMS(int milliseconds); // 等待xx毫秒
void WaitForS(int seconds);       // 等待xx秒
void WaitForMin(int minutes);     // 等待xx分钟
void WaitForHour(int hours);      // 等待xx小时

// 键鼠操作类型
enum class AO_ActionType
{
    KeyDown = 0,
    KeyUp,
    MouseMove,
    MouseLeftDown,
    MouseLeftUp,
    MouseRightDown,
    MouseRightUp,
    MouseMiddleDown,
    MouseMiddleUp,
    MouseWheel
};

// 键盘动作数据
struct AO_KeyboardRecord
{
    DWORD vkCode;      // 虚拟键码
};

// 鼠标动作数据
struct AO_MouseMoveRecord                           // 鼠标移动数据
{
    AO_Point position; // 光标位置
    AO_MouseMoveRecord()
    {
        position = AO_Point(0, 0);
    }
};
typedef AO_MouseMoveRecord AO_MouseClickRecord;     // 鼠标点击数据
struct AO_MouseWheelRecord                          // 鼠标滚轮数据
{
    AO_Point position;  // 光标位置
    int delta = 0;      // 滚轮滚动值，向上/向下滚动最小单位时，该值是120/-120
    AO_MouseWheelRecord()
    {
        position = AO_Point(0, 0);
        delta = 0;
    }
};
union AO_ActionData
{
    AO_KeyboardRecord keyboard;
    AO_MouseMoveRecord mouseMove;
    AO_MouseClickRecord mouseClick;
    AO_MouseWheelRecord mouseWheel;
    AO_ActionData()
    {
        keyboard = AO_KeyboardRecord();
        mouseMove = AO_MouseMoveRecord();
        mouseClick = AO_MouseClickRecord();
        mouseWheel = AO_MouseWheelRecord();
    }
};

// 键鼠统一的操作记录结构体
struct AO_ActionRecord
{
    AO_ActionType actionType;
    AO_ActionData data;
    double timeSinceStart; // 从录制开始的时间偏移（以毫秒为单位）
};

// 键鼠操作的导入/导出
bool SaveRecordsToTxtFile(const std::vector<AO_ActionRecord>& records, const std::string& filePath);
std::vector<AO_ActionRecord> LoadRecordsFromTxtFile(const std::string& filePath);

// 获取当前鼠标指针的全局坐标
AO_Point GetMousePos();

// 将鼠标指针基于当前位置偏移(dx, dy)
bool MouseOffset(int dx, int dy);

// 将鼠标指针移动至全局坐标(x, y)处
bool MouseMoveTo(int x, int y);
bool MouseMoveTo(const AO_Point& point);

// 鼠标左键按下
void MouseLeftDown();
void MouseLeftDown(int x, int y);
void MouseLeftDown(const AO_Point& point);

// 鼠标左键抬起
void MouseLeftUp();
void MouseLeftUp(int x, int y);
void MouseLeftUp(const AO_Point& point);

// 鼠标左键单击，可以指定左键按下和左键抬起之间的时间间隔（单位：毫秒）
void MouseLeftClick(int timeIntervalMS = 100);
void MouseLeftClick(int x, int y, int timeIntervalMS = 100);
void MouseLeftClick(const AO_Point& point, int timeIntervalMS = 100);

// 鼠标右键按下
void MouseRightDown();
void MouseRightDown(int x, int y);
void MouseRightDown(const AO_Point& point);

// 鼠标右键抬起
void MouseRightUp();
void MouseRightUp(int x, int y);
void MouseRightUp(const AO_Point& point);

// 鼠标右键单击，可以指定右键按下和右键抬起之间的时间间隔（单位：毫秒）
void MouseRightClick(int timeIntervalMS = 100);
void MouseRightClick(int x, int y, int timeIntervalMS = 100);
void MouseRightClick(const AO_Point& point, int timeIntervalMS = 100);

// 鼠标中键按下
void MouseMiddleDown();
void MouseMiddleDown(int x, int y);
void MouseMiddleDown(const AO_Point& point);

// 鼠标中键抬起
void MouseMiddleUp();
void MouseMiddleUp(int x, int y);
void MouseMiddleUp(const AO_Point& point);

// 鼠标中键单击，可以指定中键按下和中键抬起之间的时间间隔（单位：毫秒）
void MouseMiddleClick(int timeIntervalMS = 100);
void MouseMiddleClick(int x, int y, int timeIntervalMS = 100);
void MouseMiddleClick(const AO_Point& point, int timeIntervalMS = 100);

// 鼠标滚轮滚动，当滚轮向上/向下滚动最小单位时，delta值是120/-120
void MouseWheel(int delta);
void MouseWheel(int x, int y, int delta, int timeIntervalMS = 100);   // 将鼠标指针移动到(x, y)处再滚动滚轮
void MouseWheel(const AO_Point& point, int delta, int timeIntervalMS = 100);

// 常用修饰键，更多修饰键参考https://learn.microsoft.com/zh-cn/windows/win32/inputdev/virtual-key-codes
enum AO_ModifierKey : BYTE
{
    BACK        = VK_BACK,      // 退格键
    TAB         = VK_TAB,       // Tab键
    RETURN      = VK_RETURN,    // 回车键
    LEFTSHIFT   = VK_LSHIFT,    // 左Shift键
    RIGHTSHIFT  = VK_RSHIFT,    // 右Shift键
    LEFTCTRL    = VK_LCONTROL,  // 左Ctrl键
    RIGHTCTRL   = VK_RCONTROL,  // 右Ctrl键
    CAPITAL     = VK_CAPITAL,   // Caps Lock键
    ESC         = VK_ESCAPE,    // Esc键
    SPACE       = VK_SPACE,     // 空格键
    UP          = VK_UP,        // 方向上键
    DOWN        = VK_DOWN,      // 方向下键
    LEFT        = VK_LEFT,      // 方向左键
    RIGHT       = VK_RIGHT,     // 方向右键
    DELE        = VK_DELETE,    // Delete键
    INSERT      = VK_INSERT,    // Insert键
    LEFTWIN     = VK_LWIN,      // 左Windows键
    RIGHTWIN    = VK_RWIN,      // 右Windows键
};

// 按下键盘按键
void KeyboardDown(BYTE key);

// 松开键盘按键
void KeyboardUp(BYTE key);

// 点击键盘按键（包括按下和松开两个步骤）
void KeyboardClick(BYTE key, int timeIntervalMS = 100);

// 检测当前是否是大写锁定状态
bool IsCapsLockOn();

// 键鼠操作录制器
class AO_ActionRecorder
{
public:
    AO_ActionRecorder();
    ~AO_ActionRecorder();
    void StartRecording();  // 开始录制
    void StopRecording();   // 结束录制
    std::vector<AO_ActionRecord> GetRecords() const; // 获取录制的所有操作

private:
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    void RecordAction(const AO_ActionRecord& record);
    void MessageLoop(); // 消息循环函数

    static AO_ActionRecorder* instance;
    HHOOK keyboardHook;
    HHOOK mouseHook;
    AO_Timer timer;
    std::vector<AO_ActionRecord> records;
    bool isRecording = false;
    std::thread hookThread;   // 用于钩子和消息循环的线程
    bool isStopMessageLoop = false;   // 控制消息循环的停止
};

// 键鼠操作模拟器
class AO_ActionSimulator
{
public:
    AO_ActionSimulator();
    void Start(const std::vector<AO_ActionRecord>& records); // 开始模拟（异步方式），不会阻塞当前线程
    void WaitForEnd(); // 等待模拟结束，这会阻塞当前线程       
    void Pause();      // 暂停
    void Resume();     // 从暂停中恢复
    void Stop();       // 停止

private:
    std::vector<AO_ActionRecord> records;
    bool isRunning = false;
    bool isPaused = false;
    std::thread simulationThread;
    std::condition_variable conditionVariable;
    std::mutex mtx;

    void SimulateActions();
    void PerformAction(const AO_ActionRecord& record);
};

// 将字符串添加到Windows剪切板
bool CopyTextToClipboard(const std::string& text);

// 将剪切板内容粘贴出来，本质上是执行Ctrl+V组合键
void PasteTextFromClipboard();

// 获取剪切板中的字符串。注意，如果剪切板中的内容不是文本内容，将返回false
bool GetClipboardContents(std::string& text);

// 在屏幕指定区域截图并保存为文件。图像编码格式将通过filePath文件后缀名自动判断。如果是不支持的图像编码格式或保存路径不存在将返回false。
bool CaptureScreenToFile(const AO_Rect& rect, const std::string& filePath);
bool CaptureScreenToFile(const AO_MonitorInfo& monitor, const std::string& filePath); // 截取某个显示器的全部内容

// 在屏幕指定区域截图并保存到剪切板
bool CaptureScreenToClipboard(const AO_Rect& rect);
bool CaptureScreenToClipboard(const AO_MonitorInfo& monitor); // 截取某个显示器的全部内容

// 窗口
struct AO_Window
{
    HWND hwnd = nullptr;          // 句柄
    std::string title = "";       // 标题
    std::string className = "";   // 类名
    AO_Rect rect;                 // 位置和大小
    bool isVisible = true;        // 是否可见
    bool isMinimized = false;     // 是否最小化
    bool isMaximized = false;     // 是否最大化
};

// 获取当前所有窗口。isStreamliningMode为true时，将忽略工具窗口、无标题窗口、大小为0的窗口等。
std::vector<AO_Window> GetAllOpenWindows(bool isStreamliningMode = true);

// 获取所有标题包含传入字符串的窗口。isConsiderUpperAndLower为false时，将不考虑字符串的大小写差异。
std::vector<AO_Window> GetWindows(const std::string& titleSubString, bool isConsiderUpperAndLower = false);

// 设置窗口可见性
void SetWindowVisibility(AO_Window& window, bool isVisible);

// 最小化窗口
void MinimizeWindow(AO_Window& window);

// 最大化窗口
void MaximizeWindow(AO_Window& window);

// 恢复窗口（从最小化或最大化状态恢复）
void RestoreWindow(AO_Window& window);

// 移动窗口
void MoveWindow(AO_Window& window, int newLeftTopX, int newLeftTopY);
void MoveWindow(AO_Window& window, const AO_Point& newLeftTopPoint);

// 改变窗口大小
void ResizeWindow(AO_Window& window, int newWidth, int newHeight);

// 移动并改变窗口大小。如果将窗口大小设置为与屏幕分辨率相同（或更大），那么窗口可能不会移动！
void MoveAndResizeWindow(AO_Window& window, const AO_Rect& newRect);
void MoveAndResizeWindow(AO_Window& window, const AO_Point& newLeftTopPoint, const AO_Point& newRightBottomPoint);
void MoveAndResizeWindow(AO_Window& window, const AO_Point& newLeftTopPoint, int newWidth, int newHeight);
void MoveAndResizeWindow(AO_Window& window, int newLeftTopX, int newLeftTopY, int newWidth, int newHeight);

// 窗口置顶（窗口置为前台）
void BringWindowToFront(AO_Window& window);

// 窗口置于后台，使其不再是活动窗口
void SendWindowToBack(AO_Window& window);


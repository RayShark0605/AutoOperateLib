#include "AutoOperateLib.h"

#include <iomanip>
#include <sstream>

using namespace std;

AO_Point::AO_Point() {}
AO_Point::AO_Point(int x, int y) :x(x), y(y) {}
AO_Point::AO_Point(POINT pt)
{
    x = pt.x;
    y = pt.y;
}

string WCharToString(const wchar_t* wchar)
{
    char buffer[256];
#pragma warning(suppress : 4996)
    wcstombs(buffer, wchar, 256);
    return string(buffer);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    vector<AO_MonitorInfo>* monitors = reinterpret_cast<vector<AO_MonitorInfo>*>(dwData);
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);

    if (GetMonitorInfo(hMonitor, &monitorInfo))
    {
        DISPLAY_DEVICE displayDevice;
        displayDevice.cb = sizeof(DISPLAY_DEVICE);
        if (EnumDisplayDevices(monitorInfo.szDevice, 0, &displayDevice, 0))
        {
            DEVMODE devMode;
            ZeroMemory(&devMode, sizeof(devMode));
            devMode.dmSize = sizeof(DEVMODE);

            if (EnumDisplaySettingsEx(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode, 0))
            {
                AO_MonitorInfo info;
                info.deviceName = WCharToString(monitorInfo.szDevice);
                info.leftTop.x = devMode.dmPosition.x;
                info.leftTop.y = devMode.dmPosition.y;
                info.width = devMode.dmPelsWidth;
                info.height = devMode.dmPelsHeight;

                monitors->push_back(info);
            }
        }
    }
    return TRUE;
}
vector<AO_MonitorInfo> GetMonitorsInfo()
{
    vector<AO_MonitorInfo> monitors;
    SetProcessDPIAware();

    if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        return {};
    }
    return monitors;
}

AO_Timer::AO_Timer() : isRunning(false), isPaused(false), pausedDuration(0)
{
    QueryPerformanceFrequency(&frequency);
}
void AO_Timer::Start()
{
    QueryPerformanceCounter(&startTime);
    isRunning = true;
    isPaused = false;
    pausedDuration = 0;
}
void AO_Timer::Stop()
{
    if (isRunning && !isPaused)
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        pausedDuration += (currentTime.QuadPart - startTime.QuadPart);
        isRunning = false;
    }
}
void AO_Timer::Pause()
{
    if (isRunning && !isPaused)
    {
        QueryPerformanceCounter(&pauseTime);
        isPaused = true;
    }
}
void AO_Timer::Resume()
{
    if (isRunning && isPaused)
    {
        LARGE_INTEGER resumeTime;
        QueryPerformanceCounter(&resumeTime);
        startTime.QuadPart += (resumeTime.QuadPart - pauseTime.QuadPart);
        isPaused = false;
    }
}
void AO_Timer::Restart()
{
    QueryPerformanceCounter(&startTime);
    isRunning = true;
    isPaused = false;
    pausedDuration = 0;
}
void AO_Timer::Reset()
{
    isRunning = false;
    isPaused = false;
    pausedDuration = 0;
}
double AO_Timer::ElapsedMilliseconds() const
{
    if (isRunning)
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        if (isPaused)
        {
            return (pauseTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
        }
        return (currentTime.QuadPart - startTime.QuadPart + pausedDuration) * 1000.0 / frequency.QuadPart;
    }
    return pausedDuration * 1000.0 / frequency.QuadPart;
}
double AO_Timer::ElapsedSeconds() const
{
    return ElapsedMilliseconds() / 1000;
}
double AO_Timer::ElapsedMinutes() const
{
    return ElapsedMilliseconds() / 60000;
}

string GetCurrentTimeAsString()
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    ostringstream oss;
    oss << setfill('0')
        << systemTime.wYear << "-"
        << setw(2) << systemTime.wMonth << "-"
        << setw(2) << systemTime.wDay << "_"
        << setw(2) << systemTime.wHour << "-"
        << setw(2) << systemTime.wMinute << "-"
        << setw(2) << systemTime.wSecond << "-"
        << setw(3) << systemTime.wMilliseconds;
    return oss.str();
}

unsigned long long GetCurrentTimeAsNum()
{
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    ULARGE_INTEGER largeInteger;
    largeInteger.LowPart = fileTime.dwLowDateTime;
    largeInteger.HighPart = fileTime.dwHighDateTime;

    return largeInteger.QuadPart / 10000;
}

void WaitForMS(int milliseconds)
{
    this_thread::sleep_for(chrono::milliseconds(milliseconds));
}
void WaitForS(int seconds)
{
    this_thread::sleep_for(chrono::seconds(seconds));
}
void WaitForMin(int minutes)
{
    this_thread::sleep_for(chrono::minutes(minutes));
}
void WaitForHour(int hours)
{
    this_thread::sleep_for(chrono::hours(hours));
}

AO_Point GetMousePos()
{
    POINT pt;
    if (GetCursorPos(&pt))
    {
        return AO_Point(pt.x, pt.y);
    }
    return AO_Point(-1, -1);
}

bool MouseOffset(int dx, int dy)
{
    const AO_Point point = GetMousePos();
    return MouseMoveTo(point.x + dx, point.y + dy);
}

bool MouseMoveTo(int x, int y)
{
    return SetCursorPos(x, y);
}
bool MouseMoveTo(const AO_Point& point)
{
    return SetCursorPos(point.x, point.y);
}

void MouseLeftDown()
{
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
}
void MouseLeftDown(int x, int y)
{
    MouseMoveTo(x, y);
    MouseLeftDown();
}
void MouseLeftDown(const AO_Point& point)
{
    MouseMoveTo(point.x, point.y);
    MouseLeftDown();
}

void MouseLeftUp()
{
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
void MouseLeftUp(int x, int y)
{
    MouseMoveTo(x, y);
    MouseLeftUp();
}
void MouseLeftUp(const AO_Point& point)
{
    MouseMoveTo(point.x, point.y);
    MouseLeftUp();
}

void MouseLeftClick(int timeIntervalMS)
{
    MouseLeftDown();
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseLeftUp();
}
void MouseLeftClick(int x, int y, int timeIntervalMS)
{
    MouseLeftDown(x, y);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseLeftUp(x, y);
}
void MouseLeftClick(const AO_Point& point, int timeIntervalMS)
{
    MouseLeftDown(point);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseLeftUp(point);
}

void MouseRightDown()
{
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
}
void MouseRightDown(int x, int y)
{
    MouseMoveTo(x, y);
    MouseRightDown();
}
void MouseRightDown(const AO_Point& point)
{
    MouseMoveTo(point.x, point.y);
    MouseRightDown();
}

void MouseRightUp()
{
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}
void MouseRightUp(int x, int y)
{
    MouseMoveTo(x, y);
    MouseRightUp();
}
void MouseRightUp(const AO_Point& point)
{
    MouseMoveTo(point.x, point.y);
    MouseRightUp();
}

void MouseRightClick(int timeIntervalMS)
{
    MouseRightDown();
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseRightUp();
}
void MouseRightClick(int x, int y, int timeIntervalMS)
{
    MouseRightDown(x, y);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseRightUp(x, y);
}
void MouseRightClick(const AO_Point& point, int timeIntervalMS)
{
    MouseRightDown(point);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseRightUp(point);
}

void MouseMiddleDown()
{
    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
}
void MouseMiddleDown(int x, int y)
{
    MouseMoveTo(x, y);
    MouseMiddleDown();
}
void MouseMiddleDown(const AO_Point& point)
{
    MouseMoveTo(point);
    MouseMiddleDown();
}

void MouseMiddleUp()
{
    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
}
void MouseMiddleUp(int x, int y)
{
    MouseMoveTo(x, y);
    MouseMiddleUp();
}
void MouseMiddleUp(const AO_Point& point)
{
    MouseMoveTo(point);
    MouseMiddleUp();
}

void MouseMiddleClick(int timeIntervalMS)
{
    MouseMiddleDown();
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseMiddleUp();
}
void MouseMiddleClick(int x, int y, int timeIntervalMS)
{
    MouseMiddleDown(x, y);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseMiddleUp(x, y);
}
void MouseMiddleClick(const AO_Point& point, int timeIntervalMS)
{
    MouseMiddleDown(point);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    MouseMiddleUp(point);
}

void KeyboardDown(BYTE key)
{
    keybd_event(key, 0, 0, 0);
}

void KeyboardUp(BYTE key)
{
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

void KeyboardClick(BYTE key, int timeIntervalMS)
{
    KeyboardDown(key);
    this_thread::sleep_for(chrono::milliseconds(timeIntervalMS));
    KeyboardUp(key);
}

bool IsCapsLockOn()
{
    SHORT capsLockState = GetKeyState(VK_CAPITAL);
    return (capsLockState & 0x0001) != 0;
}

AO_ActionRecorder* AO_ActionRecorder::instance = nullptr;
AO_ActionRecorder::AO_ActionRecorder() : isRecording(false), isStopMessageLoop(false), keyboardHook(nullptr), mouseHook(nullptr)
{
}
AO_ActionRecorder::~AO_ActionRecorder()
{
    StopRecording(); // 确保在销毁对象时停止录制
}
void AO_ActionRecorder::StartRecording()
{
    if (isRecording)
    {
        return;
    }

    instance = this;
    records.clear();
    isRecording = true;
    isStopMessageLoop = false;

    // 启动钩子和消息循环的线程
    hookThread = thread(&AO_ActionRecorder::MessageLoop, this);
}
void AO_ActionRecorder::StopRecording()
{
    if (!isRecording)
    {
        return;
    }

    isStopMessageLoop = true; // 标志消息循环停止

    if (hookThread.joinable())
    {
        hookThread.join(); // 等待消息循环线程结束
    }

    timer.Stop();
    isRecording = false;
}
vector<AO_ActionRecord> AO_ActionRecorder::GetRecords() const
{
    return records;
}
LRESULT CALLBACK AO_ActionRecorder::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && instance->isRecording)
    {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        AO_ActionRecord record;
        record.actionType = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? AO_ActionType::KeyDown : AO_ActionType::KeyUp;
        record.data.keyboard.vkCode = pKeyboard->vkCode;
        record.timeSinceStart = instance->timer.ElapsedMilliseconds();
        instance->RecordAction(record);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
LRESULT CALLBACK AO_ActionRecorder::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && instance->isRecording)
    {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        AO_ActionRecord record;
        record.timeSinceStart = instance->timer.ElapsedMilliseconds();

        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            record.actionType = AO_ActionType::MouseLeftDown;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_LBUTTONUP:
            record.actionType = AO_ActionType::MouseLeftUp;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_RBUTTONDOWN:
            record.actionType = AO_ActionType::MouseRightDown;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_RBUTTONUP:
            record.actionType = AO_ActionType::MouseRightUp;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_MBUTTONDOWN:
            record.actionType = AO_ActionType::MouseMiddleDown;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_MBUTTONUP:
            record.actionType = AO_ActionType::MouseMiddleUp;
            record.data.mouseClick.position = pMouse->pt;
            break;
        case WM_MOUSEMOVE:
            record.actionType = AO_ActionType::MouseMove;
            record.data.mouseMove.position = pMouse->pt;
            break;
        case WM_MOUSEWHEEL:
            record.actionType = AO_ActionType::MouseWheel;
            record.data.mouseWheel.position = pMouse->pt;
            record.data.mouseWheel.delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
            break;
        default:
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }
        instance->RecordAction(record);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
void AO_ActionRecorder::RecordAction(const AO_ActionRecord& record)
{
    records.push_back(record);
}
void AO_ActionRecorder::MessageLoop()
{
    // 在与消息循环相同的线程中安装钩子
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    if (!keyboardHook)
    {
        return;
    }

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);
    if (!mouseHook)
    {
        UnhookWindowsHookEx(keyboardHook);
        return;
    }

    timer.Reset();
    timer.Start();

    MSG msg;
    while (!isStopMessageLoop)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(50); // 避免忙等待
    }

    // 清理钩子
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
}

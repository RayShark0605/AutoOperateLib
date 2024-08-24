#include "AutoOperateLib.h"

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <gdiplus.h>

using namespace std;

AO_Point::AO_Point()
{
    x = y = 0;
}
AO_Point::AO_Point(int x, int y) :x(x), y(y) {}
AO_Point::AO_Point(POINT pt)
{
    x = pt.x;
    y = pt.y;
}
POINT AO_Point::GetWin32Point() const
{
    POINT pt;
    pt.x = x;
    pt.y = y;
    return pt;
}

AO_Rect::AO_Rect()
{
    leftTop = AO_Point(0, 0);
    rightBottom = AO_Point(0, 0);
    width = height = 0;
}
AO_Rect::AO_Rect(const AO_Point& leftTop, const AO_Point& rightBottom)
{
    this->leftTop = leftTop;
    this->rightBottom = rightBottom;
    width = rightBottom.x - leftTop.x;
    height = rightBottom.y - leftTop.y;
}
AO_Rect::AO_Rect(const AO_Point& leftTop, int width, int height)
{
    this->leftTop = leftTop;
    this->width = width;
    this->height = height;
    rightBottom = AO_Point(leftTop.x + width, leftTop.y + height);
}
AO_Rect::AO_Rect(int leftTopX, int leftTopY, int width, int height)
{
    leftTop = AO_Point(leftTopX, leftTopY);
    this->width = width;
    this->height = height;
    rightBottom = AO_Point(leftTopX + width, leftTopY + height);
}
AO_Rect::AO_Rect(const RECT& rect)
{
    leftTop = AO_Point(rect.left, rect.top);
    rightBottom = AO_Point(rect.right, rect.bottom);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}
RECT AO_Rect::GetWin32Rect() const
{
    RECT rect;
    rect.left = leftTop.x;
    rect.bottom = rightBottom.y;
    rect.right = rightBottom.x;
    rect.top = leftTop.y;
    return rect;
}

AO_Rect AO_MonitorInfo::GetRect() const
{
    return AO_Rect(leftTop, width, height);
}

wstring StringToWString(const string& str)
{
    int len;
    const int slength = static_cast<int>(str.length());
    len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
    wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, &r[0], len);
    return r;
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
AO_Timer::~AO_Timer()
{
    stopCallbackChecker = true;
    Stop();
}
void AO_Timer::Start()
{
    QueryPerformanceCounter(&startTime);
    isRunning = true;
    isPaused = false;
    pausedDuration = 0;

    if (userCallback)
    {
        stopCallbackChecker = false;
        callbackThread = thread(&AO_Timer::CallbackChecker, this);
    }
}
void AO_Timer::Stop()
{
    if (isRunning && !isPaused)
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        pausedDuration += (currentTime.QuadPart - startTime.QuadPart);
        isRunning = false;

        stopCallbackChecker = true;
        if (callbackThread.joinable())
        {
            callbackThread.join();
        }
    }
}
void AO_Timer::Pause()
{
    if (isRunning && !isPaused)
    {
        QueryPerformanceCounter(&pauseTime);
        isPaused = true;

        stopCallbackChecker = true;
        if (callbackThread.joinable())
        {
            callbackThread.join();
        }
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

        if (userCallback)
        {
            stopCallbackChecker = false;
            callbackThread = thread(&AO_Timer::CallbackChecker, this);
        }
    }
}
void AO_Timer::Restart()
{
    Stop();
    Start();
}
void AO_Timer::Reset()
{
    Stop();
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
void AO_Timer::SetCallback(double intervalMilliseconds, CallbackFunction callback, bool isAsynCallback)
{
    userCallback = callback;
    callbackIntervalMilliseconds = intervalMilliseconds;
    this->isAsynCallback = isAsynCallback;
}
void AO_Timer::CallbackChecker()
{
    double startTime = ElapsedMilliseconds();
    double callbackTime = startTime + callbackIntervalMilliseconds;
    while (!stopCallbackChecker)
    {
        if (!isPaused && !stopCallbackChecker && userCallback)
        {
            while (!stopCallbackChecker && ElapsedMilliseconds() < callbackTime)
            {

            }
            if (!stopCallbackChecker)
            {
                if (isAsynCallback)
                {
                    thread t([&]() {userCallback(); });
                    t.detach();
                }
                else
                {
                    userCallback();
                }
            }
            startTime = ElapsedMilliseconds();
            callbackTime = startTime + callbackIntervalMilliseconds;
        }
    }
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

AO_HotkeyManager::AO_HotkeyManager()
{
    isRunning = true;
    processThread = thread([this](){ this->MessageLoop(); });
}
AO_HotkeyManager::~AO_HotkeyManager()
{
    isRunning = false;
    if (processThread.joinable())
    {
        PostThreadMessage(GetThreadId(processThread.native_handle()), WM_QUIT, 0, 0);
        processThread.join();
    }
}
void AO_HotkeyManager::RegisterHotkey(int id, UINT modifiers, UINT vk, HotkeyCallback callback)
{
    lock_guard<mutex> lock(mtx);
    hotkeys[id] = { modifiers, vk };
    callbacks[id] = callback;
}
void AO_HotkeyManager::UnregisterHotkey(int id)
{
    lock_guard<mutex> lock(mtx);
    hotkeys.erase(id);
    callbacks.erase(id);
}
void AO_HotkeyManager::MessageLoop()
{
    // 必须确保 RegisterHotKey 和 GetMessage 事件循环在同一个线程
    while (isRunning)
    {
        {
            lock_guard<mutex> lock(mtx);
            for (const auto& pair : hotkeys)
            {
                RegisterHotKey(nullptr, pair.first, pair.second.first, pair.second.second);
            }
        }
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (msg.message == WM_HOTKEY && callbacks.find(msg.wParam) != callbacks.end())
            {
                callbacks[msg.wParam]();
            }
            else if (msg.message == WM_QUIT)
            {
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        {
            lock_guard<mutex> lock(mtx);
            for (const auto& pair : hotkeys)
            {
                UnregisterHotKey(nullptr, pair.first);
            }
        }
        
        WaitForMS(500);
    }
}

bool SaveRecordsToTxtFile(const vector<AO_ActionRecord>& records, const string& filePath)
{
    ofstream outFile(filePath);
    if (!outFile.is_open())
    {
        return false;
    }
    for (const AO_ActionRecord& record : records)
    {
        outFile << "ActionType: " << static_cast<int>(record.actionType) << ", TimeSinceStart: " << record.timeSinceStart << "ms" << endl;
        switch (record.actionType)
        {
        case AO_ActionType::KeyDown:
        case AO_ActionType::KeyUp:
            outFile << "  vkCode: " << record.data.keyboard.vkCode << endl;;
            break;
        case AO_ActionType::MouseMove:
            outFile << "  MouseMove Position: (" << record.data.mouseMove.position.x << ", " << record.data.mouseMove.position.y << ")" << endl;
            break;
        case AO_ActionType::MouseLeftDown:
        case AO_ActionType::MouseLeftUp:
        case AO_ActionType::MouseRightDown:
        case AO_ActionType::MouseRightUp:
        case AO_ActionType::MouseMiddleDown:
        case AO_ActionType::MouseMiddleUp:
            outFile << "  MouseClick Position: (" << record.data.mouseClick.position.x << ", " << record.data.mouseClick.position.y << ")" << endl;
            break;
        case AO_ActionType::MouseWheel:
            outFile << "  MouseWheel Position: (" << record.data.mouseWheel.position.x << ", " << record.data.mouseWheel.position.y << "), Delta: " << record.data.mouseWheel.delta << endl;
            break;
        default:
            outFile << "  Unknown action type" << endl;
            break;
        }
    }
    outFile.close();
    return true;
}
vector<AO_ActionRecord> LoadRecordsFromTxtFile(const string& filePath)
{
    ifstream inFile(filePath);

    if (!inFile.is_open())
    {
        return {};
    }

    vector<AO_ActionRecord> records;
    string line;
    while (getline(inFile, line))
    {
        if (line.find("ActionType:") == string::npos)
        {
            continue;
        }
        int actionTypeInt = 0;
        double timeSinceStart = 0;

        {
            istringstream iss(line);
            char c = 0;
            iss >> skipws >> line >> actionTypeInt >> c >> line >> timeSinceStart;
        }
        
        AO_ActionRecord currentRecord;
        currentRecord.actionType = static_cast<AO_ActionType>(actionTypeInt);
        currentRecord.timeSinceStart = timeSinceStart;

        if (!getline(inFile, line))
        {
            break;
        }
        istringstream iss(line);
        if (line.find("vkCode:") != string::npos)
        {
            DWORD vkCode = 0;
            iss >> line >> vkCode;
            currentRecord.data.keyboard.vkCode = vkCode;
            records.push_back(currentRecord);
        }
        else if (line.find("MouseMove Position:") != string::npos)
        {
            int x = 0, y = 0;
            char c = 0;
            iss >> line >> line >> c >> x >> c >> y;
            currentRecord.data.mouseMove.position = AO_Point(x, y);
            records.push_back(currentRecord);
        }
        else if (line.find("MouseClick Position:") != string::npos)
        {
            int x = 0, y = 0;
            char c = 0;
            iss >> line >> line >> c >> x >> c >> y;
            currentRecord.data.mouseClick.position = AO_Point(x, y);
            records.push_back(currentRecord);
        }
        else if (line.find("MouseWheel Position:") != string::npos)
        {
            int x = 0, y = 0, delta = 0;
            char c = 0;
            iss >> line >> line >> c >> x >> c >> y >> line >> line >> delta;
            currentRecord.data.mouseWheel.position = AO_Point(x, y);
            currentRecord.data.mouseWheel.delta = delta;
            records.push_back(currentRecord);
        }
    }
    inFile.close();
    return records;
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

void MouseWheel(int delta)
{
    mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
}
void MouseWheel(int x, int y, int delta, int timeIntervalMS)
{
    MouseMoveTo(x, y);
    WaitForMS(timeIntervalMS);
    MouseWheel(delta);
}
void MouseWheel(const AO_Point& point, int delta, int timeIntervalMS)
{
    MouseMoveTo(point.x, point.y);
    WaitForMS(timeIntervalMS);
    MouseWheel(delta);
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

AO_ActionTrigger* AO_ActionTrigger::instance = nullptr;
AO_ActionTrigger::AO_ActionTrigger()
{
    isRunning = true;
    instance = this;

    hookThread = thread(&AO_ActionTrigger::MessageLoop, this);
    SetThreadPriority(hookThread.native_handle(), THREAD_PRIORITY_HIGHEST);
    
    timer.Start();
}
AO_ActionTrigger::~AO_ActionTrigger()
{
    timer.Stop();

    isRunning = false;
    if (hookThread.joinable())
    {
        hookThread.join(); // 等待消息循环线程结束
    }
}
void AO_ActionTrigger::RegisterCallback(AO_ActionType actionType, Callback callback)
{
    const double timeSinceStart = timer.ElapsedMilliseconds();
    callbacks[actionType] = callback;
    startTimes[actionType] = timeSinceStart;
}
LRESULT CALLBACK AO_ActionTrigger::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    const double timeSinceStart = instance->timer.ElapsedMilliseconds();
    if (nCode == HC_ACTION)
    {
        const KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        AO_ActionType actionType;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            actionType = AO_ActionType::KeyDown;
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            actionType = AO_ActionType::KeyUp;
        }
        else
        {
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        AO_ActionRecord record;
        record.actionType = actionType;
        record.data.keyboard.vkCode = pKeyboard->vkCode;
        if (instance->startTimes.find(actionType) != instance->startTimes.end())
        {
            record.timeSinceStart = timeSinceStart - instance->startTimes.at(actionType);
        }
        instance->TriggerCallback(record);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
LRESULT CALLBACK AO_ActionTrigger::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    const double timeSinceStart = instance->timer.ElapsedMilliseconds();
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        AO_ActionType actionType;

        switch (wParam)
        {
        case WM_MOUSEMOVE:
            actionType = AO_ActionType::MouseMove;
            break;
        case WM_LBUTTONDOWN:
            actionType = AO_ActionType::MouseLeftDown;
            break;
        case WM_LBUTTONUP:
            actionType = AO_ActionType::MouseLeftUp;
            break;
        case WM_RBUTTONDOWN:
            actionType = AO_ActionType::MouseRightDown;
            break;
        case WM_RBUTTONUP:
            actionType = AO_ActionType::MouseRightUp;
            break;
        case WM_MBUTTONDOWN:
            actionType = AO_ActionType::MouseMiddleDown;
            break;
        case WM_MBUTTONUP:
            actionType = AO_ActionType::MouseMiddleUp;
            break;
        case WM_MOUSEWHEEL:
            actionType = AO_ActionType::MouseWheel;
            break;
        default:
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        AO_ActionRecord record;
        record.actionType = actionType;
        record.data.mouseMove.position = AO_Point(pMouse->pt.x, pMouse->pt.y);
        if (instance->startTimes.find(actionType) != instance->startTimes.end())
        {
            record.timeSinceStart = timeSinceStart - instance->startTimes.at(actionType);
        }
        if (actionType == AO_ActionType::MouseWheel)
        {
            record.data.mouseWheel.delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
        }
        instance->TriggerCallback(record);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
void AO_ActionTrigger::TriggerCallback(const AO_ActionRecord& record) const
{
    const auto it = callbacks.find(record.actionType);
    if (it != callbacks.end())
    {
        it->second(record);
    }
}
void AO_ActionTrigger::MessageLoop()
{
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
    MSG msg;
    while (isRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    // 清理钩子
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
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
    SetThreadPriority(hookThread.native_handle(), THREAD_PRIORITY_HIGHEST);
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
    }

    // 清理钩子
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
}

AO_ActionSimulator::AO_ActionSimulator() : isRunning(false), isPaused(false)
{
}
void AO_ActionSimulator::Start(const vector<AO_ActionRecord>& records)
{
    this->records = records;
    isRunning = true;
    isPaused = false;

    simulationThread = thread(&AO_ActionSimulator::SimulateActions, this);
    const HANDLE threadHandle = simulationThread.native_handle();
    SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
}
void AO_ActionSimulator::WaitForEnd()
{
    if (simulationThread.joinable())
    {
        simulationThread.join();
    }
}
void AO_ActionSimulator::Pause()
{
    unique_lock<mutex> lock(mtx);
    isPaused = true;
}
void AO_ActionSimulator::Resume()
{
    unique_lock<mutex> lock(mtx);
    isPaused = false;
    conditionVariable.notify_one();
}
void AO_ActionSimulator::Stop()
{
    isRunning = false;
    if (simulationThread.joinable())
    {
        simulationThread.join();
    }
}
void AO_ActionSimulator::SimulateActions()
{
    AO_Timer timer;
    timer.Start();
    for (const AO_ActionRecord& record : records)
    {
        while (isPaused)
        {
            unique_lock<mutex> lock(mtx);
            conditionVariable.wait(lock, [this]() { return !isPaused; });
        }

        if (!isRunning)
        {
            break;
        }

        const double elapsed = timer.ElapsedMilliseconds();
        if (record.timeSinceStart > elapsed)
        {
            this_thread::sleep_for(chrono::milliseconds(static_cast<int>(record.timeSinceStart - elapsed)));
        }

        PerformAction(record);
    }
    isRunning = false;
}
void AO_ActionSimulator::PerformAction(const AO_ActionRecord& record)
{
    switch (record.actionType)
    {
    case AO_ActionType::KeyDown:
        KeyboardDown(record.data.keyboard.vkCode);
        break;
    case AO_ActionType::KeyUp:
        KeyboardUp(record.data.keyboard.vkCode);
        break;
    case AO_ActionType::MouseMove:
        MouseMoveTo(record.data.mouseMove.position);
        break;
    case AO_ActionType::MouseLeftDown:
        MouseLeftDown(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseLeftUp:
        MouseLeftUp(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseRightDown:
        MouseRightDown(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseRightUp:
        MouseRightUp(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseMiddleDown:
        MouseMiddleDown(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseMiddleUp:
        MouseMiddleUp(record.data.mouseClick.position);
        break;
    case AO_ActionType::MouseWheel:
        MouseWheel(record.data.mouseWheel.position, record.data.mouseWheel.delta);
        break;
    default:
        break;
    }
}

bool CopyTextToClipboard(const string& text)
{
    if (!OpenClipboard(nullptr))
    {
        return false;
    }

    // 清空剪切板
    if (!EmptyClipboard())
    {
        CloseClipboard();
        return false;
    }

    // 为字符串分配全局内存
    const HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(char));
    if (!hGlobal)
    {
        CloseClipboard();
        return false;
    }

    // 将字符串复制到全局内存
    const LPVOID pGlobal = GlobalLock(hGlobal);
    memcpy(pGlobal, text.c_str(), text.size() + 1);
    GlobalUnlock(hGlobal);

    // 将全局内存内容设置为剪切板文本数据
    if (!SetClipboardData(CF_TEXT, hGlobal))
    {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

void PasteTextFromClipboard()
{
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('V', 0, 0, 0);
    keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}

bool GetClipboardContents(string& text)
{
    if (!OpenClipboard(nullptr))
    {
        return false;
    }
    const HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData)
    {
        CloseClipboard();
        return false;
    }
    const char* pText = static_cast<char*>(GlobalLock(hData));
    if (!pText)
    {
        GlobalUnlock(hData);
        CloseClipboard();
        return false;
    }
    text = string(pText);
    GlobalUnlock(hData);
    CloseClipboard();
    return true;
}

#pragma comment (lib,"Gdiplus.lib")

// 辅助函数：获取图像编码器的CLSID
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0; // 编码器的数量
    UINT size = 0; // 图像编码器的大小（以字节为单位）

    Gdiplus::ImageCodecInfo* pImageCodecInfo = nullptr;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
    {
        return -1; // 没有找到编码器
    }

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo)
    {
        return -1; // 内存分配失败
    }

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return i; // 找到了编码器
        }
    }

    free(pImageCodecInfo);
    return -1; // 没有找到编码器
}
bool CaptureScreenToFile(const AO_Rect& rect, const string& filePath)
{
    const wstring wFilePath = StringToWString(filePath);

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // 获取屏幕设备上下文
    const HDC screenDC = GetDC(nullptr);
    const HDC memDC = CreateCompatibleDC(screenDC);

    // 创建兼容的位图
    const HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, rect.width, rect.height);
    const HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

    // 从屏幕设备上下文中复制内容到内存设备上下文中
    BitBlt(memDC, 0, 0, rect.width, rect.height, screenDC, rect.leftTop.x, rect.leftTop.y, SRCCOPY);

    // 创建GDI+ Bitmap对象
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, nullptr);

    // 根据文件扩展名获取对应的编码器CLSID
    CLSID clsid;
    wstring extension = wFilePath.substr(wFilePath.find_last_of(L".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

    int findClsidResult = -1;
    if (extension == L"png")
    {
        findClsidResult = GetEncoderClsid(L"image/png", &clsid);
    }
    else if (extension == L"jpg" || extension == L"jpeg")
    {
        findClsidResult = GetEncoderClsid(L"image/jpeg", &clsid);
    }
    else if (extension == L"bmp")
    {
        findClsidResult = GetEncoderClsid(L"image/bmp", &clsid);
    }
    else if (extension == L"gif")
    {
        findClsidResult = GetEncoderClsid(L"image/gif", &clsid);
    }
    else if (extension == L"tiff")
    {
        findClsidResult = GetEncoderClsid(L"image/tiff", &clsid);
    }
    if (findClsidResult < 0)
    {
        delete bitmap;  // 释放bitmap对象
        SelectObject(memDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);

        // 关闭GDI+
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // 保存截图到文件
    const Gdiplus::Status status = bitmap->Save(wFilePath.c_str(), &clsid, nullptr);

    // 清理资源
    delete bitmap;  // 释放bitmap对象
    SelectObject(memDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    // 关闭GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (status == Gdiplus::Status::Ok);
}
bool CaptureScreenToFile(const AO_MonitorInfo& monitor, const string& filePath)
{
    return CaptureScreenToFile(monitor.GetRect(), filePath);
}

bool CaptureScreenToClipboard(const AO_Rect& rect)
{
    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // 获取屏幕设备上下文
    const HDC screenDC = GetDC(nullptr);
    const HDC memDC = CreateCompatibleDC(screenDC);

    // 创建兼容的位图
    const HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, rect.width, rect.height);
    const HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

    // 从屏幕设备上下文中复制内容到内存设备上下文中
    BitBlt(memDC, 0, 0, rect.width, rect.height, screenDC, rect.leftTop.x, rect.leftTop.y, SRCCOPY);

    // 打开剪贴板
    if (!OpenClipboard(nullptr))
    {
        SelectObject(memDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // 清空剪贴板
    EmptyClipboard();

    // 将位图设置到剪贴板中
    if (SetClipboardData(CF_BITMAP, hBitmap) == nullptr)
    {
        CloseClipboard();
        SelectObject(memDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // 关闭剪贴板
    CloseClipboard();

    // 清理资源
    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    // 此处不应删除 hBitmap，因为它已被设置到剪贴板并将由系统管理

    // 关闭GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return true;
}
bool CaptureScreenToClipboard(const AO_MonitorInfo& monitor)
{
    return CaptureScreenToClipboard(monitor.GetRect());
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto params = reinterpret_cast<pair<vector<AO_Window>*, bool>*>(lParam);
    vector<AO_Window>* windows = params->first;
    const bool isStreamliningMode = params->second;

    char title[256];
    char className[256];

    // 获取窗口标题
    GetWindowTextA(hwnd, title, sizeof(title));

    // 获取窗口类名
    GetClassNameA(hwnd, className, sizeof(className));

    // 获取窗口的可见性
    const bool isVisible = (IsWindowVisible(hwnd) != 0);

    // 获取窗口的最小化状态
    const bool isMinimized = (IsIconic(hwnd) != 0);

    // 获取窗口的最大化状态
    const bool isMaximized = (IsZoomed(hwnd) != 0);

    // 获取窗口的矩形区域
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    const bool isEmptyWindow = (strlen(title) == 0 || rect.right == rect.left || rect.top == rect.bottom);
    const bool isSubWindowOrToolWindow = (GetWindow(hwnd, GW_OWNER) || (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));

    if (isStreamliningMode && (isEmptyWindow || isSubWindowOrToolWindow))
    {
        return TRUE;  // 忽略该窗口，继续枚举下一个窗口
    }

    // 填充 AO_Window 结构体
    AO_Window window;
    window.hwnd = hwnd;
    window.title = title;
    window.className = className;
    window.rect = AO_Rect(AO_Point(rect.left, rect.top), AO_Point(rect.right, rect.bottom));
    window.isVisible = isVisible;
    window.isMinimized = isMinimized;
    window.isMaximized = isMaximized;

    // 将窗口信息添加到列表中
    windows->push_back(window);

    return TRUE;
}
vector<AO_Window> GetAllOpenWindows(bool isStreamliningMode)
{
    vector<AO_Window> windows;
    const pair<vector<AO_Window>*, bool> params = make_pair(&windows, isStreamliningMode);
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&params));
    return windows;
}

vector<AO_Window> GetWindows(const string& titleSubString, bool isConsiderUpperAndLower)
{
    const vector<AO_Window> allWindows = GetAllOpenWindows();
    vector<AO_Window> filteredWindows;
    filteredWindows.reserve(allWindows.size());

    string lowerSearchString = titleSubString;
    if (!isConsiderUpperAndLower)
    {
        transform(lowerSearchString.begin(), lowerSearchString.end(), lowerSearchString.begin(), ::tolower);
    }
    for (const AO_Window& window : allWindows)
    {
        string lowerTitle = window.title;
        if (!isConsiderUpperAndLower)
        {
            transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
        }
        if (lowerTitle.find(lowerSearchString) != string::npos)
        {
            filteredWindows.push_back(window);
        }
    }
    filteredWindows.shrink_to_fit();
    return filteredWindows;
}

void SetWindowVisibility(AO_Window& window, bool isVisible)
{
    ShowWindow(window.hwnd, isVisible ? SW_SHOW : SW_HIDE);
    window.isVisible = isVisible;
}

void MinimizeWindow(AO_Window& window)
{
    ShowWindow(window.hwnd, SW_MINIMIZE);
    window.isMinimized = true;
}

void MaximizeWindow(AO_Window& window)
{
    ShowWindow(window.hwnd, SW_MAXIMIZE);
    window.isMaximized = true;
}

void RestoreWindow(AO_Window& window)
{
    ShowWindow(window.hwnd, SW_RESTORE);
    window.isMinimized = false;
    window.isMaximized = false;
}

void MoveWindow(AO_Window& window, int newLeftTopX, int newLeftTopY)
{
    const int left = newLeftTopX;
    const int top = newLeftTopY;
    const int width = window.rect.width;
    const int height = window.rect.height;
    MoveWindow(window.hwnd, left, top, width, height, TRUE);

    window.rect = AO_Rect(newLeftTopX, newLeftTopY, width, height);
}
void MoveWindow(AO_Window& window, const AO_Point& newLeftTopPoint)
{
    MoveWindow(window, newLeftTopPoint.x, newLeftTopPoint.y);
}

void ResizeWindow(AO_Window& window, int newWidth, int newHeight)
{
    const int left = window.rect.leftTop.x;
    const int top = window.rect.leftTop.y;
    MoveWindow(window.hwnd, left, top, newWidth, newHeight, TRUE);
    window.rect = AO_Rect(window.rect.leftTop, newWidth, newHeight);
}

void MoveAndResizeWindow(AO_Window& window, const AO_Rect& newRect)
{
    const int left = newRect.leftTop.x;
    const int top = newRect.leftTop.y;
    const int width = newRect.width;
    const int height = newRect.height;

    MoveWindow(window.hwnd, left, top, width, height, TRUE);

    window.rect = newRect;
}
void MoveAndResizeWindow(AO_Window& window, const AO_Point& newLeftTopPoint, const AO_Point& newRightBottomPoint)
{
    MoveAndResizeWindow(window, AO_Rect(newLeftTopPoint, newRightBottomPoint));
}
void MoveAndResizeWindow(AO_Window& window, const AO_Point& newLeftTopPoint, int newWidth, int newHeight)
{
    MoveAndResizeWindow(window, AO_Rect(newLeftTopPoint, newWidth, newHeight));
}
void MoveAndResizeWindow(AO_Window& window, int newLeftTopX, int newLeftTopY, int newWidth, int newHeight)
{
    MoveAndResizeWindow(window, AO_Rect(newLeftTopX, newLeftTopY, newWidth, newHeight));
}

void BringWindowToFront(AO_Window& window)
{
    SetForegroundWindow(window.hwnd);
}

void SendWindowToBack(AO_Window& window)
{
    SetWindowPos(window.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

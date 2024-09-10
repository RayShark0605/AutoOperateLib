#include "../../AutoOperateLib/Base.h"
#include <iostream>
#include <unordered_set>
#include <sstream>

using namespace std;

AO_IniContent config;
const string configFilePath = "config.ini";
string currentProcessName;
unordered_set<HWND> hidenWindowsHWND;

bool ReadConfig()
{
	if (!IsFileExists(configFilePath))
	{
		config = AO_IniContent();
		MessageBox(nullptr, L"√ª”–’“µΩconfig.ini£°", L"¥ÌŒÛ", MB_ICONSTOP | MB_OK);
		return false;
	}
	config = ReadIniFile(configFilePath);
	return true;
}

vector<AO_Window> GetHidenWindows()
{
	const vector<string> hidenProcessSubName = StringSplit(config["config"]["hidenProcessSubName"], ';');
	const vector<string> hidenWindowSubName = StringSplit(config["config"]["hidenWindowSubName"], ';');
	const vector<string> showProcessSubName = StringSplit(config["config"]["showProcessSubName"], ';');
	const vector<string> showWindowsSubName = StringSplit(config["config"]["showWindowsSubName"], ';');

	vector<AO_Window> hidenWindows;
	for (const string& processSubName : hidenProcessSubName)
	{
		if (processSubName.empty())
		{
			continue;
		}
		const vector<AO_Window> temp = GetProcessWindows(processSubName);
		hidenWindows.insert(hidenWindows.end(), temp.begin(), temp.end());
	}
	for (const string& windowSubName : hidenWindowSubName)
	{
		if (windowSubName.empty())
		{
			continue;
		}
		const vector<AO_Window> temp = GetWindows(windowSubName);
		hidenWindows.insert(hidenWindows.end(), temp.begin(), temp.end());
	}
	if (!currentProcessName.empty())
	{
		const vector<AO_Window> temp = GetWindows(currentProcessName);
		hidenWindows.insert(hidenWindows.end(), temp.begin(), temp.end());
	}
	return hidenWindows;
}

vector<AO_Window> GetShownWindows()
{
	const vector<string> showProcessSubName = StringSplit(config["config"]["showProcessSubName"], ';');
	const vector<string> showWindowsSubName = StringSplit(config["config"]["showWindowsSubName"], ';');

	vector<AO_Window> shownWindows;
	for (const string& processSubName : showProcessSubName)
	{
		if (processSubName.empty())
		{
			continue;
		}
		const vector<AO_Window> temp = GetProcessWindows(processSubName);
		shownWindows.insert(shownWindows.end(), temp.begin(), temp.end());
	}
	for (const string& windowSubName : showWindowsSubName)
	{
		if (windowSubName.empty())
		{
			continue;
		}
		const vector<AO_Window> temp = GetWindows(windowSubName);
		shownWindows.insert(shownWindows.end(), temp.begin(), temp.end());
	}
	return shownWindows;
}

void Alt1Func()
{
	if (!ReadConfig())
	{
		return;
	}

	vector<AO_Window> hidenWindows = GetHidenWindows();
	vector<AO_Window> shownWindows = GetShownWindows();

	hidenWindowsHWND.clear();
	for (AO_Window& window : hidenWindows)
	{
		SetWindowVisibility(window, false);
		hidenWindowsHWND.insert(window.hwnd);
	}
	for (AO_Window& window : shownWindows)
	{
		SetWindowVisibility(window, true);
		BringWindowToFront(window);
	}
}

void Alt2Func()
{
	vector<AO_Window> hidenWindows = GetHidenWindows();

	for (AO_Window& window : hidenWindows)
	{
		SetWindowVisibility(window, true);
		hidenWindowsHWND.erase(window.hwnd);
	}
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_LOGOFF_EVENT || signal == CTRL_SHUTDOWN_EVENT)
	{
		for (HWND hwnd : hidenWindowsHWND)
		{
			ShowWindow(hwnd, SW_SHOW);
		}
	}
	return TRUE;
}

int main(int argc, char* argv[])
{
	if (!ReadConfig())
	{
		return 0;
	}

	currentProcessName = GetCurrentProcessName(false);

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	AO_HotkeyManager hotkeyManager;
	hotkeyManager.RegisterHotkey(1, MOD_ALT, '1', []()
		{
			Alt1Func();
		});
	hotkeyManager.RegisterHotkey(2, MOD_ALT, '2', []()
		{
			Alt2Func();
		});

	while (true)
	{
		WaitForS(10);
	}
	
	return 0;
}

#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>


using namespace std;

int main(int argc, char* argv[])
{
    AO_HotkeyManager s;
    s.RegisterHotkey(1, MOD_CONTROL, 'V', []() {
        MessageBox(nullptr, L"Ctrl+V pressed!", L"Hotkey Triggered", MB_OK);
        });

    WaitForS(10);

    s.UnregisterHotkey(1);
    cout << "反注册" << endl;

    WaitForS(5);

    return 0;


}









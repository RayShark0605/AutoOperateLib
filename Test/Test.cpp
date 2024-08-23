#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>


using namespace std;

int main(int argc, char* argv[])
{
    vector<AO_Window> windows = GetWindows("autooperate");
    WaitForS(5);
    SendWindowToBack(windows[1]);
    WaitForS(10);
    return 0;
}









#include <thread>

#include "MainWindow.h"
#include <QApplication>
using namespace std;

void StartClick()
{
    int a = 3;
    a++;
    return;
}
AO_HotkeyManager* hotkeyManager = nullptr;

void InitialFunc()
{
    hotkeyManager = new AO_HotkeyManager();
    hotkeyManager->RegisterHotkey(1, MOD_CONTROL, '1', []() {StartClick(); });
    WaitForS(9999);
}

int main(int argc, char* argv[])
{
    /*thread initialThread(InitialFunc);
    initialThread.detach();
    

    WaitForS(20);

    return 0;*/



    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
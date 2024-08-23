#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>


using namespace std;

int main(int argc, char* argv[])
{
    vector<AO_ActionRecord> records;

    cout << "三秒后开始记录！" << endl;
    WaitForS(2);

    {
        AO_ActionRecorder recorder;
        cout << "开始记录..." << endl;
        recorder.StartRecording();
        WaitForS(12);
        cout << "结束记录" << endl;
        recorder.StopRecording();
        records = recorder.GetRecords();
    }
    

    cout << "两秒后开始模拟！" << endl;
    WaitForS(2);

    AO_ActionSimulator simulator;
    cout << "开始模拟！" << endl;
    simulator.Start(records);
    simulator.WaitForEnd();
    cout << "模拟结束！" << endl;

    return 0;
}









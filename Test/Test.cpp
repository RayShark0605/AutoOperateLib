#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>


using namespace std;

int main(int argc, char* argv[])
{
    AO_ActionRecorder recorder;

    cout << "开始" << endl;
    recorder.StartRecording();
    WaitForS(8);
    cout << "结束" << endl;
    recorder.StopRecording();
    const vector<AO_ActionRecord> records = recorder.GetRecords();





    return 0;
}









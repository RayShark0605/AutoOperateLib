#include "../AutoOperateLib/CV.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    AO_Rect position;
    vector<AO_MonitorInfo> monitors = GetMonitorsInfo();
    const bool a = AO_OCR::IsScreenExistsWords({ "工具" }, position, AO_Rect(0, 0, 1058, 102));



    return 0;
}








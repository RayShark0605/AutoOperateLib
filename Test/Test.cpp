#include "../AutoOperateLib/CV.h"
#include <iostream>


#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>


#include <stdio.h>
#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <memory>

using namespace std;

const std::string APIKey = "";
const std::string secrectKey = "";

int main(int argc, char* argv[])
{
    AO_OCR::AO_BaiduOCR ocr(APIKey, secrectKey, AO_OCR::AO_BaiduOCR::GENERAL_POSITION);
    vector<AO_MonitorInfo> monitors = GetMonitorsInfo();

    const size_t times = 50;
    AO_Timer timer;
    double averageTime = 0;
    for (int i = 0; i < times; i++)
    {
        timer.Start();
        AO_OCR::AO_BaiduOCR::Result result = ocr.DetectFromScreen(monitors[0]);
        const double time = timer.ElapsedMilliseconds();
        timer.Stop();
        averageTime += time;
        cout << result.num << endl;
    }
    averageTime /= times;
    cout << "平均耗时：" << averageTime << "ms" << endl;

    return 0;
}








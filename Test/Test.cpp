#include "../AutoOperateLib/CV.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    const AO_Color targetColor = GetScreenPixelColor(131, 1056);

    AO_Point position;
    const vector<AO_Point> points = ScreenFindColor(targetColor, AO_Rect(20, 500, 1920, 800), 1);


    return 0;
}








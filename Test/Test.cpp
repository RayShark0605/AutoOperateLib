#include "../AutoOperateLib/Base.h"
#include <iostream>


using namespace std;


int main(int argc, char* argv[])
{
    // 定义搜索区域 (从左上角坐标(100, 100)到右下角坐标(500, 500)的区域)
    const AO_Rect area(100, 100, 400, 400);
    DrawRectangleOnScreen(area, 3000, false); // 在屏幕上绘制空心矩形，持续3秒
    WaitForS(5);

    return 0;
}








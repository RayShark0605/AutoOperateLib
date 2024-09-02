#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    const cv::Mat image = cv::imread("C:/Users/localuser/Desktop/image.bmp");
    const cv::Mat targetImage = cv::imread("C:/Users/localuser/Desktop/target2.jpg");
    AO_Rect rect;
    const bool success = FindImage(image, targetImage, rect);
    return 0;
}









#pragma once
#include "Base.h"

#include <opencv2/opencv.hpp>


// 在屏幕指定区域截图并导出为OpenCV影像格式
cv::Mat CaptureScreenToCvMat(const AO_Rect& rect);
cv::Mat CaptureScreenToCvMat(const AO_MonitorInfo& monitor); // 截取某个显示器的全部内容

// 从image中查找targetImage并要求置信度在confidence以上，返回值rect为在confidence以上的最高置信度所在的矩形。如果查找不到则返回false
// confidence的范围是 0 ~ 1
// 使用的是OpenCV的matchTemplate方法，对影像的缩放没有抵抗能力
bool FindImage(const cv::Mat& image, const cv::Mat& targetImage, AO_Rect& rect, double confidence = 0.85);

// OCR文字提取
namespace AO_OCR
{
	// 百度OCR API，每次约1s
	class AO_BaiduOCR
	{
	public:
		// 百度OCR API
		enum AO_BaiduOCRType
		{
			GENERAL,                 // 通用文字识别（标准版）
			GENERAL_POSITION,        // 通用文字识别（标准含位置版）
			HIGHPRECESION,           // 通用文字识别（高精度版）
			HIGHPRECESION_POSITION   // 通用文字识别（高精度含位置版）
		};

		AO_BaiduOCR(const std::string& APIKey, const std::string& secrectKey, AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION);

		void SetOnlyEnglish(bool isOnlyEnglish);
		void SetDetectDirection(bool isDetectDirection);

		struct Probability				// 置信度
		{
			double average = 0;
			double min = 0;
			double variance = 0;
			Probability();
		};
		struct Word						// 单词
		{
			std::string words = "";		// 单词内容
			Probability probability;	// 单词置信度
			AO_Rect location;			// 单词位置的最小包络框
		};
		struct Result					// OCR结果
		{
			std::vector<Word> wordsResult;
			unsigned int num = 0;			// 单词数
			size_t logID = 0;				// 唯一ID
			std::string errorMessage = "";	// 错误信息
			int errorCode = 0;				// 错误代码
		};

		Result DetectFromScreen(const AO_Rect& rect);					// 从全局屏幕中某个矩形范围内截取图片进行识别
		Result DetectFromScreen(const AO_MonitorInfo& monitor);			// 截取某个显示器的全部内容进行识别
		Result DetectFromImageFile(const std::string& imagePath);		// 读取某个图片文件进行识别

	private:
		AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION;
		std::string APIKey = "";
		std::string secrectKey = "";
		bool isOnlyEnglish = false;
		bool isDetectDirection = false;
	};



}


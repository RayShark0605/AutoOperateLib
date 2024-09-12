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
	// OCR文字识别基类
	class AO_IOCR
	{
	};

	// 百度OCR API
	class AO_BaiduOCR : public AO_IOCR
	{
	public:
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
			std::string content = "";	// 单词内容
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

		Result DetectFromScreen(const AO_Rect& rect) const;					// 从全局屏幕中某个矩形范围内截取图片进行识别
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// 截取某个显示器的全部内容进行识别
		Result DetectFromImageFile(const std::string& imagePath) const;		// 读取某个图片文件进行识别

	private:
		const AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION;
		const std::string APIKey = "";
		const std::string secrectKey = "";
		bool isOnlyEnglish = false;
		bool isDetectDirection = false;
	};

	// 阿里OCR API
	class AO_AliOCR : public AO_IOCR
	{
	public:
		AO_AliOCR(const std::string& appCode);

		struct Word
		{
			std::string content = "";
			double probability = 0;
			AO_Rect location;
			int rectAngle = 0;
		};
		struct Result
		{
			std::string requestID = "";
			std::vector<Word> wordsResult;
			bool isSuccess = false;
		};

		Result DetectFromScreen(const AO_Rect& rect) const;					// 从全局屏幕中某个矩形范围内截取图片进行识别
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// 截取某个显示器的全部内容进行识别
		Result DetectFromImageFile(const std::string& imagePath) const;		// 读取某个图片文件进行识别

	private:
		const std::string appCode = "";
	};

	// 腾讯OCR API
	class AO_TencentOCR
	{
	public:
		AO_TencentOCR(const std::string& secrectID, const std::string& secrectKey);
		
		struct Word
		{
			std::string content = "";
			double probability = 0;
			AO_Rect location;
		};
		struct Result
		{
			std::string requestID = "";
			std::vector<Word> wordsResult;
		};

		Result DetectFromScreen(const AO_Rect& rect) const;					// 从全局屏幕中某个矩形范围内截取图片进行识别
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// 截取某个显示器的全部内容进行识别
		Result DetectFromImageFile(const std::string& imagePath) const;		// 读取某个图片文件进行识别

	private:
		const std::string secrectID = "";
		const std::string secrectKey = "";
	};

	// OCR引擎管理者
	class AO_OCREngineManager
	{
	public:
		static bool IsReady();
		static void Initialize(const std::string& configIniFilePath = "ocr_config.ini");

		static void SetBaiduOCR(AO_BaiduOCR* baiduOCR);
		static AO_BaiduOCR* GetBaiduOCR();

		static void SetAliOCR(AO_AliOCR* aliOCR);
		static AO_AliOCR* GetAliOCR();

		static void SetTencentOCR(AO_TencentOCR* tencentOCR);
		static AO_TencentOCR* GetTencentOCR();


	private:
		static AO_BaiduOCR* baiduOCR;
		static AO_AliOCR* aliOCR;
		static AO_TencentOCR* tencentOCR;

		AO_OCREngineManager();
		~AO_OCREngineManager();
		AO_OCREngineManager(const AO_OCREngineManager&) = delete;
		AO_OCREngineManager& operator=(const AO_OCREngineManager&) = delete;
	};

	bool IsScreenExistsWords(const std::vector<std::string>& words, AO_Rect& wordPosition, const AO_Rect& screenArea, double minProbability = 0, const std::string& configIniFilePath = "ocr_config.ini");
}



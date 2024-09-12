#pragma once
#include "Base.h"

#include <opencv2/opencv.hpp>


// ����Ļָ�������ͼ������ΪOpenCVӰ���ʽ
cv::Mat CaptureScreenToCvMat(const AO_Rect& rect);
cv::Mat CaptureScreenToCvMat(const AO_MonitorInfo& monitor); // ��ȡĳ����ʾ����ȫ������

// ��image�в���targetImage��Ҫ�����Ŷ���confidence���ϣ�����ֵrectΪ��confidence���ϵ�������Ŷ����ڵľ��Ρ�������Ҳ����򷵻�false
// confidence�ķ�Χ�� 0 ~ 1
// ʹ�õ���OpenCV��matchTemplate��������Ӱ�������û�еֿ�����
bool FindImage(const cv::Mat& image, const cv::Mat& targetImage, AO_Rect& rect, double confidence = 0.85);

// OCR������ȡ
namespace AO_OCR
{
	// OCR����ʶ�����
	class AO_IOCR
	{
	};

	// �ٶ�OCR API
	class AO_BaiduOCR : public AO_IOCR
	{
	public:
		enum AO_BaiduOCRType
		{
			GENERAL,                 // ͨ������ʶ�𣨱�׼�棩
			GENERAL_POSITION,        // ͨ������ʶ�𣨱�׼��λ�ð棩
			HIGHPRECESION,           // ͨ������ʶ�𣨸߾��Ȱ棩
			HIGHPRECESION_POSITION   // ͨ������ʶ�𣨸߾��Ⱥ�λ�ð棩
		};

		AO_BaiduOCR(const std::string& APIKey, const std::string& secrectKey, AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION);

		void SetOnlyEnglish(bool isOnlyEnglish);
		void SetDetectDirection(bool isDetectDirection);

		struct Probability				// ���Ŷ�
		{
			double average = 0;
			double min = 0;
			double variance = 0;
			Probability();
		};
		struct Word						// ����
		{
			std::string content = "";	// ��������
			Probability probability;	// �������Ŷ�
			AO_Rect location;			// ����λ�õ���С�����
		};
		struct Result					// OCR���
		{
			std::vector<Word> wordsResult;
			unsigned int num = 0;			// ������
			size_t logID = 0;				// ΨһID
			std::string errorMessage = "";	// ������Ϣ
			int errorCode = 0;				// �������
		};

		Result DetectFromScreen(const AO_Rect& rect) const;					// ��ȫ����Ļ��ĳ�����η�Χ�ڽ�ȡͼƬ����ʶ��
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// ��ȡĳ����ʾ����ȫ�����ݽ���ʶ��
		Result DetectFromImageFile(const std::string& imagePath) const;		// ��ȡĳ��ͼƬ�ļ�����ʶ��

	private:
		const AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION;
		const std::string APIKey = "";
		const std::string secrectKey = "";
		bool isOnlyEnglish = false;
		bool isDetectDirection = false;
	};

	// ����OCR API
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

		Result DetectFromScreen(const AO_Rect& rect) const;					// ��ȫ����Ļ��ĳ�����η�Χ�ڽ�ȡͼƬ����ʶ��
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// ��ȡĳ����ʾ����ȫ�����ݽ���ʶ��
		Result DetectFromImageFile(const std::string& imagePath) const;		// ��ȡĳ��ͼƬ�ļ�����ʶ��

	private:
		const std::string appCode = "";
	};

	// ��ѶOCR API
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

		Result DetectFromScreen(const AO_Rect& rect) const;					// ��ȫ����Ļ��ĳ�����η�Χ�ڽ�ȡͼƬ����ʶ��
		Result DetectFromScreen(const AO_MonitorInfo& monitor) const;		// ��ȡĳ����ʾ����ȫ�����ݽ���ʶ��
		Result DetectFromImageFile(const std::string& imagePath) const;		// ��ȡĳ��ͼƬ�ļ�����ʶ��

	private:
		const std::string secrectID = "";
		const std::string secrectKey = "";
	};

	// OCR���������
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



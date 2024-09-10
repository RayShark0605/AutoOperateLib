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
	// �ٶ�OCR API��ÿ��Լ1s
	class AO_BaiduOCR
	{
	public:
		// �ٶ�OCR API
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
			std::string words = "";		// ��������
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

		Result DetectFromScreen(const AO_Rect& rect);					// ��ȫ����Ļ��ĳ�����η�Χ�ڽ�ȡͼƬ����ʶ��
		Result DetectFromScreen(const AO_MonitorInfo& monitor);			// ��ȡĳ����ʾ����ȫ�����ݽ���ʶ��
		Result DetectFromImageFile(const std::string& imagePath);		// ��ȡĳ��ͼƬ�ļ�����ʶ��

	private:
		AO_BaiduOCRType type = AO_BaiduOCRType::GENERAL_POSITION;
		std::string APIKey = "";
		std::string secrectKey = "";
		bool isOnlyEnglish = false;
		bool isDetectDirection = false;
	};



}


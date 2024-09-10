#include "CV.h"
#include <curl/curl.h>
#include <json/json.h>
using namespace std;

cv::Mat CaptureScreenToCvMat(const AO_Rect& rect)
{
    const HDC hScreenDC = GetDC(NULL);
    // 创建内存设备上下文
    const HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // 创建兼容位图
    const HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, rect.width, rect.height);
    // 选择位图到内存设备上下文中
    const HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // 截取屏幕到内存设备上下文
    BitBlt(hMemoryDC, 0, 0, rect.width, rect.height, hScreenDC, rect.leftTop.x, rect.leftTop.y, SRCCOPY);

    // 创建位图信息头
    BITMAPINFOHEADER bmi = { 0 };
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = rect.width;
    bmi.biHeight = -rect.height;  // 负高度表示将图像翻转过来
    bmi.biPlanes = 1;
    bmi.biBitCount = 24;  // 24位色深
    bmi.biCompression = BI_RGB;

    // 创建cv::Mat并从位图中获取数据
    cv::Mat mat(rect.height, rect.width, CV_8UC3);
    GetDIBits(hMemoryDC, hBitmap, 0, rect.height, mat.data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

    // 释放资源
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return mat;
}
cv::Mat CaptureScreenToCvMat(const AO_MonitorInfo& monitor)
{
    return CaptureScreenToCvMat(monitor.GetRect());
}

bool FindImage(const cv::Mat& image, const cv::Mat& targetImage, AO_Rect& rect, double confidence)
{
    if (image.empty() || targetImage.empty() || confidence < 0 || confidence > 1 || image.rows <= targetImage.rows || image.cols <= targetImage.cols)
    {
        return false;
    }

    // 结果矩阵
    cv::Mat result;
    const int resultCols = image.cols - targetImage.cols + 1;
    const int resultRows = image.rows - targetImage.rows + 1;
    result.create(resultRows, resultCols, CV_32FC1);

    // 使用 cv::matchTemplate 进行模板匹配
    cv::matchTemplate(image, targetImage, result, cv::TM_CCOEFF_NORMED);

    // 找到匹配结果中的最大值及其位置
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    if (maxVal >= confidence)
    {
        rect = AO_Rect(maxLoc.x, maxLoc.y, targetImage.cols, targetImage.rows);
        return true;
    }

    return false;
}

namespace AO_OCR
{
    namespace internal
    {
        size_t WriteDataFunc(void* buffer, size_t size, size_t nmemb, void* userp)
        {
            string* str = dynamic_cast<string*>((string*)userp);
            str->append((char*)buffer, size * nmemb);
            return nmemb;
        }
        string GetAccessToken(const string& APIKey, const string& secrectKey)
        {
            CURL* curl = curl_easy_init();
            if (!curl)
            {
                return "";
            }
            string accessToken = "";
            CURLcode res;
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_URL, "https://aip.baidubce.com/oauth/2.0/token");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
            curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
            headers = curl_slist_append(headers, "Accept: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            const string data = "grant_type=client_credentials&client_id=" + APIKey + "&client_secret=" + secrectKey;
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &accessToken);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataFunc);
            res = curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            Json::Value obj;
            string error;
            Json::CharReaderBuilder crbuilder;
            unique_ptr<Json::CharReader> reader(crbuilder.newCharReader());
            reader->parse(accessToken.data(), accessToken.data() + accessToken.size(), &obj, &error);
            return obj["access_token"].asString();
        }

        const static string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        string Base64Encode(unsigned char const* bytes, unsigned int len)
        {
            string ret = "";
            int i = 0;
            int j = 0;
            unsigned char charArray3[3];
            unsigned char charArray4[4];

            while (len--)
            {
                charArray3[i++] = *(bytes++);
                if (i == 3)
                {
                    charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                    charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                    charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                    charArray4[3] = charArray3[2] & 0x3f;

                    for (i = 0; i < 4; i++)
                    {
                        ret += base64Chars[charArray4[i]];
                    }
                    i = 0;
                }
            }

            if (i)
            {
                for (j = i; j < 3; j++)
                {
                    charArray3[j] = '\0';
                }

                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);

                for (j = 0; j < i + 1; j++)
                {
                    ret += base64Chars[charArray4[j]];
                }

                while (i++ < 3)
                {
                    ret += '=';
                }
            }
            return ret;
        }

        // 将cv::Mat对应的图片转为Base64编码
        string MatToBase64(const cv::Mat& image, bool isURLEncoded = false, const string& imageFormat = ".png")
        {
            vector<unsigned char> buf;
            cv::imencode(imageFormat, image, buf);  // 将cv::Mat编码为二进制数据

            // 将二进制数据进行Base64编码
            string base64String = Base64Encode(buf.data(), buf.size());

            // 如果需要URL编码
            if (isURLEncoded)
            {
                char* URLEncodedStr = curl_escape(base64String.c_str(), base64String.length());
                base64String = string(URLEncodedStr);
                curl_free(URLEncodedStr);
            }
            return base64String;
        }

        // UTF-8编码字符串转为GBK编码
        wstring UTF8ToWString(const string& str)
        {
            int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
            wstring wstr(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], len);
            return wstr;
        }
        string WStringToGBK(const wstring& wstr)
        {
            int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
            string str(len, '\0');
            WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.length(), &str[0], len, NULL, NULL);
            return str;
        }
        string UTF8ToGBK(const string& str)
        {
            wstring wstr = UTF8ToWString(str);
            return WStringToGBK(wstr);
        }

        // 读取文件得到Base64编码
        string GetFileBase64(const char* path, bool isURLEncoded = false)
        {
            string ret;
            int i = 0;
            int j = 0;
            unsigned char charArray3[3];
            unsigned char charArray4[4];
            const unsigned int bufferSize = 1024;
            unsigned char buffer[bufferSize];
            ifstream fileRead;
            fileRead.open(path, ios::binary);
            while (!fileRead.eof())
            {
                fileRead.read((char*)buffer, bufferSize * sizeof(char));
                int num = fileRead.gcount();
                int m = 0;
                while (num--)
                {
                    charArray3[i++] = buffer[m++];
                    if (i == 3)
                    {
                        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                        charArray4[3] = charArray3[2] & 0x3f;
                        for (i = 0; (i < 4); i++)
                        {
                            ret += base64Chars[charArray4[i]];
                        }
                        i = 0;
                    }
                }
            }
            fileRead.close();
            if (i)
            {
                for (j = i; j < 3; j++)
                {
                    charArray3[j] = '\0';
                }

                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] = charArray3[2] & 0x3f;
                for (j = 0; j < i + 1; j++)
                {
                    ret += base64Chars[charArray4[j]];
                }
                while ((i++ < 3))
                {
                    ret += '=';
                }
            }
            if (isURLEncoded)
            {
                ret = curl_escape(ret.c_str(), ret.length());
            }
            return ret;
        }

        // 解析API返回的信息
        AO_BaiduOCR::Result ParseOCRResult(const string& jsonStr)
        {
            AO_BaiduOCR::Result result;
            Json::CharReaderBuilder readerBuilder;
            Json::Value root;
            string errs;

            // 解析 JSON 字符串
            istringstream stream(jsonStr);
            if (!Json::parseFromStream(readerBuilder, stream, &root, &errs))
            {
                cerr << "Failed to parse JSON: " << errs << endl;
                return result;
            }

            result.logID = root["log_id"].asLargestInt();

            result.errorMessage = root["error_msg"].asString();
            if (!result.errorMessage.empty())
            {
                result.errorCode = root["error_code"].asInt();
                return result;
            }

            result.num = root["words_result_num"].asUInt();

            // 解析 words_result 数组
            const Json::Value& wordsResultArray = root["words_result"];
            for (const auto& item : wordsResultArray)
            {
                AO_BaiduOCR::Word wordsResult;

                // 解析 probability 对象
                const Json::Value& probability = item["probability"];
                wordsResult.probability.average = probability["average"].asDouble();
                wordsResult.probability.min = probability["min"].asDouble();
                wordsResult.probability.variance = probability["variance"].asDouble();

                // 解析 words 字符串
                wordsResult.words = item["words"].asString();

                // 解析 location 对象
                const Json::Value& location = item["location"];
                const unsigned int top = location["top"].asUInt();
                const unsigned int left = location["left"].asUInt();
                const unsigned int width = location["width"].asUInt();
                const unsigned int height = location["height"].asUInt();
                wordsResult.location = AO_Rect(left, top, width, height);
                result.wordsResult.push_back(wordsResult);
            }
            return result;
        }
    }
    AO_BaiduOCR::Probability::Probability():average(0), min(0), variance(0) {}

    AO_BaiduOCR::AO_BaiduOCR(const string& APIKey, const string& secrectKey, AO_BaiduOCRType type) :APIKey(APIKey), secrectKey(secrectKey), type(type)
    {
    }
    void AO_BaiduOCR::SetOnlyEnglish(bool isOnlyEnglish)
    {
        this->isOnlyEnglish = isOnlyEnglish;
    }
    void AO_BaiduOCR::SetDetectDirection(bool isDetectDirection)
    {
        this->isDetectDirection = isDetectDirection;
    }
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromScreen(const AO_Rect& rect)
    {
        const cv::Mat image = CaptureScreenToCvMat(rect);
        if (image.empty())
        {
            return AO_BaiduOCR::Result();
        }
        const string base64Data = internal::MatToBase64(image, true);

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return AO_BaiduOCR::Result();
        }

        const string accessToken = internal::GetAccessToken(APIKey, secrectKey);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        switch (type)
        {
        case AO_OCR::AO_BaiduOCR::GENERAL:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/general_basic?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::GENERAL_POSITION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/general?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::HIGHPRECESION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/accurate_basic?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::HIGHPRECESION_POSITION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/accurate?access_token=" + accessToken).c_str());
            break;
        default:
            break;
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "undefined");

        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        string data = "image=" + base64Data + "&probability=true";
        if (isOnlyEnglish)
        {
            data += "&language_type=ENG";
        }
        if (isDetectDirection)
        {
            data += "&detect_direction=true";
        }

        string resultString;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultString);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteDataFunc);
        const CURLcode res = curl_easy_perform(curl);
        resultString = internal::UTF8ToGBK(resultString);
        return internal::ParseOCRResult(resultString);
    }
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromScreen(const AO_MonitorInfo& monitor)
    {
        return DetectFromScreen(monitor.GetRect());
    }
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromImageFile(const string& imagePath)
    {
        if (!IsFileExists(imagePath))
        {
            return AO_BaiduOCR::Result();
        }

        const string base64Data = internal::GetFileBase64(imagePath.data(), true);

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return AO_BaiduOCR::Result();
        }

        const string accessToken = internal::GetAccessToken(APIKey, secrectKey);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        switch (type)
        {
        case AO_OCR::AO_BaiduOCR::GENERAL:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/general_basic?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::GENERAL_POSITION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/general?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::HIGHPRECESION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/accurate_basic?access_token=" + accessToken).c_str());
            break;
        case AO_OCR::AO_BaiduOCR::HIGHPRECESION_POSITION:
            curl_easy_setopt(curl, CURLOPT_URL, ("https://aip.baidubce.com/rest/2.0/ocr/v1/accurate?access_token=" + accessToken).c_str());
            break;
        default:
            break;
        }

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "undefined");

        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        string data = "image=" + base64Data + "&probability=true";
        if (isOnlyEnglish)
        {
            data += "&language_type=ENG";
        }
        if (isDetectDirection)
        {
            data += "&detect_direction=true";
        }

        string resultString;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultString);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteDataFunc);
        const CURLcode res = curl_easy_perform(curl);
        resultString = internal::UTF8ToGBK(resultString);
        return internal::ParseOCRResult(resultString);
    }


}

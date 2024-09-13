#include "CV.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <curl/curl.h>
#include <json/json.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
using namespace std;

cv::Mat CaptureScreenToCvMat(const AO_Rect& rect)
{
    const HDC hScreenDC = GetDC(NULL);
    const HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    const HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, rect.width, rect.height);
    const HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // 将屏幕数据复制到内存设备上下文中
    BitBlt(hMemoryDC, 0, 0, rect.width, rect.height, hScreenDC, rect.leftTop.x, rect.leftTop.y, SRCCOPY);

    // 设置位图信息
    BITMAPINFOHEADER bmi = { 0 };
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = rect.width;
    bmi.biHeight = -rect.height;  // 正常方向
    bmi.biPlanes = 1;
    bmi.biBitCount = 24;  // 24位RGB色
    bmi.biCompression = BI_RGB;

    // 为了避免数据对齐问题，我们使用临时的缓冲区来存储位图数据
    const int stride = ((rect.width * 3 + 3) & ~3);  // 行的字节数，4字节对齐
    vector<uchar> buffer(stride * rect.height);

    // 从内存设备上下文中获取位图数据
    GetDIBits(hMemoryDC, hBitmap, 0, rect.height, buffer.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

    // 创建cv::Mat，并将数据从缓冲区复制到mat中
    cv::Mat mat(rect.height, rect.width, CV_8UC3);

    // 复制数据，并进行RGB到BGR的转换
    for (int y = 0; y < rect.height; y++)
    {
        const uchar* srcRow = buffer.data() + y * stride;
        uchar* dstRow = mat.ptr<uchar>(y);
        for (int x = 0; x < rect.width; ++x)
        {
            dstRow[x * 3 + 2] = srcRow[x * 3 + 2];
            dstRow[x * 3 + 1] = srcRow[x * 3 + 1];
            dstRow[x * 3 + 0] = srcRow[x * 3 + 0];
        }
    }

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
            return size * nmemb;
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

        // 解析百度API返回的信息
        AO_BaiduOCR::Result ParseBaiduOCRResult(const string& jsonStr)
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
                AO_BaiduOCR::Word wordResult;

                // 解析 probability 对象
                const Json::Value& probability = item["probability"];
                wordResult.probability.average = probability["average"].asDouble();
                wordResult.probability.min = probability["min"].asDouble();
                wordResult.probability.variance = probability["variance"].asDouble();

                // 解析 words 字符串
                wordResult.content = item["words"].asString();

                // 解析 location 对象
                const Json::Value& location = item["location"];
                const unsigned int top = location["top"].asUInt();
                const unsigned int left = location["left"].asUInt();
                const unsigned int width = location["width"].asUInt();
                const unsigned int height = location["height"].asUInt();
                wordResult.location = AO_Rect(left, top, width, height);
                result.wordsResult.push_back(wordResult);
            }
            return result;
        }

        // 解析阿里API返回的信息
        AO_AliOCR::Result ParseAliOCRResult(const string& jsonStr)
        {
            AO_AliOCR::Result result;
            Json::CharReaderBuilder readerBuilder;
            Json::Value root;
            string errs;

            istringstream stream(jsonStr);
            if (!Json::parseFromStream(readerBuilder, stream, &root, &errs))
            {
                cerr << "Failed to parse JSON: " << errs << endl;
                return result;
            }

            result.requestID = root["request_id"].asString();
            result.isSuccess = root["success"].asBool();

            const Json::Value& retArray = root["ret"];
            for (const Json::Value& item : retArray)
            {
                AO_AliOCR::Word wordResult;

                wordResult.probability = item["prob"].asDouble();

                const Json::Value& location = item["rect"];
                wordResult.rectAngle = location["angle"].asInt();
                const int top = location["top"].asInt();
                const int left = location["left"].asInt();
                const int width = location["width"].asInt();
                const int height = location["height"].asInt();
                wordResult.location = AO_Rect(left, top, width, height);

                wordResult.content = item["word"].asString();

                result.wordsResult.push_back(wordResult);
            }
            return result;
        }

        // 解析腾讯API返回的信息
        AO_TencentOCR::Result ParseTencentOCRResult(const string& jsonStr)
        {
            AO_TencentOCR::Result result;
            Json::CharReaderBuilder readerBuilder;
            Json::Value root;
            string errs;

            istringstream stream(jsonStr);
            if (!Json::parseFromStream(readerBuilder, stream, &root, &errs))
            {
                cerr << "Failed to parse JSON: " << errs << endl;
                return result;
            }

            const Json::Value& responseValue = root["Response"];
            result.requestID = responseValue["RequestId"].asString();

            const Json::Value& textDetectionsArray = responseValue["TextDetections"];
            for (const auto& item : textDetectionsArray)
            {
                AO_TencentOCR::Word word;
                word.probability = item["Confidence"].asInt() * 0.01;
                word.content = item["DetectedText"].asString();

                const Json::Value& location = item["ItemPolygon"];
                const int X = location["X"].asInt();
                const int Y = location["Y"].asInt();
                const int width = location["Width"].asInt();
                const int height = location["Height"].asInt();
                word.location = AO_Rect(X, Y, width, height);

                result.wordsResult.push_back(word);
            }
            return result;
        }

        // 获取时间戳
        string GetTimeStamp(int64_t& timestamp)
        {
            string utcDate;
            char buff[20] = { 0 };
            struct tm sttime;
            sttime = *gmtime(&timestamp);
            strftime(buff, sizeof(buff), "%Y-%m-%d", &sttime);
            utcDate = string(buff);
            return utcDate;
        }

        string SHA256HEX(const string& str)
        {
            char buf[3];
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256_CTX sha256;
            SHA256_Init(&sha256);
            SHA256_Update(&sha256, str.c_str(), str.size());
            SHA256_Final(hash, &sha256);
            string newString = "";
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            {
                snprintf(buf, sizeof(buf), "%02x", hash[i]);
                newString = newString + buf;
            }
            return newString;
        }

        string Int2Str(int64_t n)
        {
            stringstream ss;
            ss << n;
            return ss.str();
        }

        string HmacSHA256(const string& key, const string& input)
        {
            unsigned char hash[32];

            HMAC_CTX* h;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            HMAC_CTX hmac;
            HMAC_CTX_init(&hmac);
            h = &hmac;
#else
            h = HMAC_CTX_new();
#endif

            HMAC_Init_ex(h, &key[0], key.length(), EVP_sha256(), NULL);
            HMAC_Update(h, (unsigned char*)&input[0], input.length());
            unsigned int len = 32;
            HMAC_Final(h, hash, &len);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
            HMAC_CTX_cleanup(h);
#else
            HMAC_CTX_free(h);
#endif

            stringstream ss;
            ss << setfill('0');
            for (int i = 0; i < len; i++)
            {
                ss << hash[i];
            }

            return (ss.str());
        }

        string HexEncode(const string& input)
        {
            static const char* const lut = "0123456789abcdef";
            size_t len = input.length();

            string output;
            output.reserve(2 * len);
            for (size_t i = 0; i < len; i++)
            {
                const unsigned char c = input[i];
                output.push_back(lut[c >> 4]);
                output.push_back(lut[c & 15]);
            }
            return output;
        }

        struct WriteData
        {
            string response;
        };
        size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
        {
            size_t totalSize = size * nmemb;
            WriteData* data = static_cast<WriteData*>(userdata);
            data->response.append(ptr, totalSize);
            return totalSize;
        }

        bool IsContainWords(const string& word, const vector<string> targets)
        {
            for (const string& target : targets)
            {
                if (word.size() < target.size())
                {
                    continue;
                }
                if (word.find(target) != string::npos)
                {
                    return true;
                }
            }
            return false;
        }

        void GetRotatedCorners(const AO_Rect& rect, AO_Point corners[4], double angle)
        {
            const double angleRad = angle * M_PI / 180.0;

            const double cosTheta = cos(angleRad);
            const double sinTheta = sin(angleRad);

            
            const AO_Point tl = rect.leftTop;
            const AO_Point tr(tl.x + rect.width, tl.y);
            const AO_Point bl(tl.x, tl.y + rect.height);
            const AO_Point br(tl.x + rect.width, tl.y + rect.height);

            corners[0] = AO_Point(tl.x * cosTheta - tl.y * sinTheta, tl.x * sinTheta + tl.y * cosTheta);
            corners[1] = AO_Point(tr.x * cosTheta - tr.y * sinTheta, tr.x * sinTheta + tr.y * cosTheta);
            corners[2] = AO_Point(bl.x * cosTheta - bl.y * sinTheta, bl.x * sinTheta + bl.y * cosTheta);
            corners[3] = AO_Point(br.x * cosTheta - br.y * sinTheta, br.x * sinTheta + br.y * cosTheta);
        }
        AO_Rect GetRotatedRectBoundingBox(const AO_Rect& rect, double angle)
        {
            AO_Point corners[4];
            GetRotatedCorners(rect, corners, angle);

            const double minX = min({ corners[0].x, corners[1].x, corners[2].x, corners[3].x });
            const double maxX = max({ corners[0].x, corners[1].x, corners[2].x, corners[3].x });
            const double minY = min({ corners[0].y, corners[1].y, corners[2].y, corners[3].y });
            const double maxY = max({ corners[0].y, corners[1].y, corners[2].y, corners[3].y });

            AO_Rect boundingBox;
            boundingBox.leftTop = AO_Point(minX, minY);
            boundingBox.width = maxX - minX;
            boundingBox.height = maxY - minY;

            return boundingBox;
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
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromScreen(const AO_Rect& rect) const
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
        return internal::ParseBaiduOCRResult(resultString);
    }
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromScreen(const AO_MonitorInfo& monitor) const
    {
        return DetectFromScreen(monitor.GetRect());
    }
    AO_BaiduOCR::Result AO_BaiduOCR::DetectFromImageFile(const string& imagePath) const
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
        return internal::ParseBaiduOCRResult(resultString);
    }

    AO_AliOCR::AO_AliOCR(const string& appCode) :appCode(appCode)
    {
    }
    AO_AliOCR::Result AO_AliOCR::DetectFromScreen(const AO_Rect& rect) const
    {
        const cv::Mat image = CaptureScreenToCvMat(rect);
        if (image.empty())
        {
            return AO_AliOCR::Result();
        }
        const string base64Data = internal::MatToBase64(image);

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return AO_AliOCR::Result();
        }
        const string url = "https://tysbgpu.market.alicloudapi.com/api/predict/ocr_general";
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: APPCODE " + appCode).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");

        const string bodys = "{\"image\":\"" + base64Data + "\",\"configure\":{\"output_prob\":true,\"output_keypoints\":false,\"skip_detection\":false,\"without_predicting_direction\":false}}";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodys.c_str());

        string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteDataFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        const CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            return AO_AliOCR::Result();
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        responseString = internal::UTF8ToGBK(responseString);
        return internal::ParseAliOCRResult(responseString);
    }
    AO_AliOCR::Result AO_AliOCR::DetectFromScreen(const AO_MonitorInfo& monitor) const
    {
        return DetectFromScreen(monitor.GetRect());
    }
    AO_AliOCR::Result AO_AliOCR::DetectFromImageFile(const string& imagePath) const
    {
        if (!IsFileExists(imagePath))
        {
            return AO_AliOCR::Result();
        }

        const string base64Data = internal::GetFileBase64(imagePath.data());
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return AO_AliOCR::Result();
        }
        const string url = "https://tysbgpu.market.alicloudapi.com/api/predict/ocr_general";
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: APPCODE " + appCode).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");

        const string bodys = "{\"image\":\"" + base64Data + "\",\"configure\":{\"output_prob\":true,\"output_keypoints\":false,\"skip_detection\":false,\"without_predicting_direction\":false}}";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodys.c_str());

        string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteDataFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        const CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            return AO_AliOCR::Result();
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        responseString = internal::UTF8ToGBK(responseString);
        return internal::ParseAliOCRResult(responseString);
    }

    AO_TencentOCR::AO_TencentOCR(const string& secrectID, const string& secrectKey) :secrectID(secrectID), secrectKey(secrectKey)
    {
    }
    AO_TencentOCR::Result AO_TencentOCR::DetectFromScreen(const AO_Rect& rect) const
    {
        const cv::Mat image = CaptureScreenToCvMat(rect);
        if (image.empty())
        {
            return AO_TencentOCR::Result();
        }
        const string base64Data = internal::MatToBase64(image);

        string TOKEN = "";
        const string service = "ocr";
        const string host = "ocr.tencentcloudapi.com";
        const string region = "ap-shanghai";
        const string action = "GeneralAccurateOCR";
        const string version = "2018-11-19";

        int64_t timestamp = time(nullptr);
        const string date = internal::GetTimeStamp(timestamp);

        // ************* 步骤 1：拼接规范请求串 *************
        const string httpRequestMethod = "POST";
        const string canonicalUri = "/";
        const string canonicalQueryString = "";
        const string canonicalHeaders = "content-type:application/json; charset=utf-8\nhost:" + host + "\n";
        const string signedHeaders = "content-type;host";

        string payload = "{\"ImageBase64\":\"";
        payload += base64Data;
        payload += "\"}";

        const string hashedRequestPayload = internal::SHA256HEX(payload);
        const string canonicalRequest = httpRequestMethod + "\n" + canonicalUri + "\n" + canonicalQueryString + "\n" + canonicalHeaders + "\n" + signedHeaders + "\n" + hashedRequestPayload;

        // ************* 步骤 2：拼接待签名字符串 *************
        const string algorithm = "TC3-HMAC-SHA256";
        const string RequestTimestamp = internal::Int2Str(timestamp);
        const string credentialScope = date + "/" + service + "/" + "tc3_request";
        const string hashedCanonicalRequest = internal::SHA256HEX(canonicalRequest);
        const string stringToSign = algorithm + "\n" + RequestTimestamp + "\n" + credentialScope + "\n" + hashedCanonicalRequest;

        // ************* 步骤 3：计算签名 *************
        const string kKey = "TC3" + secrectKey;
        const string kDate = internal::HmacSHA256(kKey, date);
        const string kService = internal::HmacSHA256(kDate, service);
        const string kSigning = internal::HmacSHA256(kService, "tc3_request");
        const string signature = internal::HexEncode(internal::HmacSHA256(kSigning, stringToSign));

        // ************* 步骤 4：拼接 Authorization *************
        const string authorization = algorithm + " " + "Credential=" + secrectID + "/" + credentialScope + ", " + "SignedHeaders=" + signedHeaders + ", " + "Signature=" + signature;
        const string url = "https://" + host;
        const string authorizationHeader = "Authorization: " + authorization;
        const string hostHeader = "Host: " + host;
        const string actionHeader = "X-TC-Action: " + action;
        const string timestampHeader = "X-TC-Timestamp: " + RequestTimestamp;
        const string versionHeader = "X-TC-Version: " + version;
        const string regionHeader = "X-TC-Region: " + region;
        const string tokenHeader = "X-TC-Token: " + TOKEN;

        internal::WriteData resData;

        // ************* 步骤 5：构造并发起请求 *************
        CURL* curl = curl_easy_init();
        CURLcode res;
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_URL, url.data());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);

            curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, authorizationHeader.data());
            headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
            headers = curl_slist_append(headers, hostHeader.data());
            headers = curl_slist_append(headers, actionHeader.data());
            headers = curl_slist_append(headers, timestampHeader.data());
            headers = curl_slist_append(headers, versionHeader.data());
            headers = curl_slist_append(headers, regionHeader.data());
            headers = curl_slist_append(headers, tokenHeader.data());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            const char* data = payload.data();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resData);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res == CURLE_OK)
            {
                resData.response = internal::UTF8ToGBK(resData.response);
                return internal::ParseTencentOCRResult(resData.response);
            }
        }
        curl_easy_cleanup(curl);
        return AO_TencentOCR::Result();
    }
    AO_TencentOCR::Result AO_TencentOCR::DetectFromScreen(const AO_MonitorInfo& monitor) const
    {
        return DetectFromScreen(monitor.GetRect());
    }
    AO_TencentOCR::Result AO_TencentOCR::DetectFromImageFile(const string& imagePath) const
    {
        if (!IsFileExists(imagePath))
        {
            return AO_TencentOCR::Result();
        }
        string TOKEN = "";
        const string service = "ocr";
        const string host = "ocr.tencentcloudapi.com";
        const string region = "ap-shanghai";
        const string action = "GeneralAccurateOCR";
        const string version = "2018-11-19";

        int64_t timestamp = time(nullptr);
        const string date = internal::GetTimeStamp(timestamp);

        // ************* 步骤 1：拼接规范请求串 *************
        const string httpRequestMethod = "POST";
        const string canonicalUri = "/";
        const string canonicalQueryString = "";
        const string canonicalHeaders = "content-type:application/json; charset=utf-8\nhost:" + host + "\n";
        const string signedHeaders = "content-type;host";

        string payload = "{\"ImageBase64\":\"";
        payload += internal::GetFileBase64(imagePath.data());
        payload += "\"}";

        const string hashedRequestPayload = internal::SHA256HEX(payload);
        const string canonicalRequest = httpRequestMethod + "\n" + canonicalUri + "\n" + canonicalQueryString + "\n" + canonicalHeaders + "\n" + signedHeaders + "\n" + hashedRequestPayload;

        // ************* 步骤 2：拼接待签名字符串 *************
        const string algorithm = "TC3-HMAC-SHA256";
        const string RequestTimestamp = internal::Int2Str(timestamp);
        const string credentialScope = date + "/" + service + "/" + "tc3_request";
        const string hashedCanonicalRequest = internal::SHA256HEX(canonicalRequest);
        const string stringToSign = algorithm + "\n" + RequestTimestamp + "\n" + credentialScope + "\n" + hashedCanonicalRequest;

        // ************* 步骤 3：计算签名 *************
        const string kKey = "TC3" + secrectKey;
        const string kDate = internal::HmacSHA256(kKey, date);
        const string kService = internal::HmacSHA256(kDate, service);
        const string kSigning = internal::HmacSHA256(kService, "tc3_request");
        const string signature = internal::HexEncode(internal::HmacSHA256(kSigning, stringToSign));

        // ************* 步骤 4：拼接 Authorization *************
        const string authorization = algorithm + " " + "Credential=" + secrectID + "/" + credentialScope + ", " + "SignedHeaders=" + signedHeaders + ", " + "Signature=" + signature;
        const string url = "https://" + host;
        const string authorizationHeader = "Authorization: " + authorization;
        const string hostHeader = "Host: " + host;
        const string actionHeader = "X-TC-Action: " + action;
        const string timestampHeader = "X-TC-Timestamp: " + RequestTimestamp;
        const string versionHeader = "X-TC-Version: " + version;
        const string regionHeader = "X-TC-Region: " + region;
        const string tokenHeader = "X-TC-Token: " + TOKEN;

        internal::WriteData resData;

        // ************* 步骤 5：构造并发起请求 *************
        CURL* curl = curl_easy_init();
        CURLcode res;
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_URL, url.data());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);

            curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, authorizationHeader.data());
            headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
            headers = curl_slist_append(headers, hostHeader.data());
            headers = curl_slist_append(headers, actionHeader.data());
            headers = curl_slist_append(headers, timestampHeader.data());
            headers = curl_slist_append(headers, versionHeader.data());
            headers = curl_slist_append(headers, regionHeader.data());
            headers = curl_slist_append(headers, tokenHeader.data());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            const char* data = payload.data();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal::WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resData);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res == CURLE_OK)
            {
                resData.response = internal::UTF8ToGBK(resData.response);
                return internal::ParseTencentOCRResult(resData.response);
            }
        }
        curl_easy_cleanup(curl);
        return AO_TencentOCR::Result();
    }

    AO_BaiduOCR* AO_OCREngineManager::baiduOCR = nullptr;
    AO_AliOCR* AO_OCREngineManager::aliOCR = nullptr;
    AO_TencentOCR* AO_OCREngineManager::tencentOCR = nullptr;
    bool AO_OCREngineManager::IsReady()
    {
        return baiduOCR || aliOCR || tencentOCR;
    }
    void AO_OCREngineManager::Initialize(const string& configIniFilePath)
    {
        if (!IsFileExists(configIniFilePath))
        {
            return;
        }
        const AO_IniContent config = ReadIniFile(configIniFilePath);

        if (config.find("Baidu OCR") != config.end())
        {
            const AO_IniSection& section = config.at("Baidu OCR");
            string APIKey = "", secretKey = "";
            if (section.find("API Key") != section.end())
            {
                APIKey = section.at("API Key");
            }
            if (section.find("Secret Key") != section.end())
            {
                secretKey = section.at("Secret Key");
            }
            if (!APIKey.empty() && !secretKey.empty())
            {
                AO_BaiduOCR* baiduOCR = new AO_BaiduOCR(APIKey, secretKey);
                SetBaiduOCR(baiduOCR);
            }
        }

        if (config.find("Alibaba OCR") != config.end())
        {
            const AO_IniSection& section = config.at("Alibaba OCR");
            string appCode = "";
            if (section.find("AppCode") != section.end())
            {
                appCode = section.at("AppCode");
            }
            if (!appCode.empty())
            {
                AO_AliOCR* aliOCR = new AO_AliOCR(appCode);
                SetAliOCR(aliOCR);
            }
        }

        if (config.find("Tencent OCR") != config.end())
        {
            const AO_IniSection& section = config.at("Tencent OCR");
            string secretID = "", secretKey = "";
            if (section.find("SecretId") != section.end())
            {
                secretID = section.at("SecretId");
            }
            if (section.find("SecretKey") != section.end())
            {
                secretKey = section.at("SecretKey");
            }
            if (!secretID.empty() && !secretKey.empty())
            {
                AO_TencentOCR* tencentOCR = new AO_TencentOCR(secretID, secretKey);
                SetTencentOCR(tencentOCR);
            }
        }
    }
    void AO_OCREngineManager::SetBaiduOCR(AO_BaiduOCR* baiduOCR)
    {
        if (AO_OCREngineManager::baiduOCR)
        {
            delete AO_OCREngineManager::baiduOCR;
            AO_OCREngineManager::baiduOCR = nullptr;
        }
        AO_OCREngineManager::baiduOCR = baiduOCR;
    }
    AO_BaiduOCR* AO_OCREngineManager::GetBaiduOCR()
    {
        return baiduOCR;
    }
    void AO_OCREngineManager::SetAliOCR(AO_AliOCR* aliOCR)
    {
        if (AO_OCREngineManager::aliOCR)
        {
            delete AO_OCREngineManager::aliOCR;
            AO_OCREngineManager::aliOCR = nullptr;
        }
        AO_OCREngineManager::aliOCR = aliOCR;
    }
    AO_AliOCR* AO_OCREngineManager::GetAliOCR()
    {
        return aliOCR;
    }
    void AO_OCREngineManager::SetTencentOCR(AO_TencentOCR* tencentOCR)
    {
        if (AO_OCREngineManager::tencentOCR)
        {
            delete AO_OCREngineManager::tencentOCR;
            AO_OCREngineManager::tencentOCR = nullptr;
        }
        AO_OCREngineManager::tencentOCR = tencentOCR;
    }
    AO_TencentOCR* AO_OCREngineManager::GetTencentOCR()
    {
        return tencentOCR;
    }
    AO_OCREngineManager::AO_OCREngineManager()
    {
        if (baiduOCR)
        {
            delete baiduOCR;
        }
        if (aliOCR)
        {
            delete aliOCR;
        }
        if (tencentOCR)
        {
            delete tencentOCR;
        }

        baiduOCR = nullptr;
        aliOCR = nullptr;
        tencentOCR = nullptr;
    }
    AO_OCREngineManager::~AO_OCREngineManager()
    {
        if (baiduOCR)
        {
            delete baiduOCR;
        }
        if (aliOCR)
        {
            delete aliOCR;
        }
        if (tencentOCR)
        {
            delete tencentOCR;
        }

        baiduOCR = nullptr;
        aliOCR = nullptr;
        tencentOCR = nullptr;
    }

    bool IsScreenExistsWords(const vector<string>& words, AO_Rect& wordPosition, const AO_Rect& screenArea, double minProbability, const string& configIniFilePath)
    {
        wordPosition = AO_Rect();
        if (words.empty() || minProbability < 0 || minProbability > 1 || screenArea.height == 0 || screenArea.width == 0)
        {
            return false;
        }

        if (!AO_OCREngineManager::IsReady())
        {
            AO_OCREngineManager::Initialize(configIniFilePath);

            if (!AO_OCREngineManager::IsReady())
            {
                return false;
            }
        }

        const AO_BaiduOCR* baiduOCR = AO_OCREngineManager::GetBaiduOCR();
        const AO_AliOCR* aliOCR = AO_OCREngineManager::GetAliOCR();
        const AO_TencentOCR* tencentOCR = AO_OCREngineManager::GetTencentOCR();

        if (baiduOCR)
        {
            const AO_BaiduOCR::Result result = baiduOCR->DetectFromScreen(screenArea);
            if (result.num > 0)
            {
                vector<AO_BaiduOCR::Word> matchedWords;
                matchedWords.reserve(result.num);
                for (const AO_BaiduOCR::Word& word : result.wordsResult)
                {
                    if (internal::IsContainWords(word.content, words))
                    {
                        matchedWords.push_back(word);
                    }
                }
                const auto maxElement = max_element(matchedWords.begin(), matchedWords.end(),
                    [](const AO_BaiduOCR::Word& a, const AO_BaiduOCR::Word& b)
                    {return a.probability.average < b.probability.average; }
                );
                if (maxElement->probability.average < minProbability)
                {
                    return false;
                }
                wordPosition = maxElement->location;
                return true;
            }
        }

        if (aliOCR)
        {
            const AO_AliOCR::Result result = aliOCR->DetectFromScreen(screenArea);
            if (result.isSuccess && !result.wordsResult.empty())
            {
                vector<AO_AliOCR::Word> matchedWords;
                matchedWords.reserve(result.wordsResult.size());
                for (const AO_AliOCR::Word& word : result.wordsResult)
                {
                    if (internal::IsContainWords(word.content, words))
                    {
                        matchedWords.push_back(word);
                    }
                }
                const auto maxElement = max_element(matchedWords.begin(), matchedWords.end(),
                    [](const AO_AliOCR::Word& a, const AO_AliOCR::Word& b)
                    {return a.probability < b.probability; }
                );
                if (maxElement->probability < minProbability)
                {
                    return false;
                }
                wordPosition = internal::GetRotatedRectBoundingBox(maxElement->location, maxElement->rectAngle);
                return true;
            }
        }

        if (tencentOCR)
        {
            const AO_TencentOCR::Result result = tencentOCR->DetectFromScreen(screenArea);
            if (!result.wordsResult.empty())
            {
                vector<AO_TencentOCR::Word> matchedWords;
                matchedWords.reserve(result.wordsResult.size());
                for (const AO_TencentOCR::Word& word : result.wordsResult)
                {
                    if (internal::IsContainWords(word.content, words))
                    {
                        matchedWords.push_back(word);
                    }
                }
                const auto maxElement = max_element(matchedWords.begin(), matchedWords.end(),
                    [](const AO_TencentOCR::Word& a, const AO_TencentOCR::Word& b)
                    {return a.probability < b.probability; }
                );
                if (maxElement->probability < minProbability)
                {
                    return false;
                }
                wordPosition = maxElement->location;
                return true;
            }
        }

        return false;
    }
}

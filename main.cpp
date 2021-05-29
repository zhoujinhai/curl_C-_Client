#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json/json.h"


void stringReplace(std::string& strOri, const std::string& strsrc, const std::string& strdst){
	std::string::size_type pos = 0;
	std::string::size_type srclen = strsrc.size();
	std::string::size_type dstlen = strdst.size();

	while ((pos = strOri.find(strsrc, pos)) != std::string::npos){
		strOri.replace(pos, srclen, strdst);
		pos += dstlen;
	}
}


std::string getPathShortName(std::string strFullName){
	if (strFullName.empty()){
		return "";
	}

	stringReplace(strFullName, "\\", "/");

	std::string::size_type iPos = strFullName.find_last_of('/') + 1;

	return strFullName.substr(iPos, strFullName.length() - iPos);
}


std::wstring AsciiToUnicode(const std::string& str)
{
	// 预算-缓冲区中宽字节的长度  
	int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	// 给指向缓冲区的指针变量分配内存  
	wchar_t* pUnicode = (wchar_t*)malloc(sizeof(wchar_t) * unicodeLen);
	// 开始向缓冲区转换字节  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
	std::wstring ret_str = pUnicode;
	free(pUnicode);
	return ret_str;
}


std::string UnicodeToUtf8(const std::wstring& wstr)
{
	// 预算-缓冲区中多字节的长度  
	int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	// 给指向缓冲区的指针变量分配内存  
	char* pAssii = (char*)malloc(sizeof(char) * ansiiLen);
	// 开始向缓冲区转换字节  
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
	std::string ret_str = pAssii;
	free(pAssii);
	return ret_str;
}


std::string AsciiToUtf8(const std::string& str)
{
	return UnicodeToUtf8(AsciiToUnicode(str));
}

// 回调函数1 接收所有数据
//回调函数
struct memory 
{
	char* response;
	size_t size;
	memory()
	{
		response = NULL;
		size = 0;
	}
};

size_t write_data1(void* data, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct memory* mem = (struct memory*)userp;
	char* ptr = (char*)realloc(mem->response, mem->size + realsize + 1);
	if (ptr == NULL)
		return 0;

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

// 回调函数2 数据长的话只接收最后一次数据
static size_t write_data2(void* ptr, size_t size, size_t nmemb, void* stream)
{
    std::cout << "****** predict done! ******" << std::endl;
    std::string data((const char*)ptr, (size_t)size * nmemb);
    //*((std::stringstream*) stream) << data << std::endl;
    std::string* result = static_cast<std::string*>(stream);
    *result = data;

    return size * nmemb;
}


bool reqMeshCNNFileUpload(std::string IP, std::string Port, const char* dentalPath, std::string &result) {
	std::string reqURL = "http://" + IP + ":" + Port + "/mesh/recognize";
	
	CURL* curl = nullptr;

	curl_global_init(CURL_GLOBAL_ALL);

	// 初始化easy handler句柄
	curl = curl_easy_init();
	if (nullptr == curl) {
		printf("failed to create curl connection.\n");
		return false;
	}

	// 设置post请求的url地址
	CURLcode code;
	code = curl_easy_setopt(curl, CURLOPT_URL, reqURL.c_str());
	if (code != CURLE_OK) {
		printf("failed to set url.\n");
		return false;
	}

	// 设置回调函数，如果不设置，默认输出到控制台
	code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	if (code != CURLE_OK) {
		printf("failed to set write data.\n");
		return false;
	}

	//设置接收数据的处理函数和存放变量
	struct memory stBody;  
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stBody);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

	//初始化form
	curl_mime* form;
	curl_mimepart* field;
	/* Create the form */
	form = curl_mime_init(curl);
	/* Fill in the file upload field */
	field = curl_mime_addpart(form);
	curl_mime_name(field, "file");
	curl_mime_filedata(field, dentalPath);
	/*seed form*/
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

	//HTTP报文头
	struct curl_slist* headers = nullptr;
	/*set header info，You can set more than one bar*/
	headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
	headers = curl_slist_append(headers, "User-Agent: curl");
	/* Pass in the HTTP header*/
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//执行单条请求
	CURLcode res;
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("curl_easy_perform failed! {}\n");
		return false;
	}
	result = stBody.response;

	// 释放资源
	curl_slist_free_all(headers);
	/* 释放form */
	curl_mime_free(form);
	/* always cleanup */
	curl_easy_cleanup(curl);
	/*对curl_global_init做的工作清理 类似于close的函数*/
	curl_global_cleanup();

	return true;
}


bool reqMeshCNNFilePath(std::string IP, std::string Port, const char* dentalPath, std::string& result) {
	std::string reqURL = "http://" + IP + ":" + Port + "/mesh/recognize";

	// json 数据
	std::string filename = getPathShortName(dentalPath);
	
	Json::Value value;
	value["file_path"] = Json::Value(dentalPath);
	value["filename"] = Json::Value(filename.c_str());
	std::string strResult = value.toStyledString();
	strResult = AsciiToUtf8(strResult);

	CURL* curl = nullptr;
	curl_global_init(CURL_GLOBAL_ALL);

	// 初始化easy handler句柄
	curl = curl_easy_init();
	if (nullptr == curl) {
		printf("failed to create curl connection.\n");
		return false;
	}

	// 设置post请求的url地址
	CURLcode code;
	code = curl_easy_setopt(curl, CURLOPT_URL, reqURL.c_str());
	if (code != CURLE_OK) {
		printf("failed to set url.\n");
		return false;
	}

	// 设置回调函数，如果不设置，默认输出到控制台
	code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	if (code != CURLE_OK) {
		printf("failed to set write data.\n");
		return false;
	}

	//设置接收数据的处理函数和存放变量
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResult.c_str());
	struct memory stBody;  
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stBody);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

	//HTTP报文头
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");//text/html
	headers = curl_slist_append(headers, "charsets: utf-8");
	// set method to post
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//执行单条请求
	CURLcode res;
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("curl_easy_perform failed! {}\n");
		return false;
	}
	result = stBody.response;

	curl_slist_free_all(headers);
	/* always cleanup */
	curl_easy_cleanup(curl);
	//在结束libcurl使用的时候，用来对curl_global_init做的工作清理。类似于close的函数
	curl_global_cleanup();

	return true;
}


bool parseResult(std::string result) {
	stringReplace(result, "\\\"", "\"");
	result = std::string(result, 1, result.size() - 2);
	
	Json::CharReaderBuilder readerBuild;
	Json::CharReader* reader(readerBuild.newCharReader());
	Json::Value rcvRes;
	JSONCPP_STRING jsonErrs;
	bool parseOK = reader->parse(result.c_str(), result.c_str() + result.size(), &rcvRes, &jsonErrs);

	delete reader;
	if (!parseOK) {
		std::cout << "Failed to parse the rcvRes!" << std::endl;
		return false;
	}
	std::cout << "Rcv:\n" << std::endl;

	// TODO 多维数组 多个闭环
	int rcvSize = rcvRes["text"].size();
	std::cout << rcvSize << std::endl;
	for (int i = 0; i < rcvSize; ++i) {
		Json::Value rcvResItem = rcvRes["text"][i];
		std::cout << "x: " << rcvResItem[0] << " y: " << rcvResItem[1] << " z: " << rcvResItem[2] << std::endl;
	}
	return true;
}


int main(int argc, char* argv[]) {
	// 方式一：json传递文件名
	std::string result;
	bool reqOK;
	const std::string IP = "192.168.102.116";
	reqOK = reqMeshCNNFilePath(IP, "8000", "E:/code/python_web/MeshCNN/test_models/test.obj", result);
	if (!reqOK) {
		std::cout << "req is failed!" << std::endl;
		return -1;
	}
	// 解析结果
	std::cout << "result: " << result << std::endl;
	parseResult(result);

	//// 方式二：文件上传
	//reqOK = reqMeshCNNFileUpload("192.168.102.116", "8000", "E:/code/python_web/MeshCNN/test_models/test.obj", result);
	//if (!reqOK) {
	//	std::cout << "req is failed!" << std::endl;
	//	return -1;
	//}
	//// 解析结果
	//parseResult(result);

	return 0;
}

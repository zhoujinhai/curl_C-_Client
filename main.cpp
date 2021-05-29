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
	// Ԥ��-�������п��ֽڵĳ���  
	int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	// ��ָ�򻺳�����ָ����������ڴ�  
	wchar_t* pUnicode = (wchar_t*)malloc(sizeof(wchar_t) * unicodeLen);
	// ��ʼ�򻺳���ת���ֽ�  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
	std::wstring ret_str = pUnicode;
	free(pUnicode);
	return ret_str;
}


std::string UnicodeToUtf8(const std::wstring& wstr)
{
	// Ԥ��-�������ж��ֽڵĳ���  
	int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	// ��ָ�򻺳�����ָ����������ڴ�  
	char* pAssii = (char*)malloc(sizeof(char) * ansiiLen);
	// ��ʼ�򻺳���ת���ֽ�  
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
	std::string ret_str = pAssii;
	free(pAssii);
	return ret_str;
}


std::string AsciiToUtf8(const std::string& str)
{
	return UnicodeToUtf8(AsciiToUnicode(str));
}

// �ص�����1 ������������
//�ص�����
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

// �ص�����2 ���ݳ��Ļ�ֻ�������һ������
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

	// ��ʼ��easy handler���
	curl = curl_easy_init();
	if (nullptr == curl) {
		printf("failed to create curl connection.\n");
		return false;
	}

	// ����post�����url��ַ
	CURLcode code;
	code = curl_easy_setopt(curl, CURLOPT_URL, reqURL.c_str());
	if (code != CURLE_OK) {
		printf("failed to set url.\n");
		return false;
	}

	// ���ûص���������������ã�Ĭ�����������̨
	code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	if (code != CURLE_OK) {
		printf("failed to set write data.\n");
		return false;
	}

	//���ý������ݵĴ������ʹ�ű���
	struct memory stBody;  
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stBody);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

	//��ʼ��form
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

	//HTTP����ͷ
	struct curl_slist* headers = nullptr;
	/*set header info��You can set more than one bar*/
	headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
	headers = curl_slist_append(headers, "User-Agent: curl");
	/* Pass in the HTTP header*/
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//ִ�е�������
	CURLcode res;
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("curl_easy_perform failed! {}\n");
		return false;
	}
	result = stBody.response;

	// �ͷ���Դ
	curl_slist_free_all(headers);
	/* �ͷ�form */
	curl_mime_free(form);
	/* always cleanup */
	curl_easy_cleanup(curl);
	/*��curl_global_init���Ĺ������� ������close�ĺ���*/
	curl_global_cleanup();

	return true;
}


bool reqMeshCNNFilePath(std::string IP, std::string Port, const char* dentalPath, std::string& result) {
	std::string reqURL = "http://" + IP + ":" + Port + "/mesh/recognize";

	// json ����
	std::string filename = getPathShortName(dentalPath);
	
	Json::Value value;
	value["file_path"] = Json::Value(dentalPath);
	value["filename"] = Json::Value(filename.c_str());
	std::string strResult = value.toStyledString();
	strResult = AsciiToUtf8(strResult);

	CURL* curl = nullptr;
	curl_global_init(CURL_GLOBAL_ALL);

	// ��ʼ��easy handler���
	curl = curl_easy_init();
	if (nullptr == curl) {
		printf("failed to create curl connection.\n");
		return false;
	}

	// ����post�����url��ַ
	CURLcode code;
	code = curl_easy_setopt(curl, CURLOPT_URL, reqURL.c_str());
	if (code != CURLE_OK) {
		printf("failed to set url.\n");
		return false;
	}

	// ���ûص���������������ã�Ĭ�����������̨
	code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	if (code != CURLE_OK) {
		printf("failed to set write data.\n");
		return false;
	}

	//���ý������ݵĴ������ʹ�ű���
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResult.c_str());
	struct memory stBody;  
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stBody);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

	//HTTP����ͷ
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");//text/html
	headers = curl_slist_append(headers, "charsets: utf-8");
	// set method to post
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//ִ�е�������
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
	//�ڽ���libcurlʹ�õ�ʱ��������curl_global_init���Ĺ�������������close�ĺ���
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

	// TODO ��ά���� ����ջ�
	int rcvSize = rcvRes["text"].size();
	std::cout << rcvSize << std::endl;
	for (int i = 0; i < rcvSize; ++i) {
		Json::Value rcvResItem = rcvRes["text"][i];
		std::cout << "x: " << rcvResItem[0] << " y: " << rcvResItem[1] << " z: " << rcvResItem[2] << std::endl;
	}
	return true;
}


int main(int argc, char* argv[]) {
	// ��ʽһ��json�����ļ���
	std::string result;
	bool reqOK;
	const std::string IP = "192.168.102.116";
	reqOK = reqMeshCNNFilePath(IP, "8000", "E:/code/python_web/MeshCNN/test_models/test.obj", result);
	if (!reqOK) {
		std::cout << "req is failed!" << std::endl;
		return -1;
	}
	// �������
	std::cout << "result: " << result << std::endl;
	parseResult(result);

	//// ��ʽ�����ļ��ϴ�
	//reqOK = reqMeshCNNFileUpload("192.168.102.116", "8000", "E:/code/python_web/MeshCNN/test_models/test.obj", result);
	//if (!reqOK) {
	//	std::cout << "req is failed!" << std::endl;
	//	return -1;
	//}
	//// �������
	//parseResult(result);

	return 0;
}

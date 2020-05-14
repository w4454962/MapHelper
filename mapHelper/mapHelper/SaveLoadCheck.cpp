#include <string>
#include <map>

class CSaveLoadCheck {
public:
	typedef std::map<std::string, std::string> TSaveLoadMap;

	void Reset() {
		m_map.clear();
	}

	bool Set(const char* key, const char* value) {
		if (key && value && m_map.find(key) == m_map.end()) {
			m_map[key] = this->Convert(value);
			return true;
		}
		else {
			return (m_map[key].compare(this->Convert(value)) == 0);
		}
	}

	const char* Get(const char* key) {
		TSaveLoadMap::iterator itr;

		if ((itr = m_map.find(key)) == m_map.end()) {
			return nullptr;
		}
		else {
			return this->Convert(itr->second.c_str());
		}
	}

private:
	const char* Convert(const char* src) const {
		if (0 == strcmp(src, "StringExt")) {
			return "string";
		}
		if (0 == strcmp(src, "imagefile")) {
			return "string";
		}
		if (0 == strcmp(src, "modelfile")) {
			return "string";
		}
		if (0 == strcmp(src, "radian")) {
			return "real";
		}
		if (0 == strcmp(src, "degree")) {
			return "real";
		}
		if (0 == strcmp(src, "degree")) {
			return "real";
		}
		if (0 == strcmp(src, "unitcode")) {
			return "integer";
		}
		if (0 == strcmp(src, "abilcode")) {
			return "integer";
		}
		if (0 == strcmp(src, "itemcode")) {
			return "integer";
		}

		return src;
	}
	TSaveLoadMap m_map;
};

CSaveLoadCheck g_SaveLoadCheck;

#include <windows.h>

void SaveLoadCheck_Reset() {
	g_SaveLoadCheck.Reset();
}

BOOL SaveLoadCheck_Set(LPCSTR lpszKey, LPCSTR lpszName) {
	if (g_SaveLoadCheck.Set(lpszKey, lpszName)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

LPCSTR SaveLoadCheck_Get(LPCSTR lpszKey) {
	LPCSTR result = g_SaveLoadCheck.Get(lpszKey);
	if (result == nullptr)
		result = "";

	return result;
}

int _fastcall
Utf8toAscii(char src[], char dst[], unsigned int limit)
{
	wchar_t* temp;
	unsigned int len;

	len = MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0);

	temp = (wchar_t*)malloc(len * sizeof(wchar_t));

	if (temp == NULL)
		return 0;

	if (len > 0)
	{
		len = MultiByteToWideChar(CP_UTF8, 0, src, -1, temp, len * sizeof(wchar_t));
		len = WideCharToMultiByte(CP_ACP, 0, temp, -1, 0, 0, 0, 0);
		if (len > limit) len = limit;
		WideCharToMultiByte(CP_ACP, 0, temp, -1, dst, len * sizeof(char), 0, 0);

		dst[len] = 0;
		free(temp);
		return 1;
	}
	else
	{
		free(temp);
		return 0;
	}
}

std::string UTF8_To_string(const std::string& str)
{
	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	wchar_t* pwBuf = new wchar_t[nwLen + 1];    //一定要加1，不然会出现尾巴 
	memset(pwBuf, 0, nwLen * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char* pBuf = new char[nLen + 1];
	memset(pBuf, 0, nLen + 1);
	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	std::string strRet = pBuf;

	delete[]pBuf;
	delete[]pwBuf;
	pBuf = NULL;
	pwBuf = NULL;

	return strRet;
}
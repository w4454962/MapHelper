#pragma once
#include <string>
#include <windows.h>
inline uint32_t getRegKey(const char* location, const char* name)
{
	HKEY key;
	uint32_t value;
	uint32_t size = 4;
	long ret;
	ret = RegOpenKeyExA(HKEY_CURRENT_USER, location, 0, KEY_QUERY_VALUE, &key);
	if (ret != ERROR_SUCCESS)
	{
		return 0;
	}
	ret = RegQueryValueExA(key, name, 0, 0, (LPBYTE)&value,(DWORD*) &size);
	RegCloseKey(key);
	return value;
}
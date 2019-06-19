#include "stdafx.h"

hash_t hash_(const char* str)
{
	hash_t ret{ basis };
	while (*str)
	{
		ret ^= *str;
		ret *= prime;
		str++;
	}
	return ret;
}

std::string string_replaced(const std::string& source, const std::string& from, const std::string& to) {
	std::string new_string;
	new_string.reserve(source.length());  // avoids a few memory allocations

	size_t lastPos = 0;
	size_t findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos))) {
		new_string.append(source, lastPos, findPos - lastPos);
		new_string += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	new_string += source.substr(lastPos);

	return new_string;
}

void convert_name(std::string& name)
{

	if (name.length() == 0)
		return;


	//空格结尾的删掉
	for (int i = name.length(); i > 0; --i)
	{
		if (name[i - 1] == ' ')
			name[i - 1] = '\0';
		else
			break;
			
	}

	for (size_t i = 0; i < name.length(); ++i)
	{
		uint8_t c = name[i];
		if (c && !isalnum(c) && c != '_')
			name[i] = '_';
	}
	//如果末尾是以下划线结尾的 要加上u
	if (name.length() > 0 && name[name.length() - 1] == '_')
	{
		name += 'u';
	}
	name = name.c_str();
}
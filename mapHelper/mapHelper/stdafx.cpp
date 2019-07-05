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
	name = name.c_str();

	//如果末尾是以下划线结尾的 要加上u
	if (name.length() > 0 && name[name.length() - 1] == '_')
	{
		name += 'u';
	}
}


void convert_loop_var_name(std::string& name,int limit)
{
	std::string retval;

	uint32_t j;
	uint32_t addup = 0;

	static unsigned char ConvertToAlnum[] = {
	  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	  'a', 'b', 'c', 'd', 'e', 'f', 'g',
	  'h', 'i', 'j', 'k', 'l', 'm', 'n',
	  'o', 'p', 'q', 'r', 's', 't',
	  'u', 'v', 'w', 'x', 'y', 'z',
	  'A', 'B', 'C', 'D', 'E', 'F', 'G',
	  'H', 'I', 'J', 'K', 'L', 'M', 'N',
	  'O', 'P', 'Q', 'R', 'S', 'T',
	  'U', 'V', 'W', 'X', 'Y', 'Z',
	};

	//空格结尾的删掉
	for (size_t i = name.length(); i > 0; --i)
	{
		if (name[i - 1] == ' ')
			name[i - 1] = '\0';
		else
			break;

	}
	
	for (size_t i = 0; i < name.length(); ++i)
	{
		uint8_t c = name[i];
		if (c == ' ')
		{
			retval += '_';
		}
		else if (c > 0)
		{
			if (!isalnum(c) && c != '_')
			{
				addup = addup * 256 + name[i];
				while ((i < limit - 2) && (addup > 62))
				{
					retval += ConvertToAlnum[addup % 62];
					addup /= 62;
				}
			}
			else
			{
				retval += c;
			}
		}
	}
	if (addup != 0) retval += ConvertToAlnum[addup % 62];

	name = retval;
}


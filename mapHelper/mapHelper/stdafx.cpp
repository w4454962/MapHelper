#include "stdafx.h"


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
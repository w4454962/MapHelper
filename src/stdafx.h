// stdafx.h: 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 项目特定的包含文件
//

#pragma once


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <Windows.h>


// 在此处引用程序需要的其他标头
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <inttypes.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <memory>
#ifdef _HAS_CXX17
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;

#endif

#include "utils\fp_call.h"
#include "utils\BinaryWriter.h"
#include "include\Export.h"


#pragma  warning( disable: 4307 4455 )
//控制台流

std::string string_replaced(const std::string& source, const std::string& from, const std::string& to);
void convert_name(std::string& name);
void convert_loop_var_name(std::string& name, size_t limit = 260);

typedef std::uint32_t hash_t;

constexpr hash_t prime = 0x000001B3u;
constexpr hash_t basis = 0x84222325u;

hash_t hash_(const char* str);


constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
	return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
}

constexpr unsigned int operator "" s_hash(char const* p, size_t)
{
	return hash_compile_time(p);
}
//globals


extern MakeEditorData* g_make_editor_data;
extern char fmtbuffer[0x1000];
template <typename ... Args>
void print(const char* fmt, Args ... args)
{
	if (g_make_editor_data)
	{
		snprintf(fmtbuffer, sizeof(fmtbuffer), fmt, args...);

		g_make_editor_data->out_print(fmtbuffer);
	}
	else
	{
		printf(fmt, args...);
	}
}

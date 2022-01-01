#include <fstream>
#include "YDPluginManager.h"
#include "MapHelper.h"
#include "../resource.h"
#include <base\hook\iat.h>
#include "shellapi.h"
#include <regex>

// 目的 在ydwe调用插件时 将插件重导向 maphelper附带的插件。

BOOL WINAPI fakeCreateProcessW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation ) {

	auto& manager = get_ydplugin_manager();


	std::wstring cmd = lpCommandLine;

	if (manager.m_enable) {
		int argn = 0;
		auto argv = CommandLineToArgvW(lpCommandLine, &argn);
		if (argn > 0) {
			fs::path exe_path = argv[0];
			std::wstring exe_name = exe_path.filename().wstring();
			std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::tolower);

			auto it = manager.m_plugins_path.find(exe_name);
			if (it != manager.m_plugins_path.end()) {
				cmd = L"\"" + it->second + L"\"";
				for (int i = 1; i < argn; i++) {
					std::wstring value = std::wstring(argv[i]);
					value = std::regex_replace(value, std::wregex(L"\""), L"\\\"");
					cmd += L" \"" + value + L"\"";
				}
			}
		}
		//print("execute %s\n", base::w2a(cmd).c_str());
	}
	
	
	return CreateProcessW(
		lpApplicationName,
		(LPWSTR)cmd.c_str(),
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);
}




YDPluginManager::YDPluginManager()
	:m_attach(false),
	m_extract(false),
	m_enable(false)
{
	
}

YDPluginManager::~YDPluginManager() {
	
}


void YDPluginManager::attach() {
	if (m_attach) {
		return;
	}

	std::vector<std::string> modules = {
		"sys.dll",
		"process.dll",
		"ydbase.dll"
	};

	for (auto& name : modules) {
		auto handle = GetModuleHandleA(name.c_str());
		if (handle) {
			HookInfo info;
			info.module = handle;
			info.dll_name = "Kernel32.dll";
			info.api_name = "CreateProcessW";
			info.real = base::hook::iat(info.module, info.dll_name.c_str(), info.api_name.c_str(), (uintptr_t)&fakeCreateProcessW);
			m_hookCreateProcessW.push_back(info);
		}
	}
	if (m_hookCreateProcessW.empty()) {
		return;
	}

	m_attach = true;

}

void YDPluginManager::detach() {
	if (!m_attach) {
		return;
	}
	m_attach = false;

	for (auto& info : m_hookCreateProcessW) {
		base::hook::iat(info.module, info.dll_name.c_str(), info.api_name.c_str(), info.real);
	}
}
 


void YDPluginManager::extract() {
	if (m_extract) {
		return;
	}
	m_extract = true;

	std::map<std::string, int> files = {
		{"wave.exe", IDR_EXE1}
	};
	
	fs::path path = fs::temp_directory_path() / "plugin";

	fs::create_directories(path);

	for (auto&& [name, id] : files) {
		fs::path file_path = path / name;

		fs::remove(file_path);

		auto resource = FindResourceA(g_hModule, MAKEINTRESOURCE(id), TEXT("exe"));
		if (!resource) {
			continue;
		}
		void* data = LoadResource(g_hModule, resource);
		if (!data) {
			continue;
		}
		void* file_data = LockResource(data);
		if (!file_data) {
			continue;
		}
		DWORD file_size = SizeofResource(g_hModule, resource);
		if (file_size == 0) {
			continue;
		}
		std::ofstream file(file_path, std::ios::binary);
		if (file.is_open()) {
			file.write((const char*)file_data, file_size);
			file.close();
			m_plugins_path[base::a2w(name)] = base::a2w(file_path.string());
		}
	}
}


YDPluginManager& get_ydplugin_manager() {
	static YDPluginManager instance;
	return instance;
}
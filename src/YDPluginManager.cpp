#include <fstream>
#include "YDPluginManager.h"
#include "MapHelper.h"
#include "../resource.h"
#include <base\hook\iat.h>
#include "shellapi.h"
#include <regex>
#include <lua.hpp>

static clock_t g_last_start_time;
static std::string g_last_exe_name;
static DWORD g_thread_id = 0;

lua_State* g_lua_state = nullptr;

// 目的 在ydwe调用插件时 将插件重导向 maphelper附带的插件。

static BOOL WINAPI fakeCreateProcessW(
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

	int argn = 0;
	auto argv = CommandLineToArgvW(lpCommandLine, &argn);
	if (argn > 0) {
		fs::path exe_path = argv[0];
		std::wstring exe_name = exe_path.filename().wstring();
		std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::tolower);

		g_last_exe_name = base::w2a(exe_name);
		g_last_start_time = clock();

		print("插件 [%s] 开始运行\n", g_last_exe_name.c_str());
	
		if (manager.m_enable) {
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

	}
		//print("execute %s\n", base::w2a(cmd).c_str());
		
	

	BOOL ret = CreateProcessW(
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

	return ret;
}




static DWORD WINAPI fakeWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
	DWORD ret = WaitForSingleObject(hHandle, dwMilliseconds);

	if (GetCurrentThreadId() == g_thread_id && !g_last_exe_name.empty()) {
		print("插件 [%s] 运行结束 耗时: %f 秒\n", g_last_exe_name.c_str(), (double)(clock() - g_last_start_time) / CLOCKS_PER_SEC);
		g_last_exe_name.clear();

	}

	return ret;
}

static int lua_console_write(lua_State* L) {
	if (!lua_isstring(L, 1)) {
		return 0;
	}

	size_t size;
	const char* str = lua_tolstring(L, 1, &size);
	std::string msg = std::string(str, size);
	printf("%s\n", base::u2a(msg).c_str());
	return 0;
}


static void init_script(lua_State* L) {
	const char script[] = R"(

	log.debug("--------------maphelper--------------------")

	function print(...)
		local args = {...}
		local count = select('#', ...)
		local s = {}
	
		for i = 1, count do 
			local v = args[i]
			s[#s + 1] = tostring(v)
		end 

		console_write(table.concat(s, '\t'))
	end 

)";

	lua_register(L, "console_write", lua_console_write);

	if (LUA_OK != luaL_loadstring(L, script)) {
		printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}
	lua_call(L, 0, 0);
}

static const char* fake_lua_pushstring(lua_State* L, const char* s) {
	if (g_lua_state == nullptr) {
		if (strcmp(s, "main_window_handle") == 0) {
			g_lua_state = L;
			HWND hwnd = *(HWND*)((uint32_t)GetModuleHandleA(NULL) + 0x403C9C);
			SetTimer(hwnd, 10086, 100, [](HWND hwnd, UINT, UINT_PTR, DWORD) {
				init_script(g_lua_state);
				::KillTimer(hwnd, 10086);
			});
		}
	}
	return lua_pushstring(L, s);
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

	g_thread_id = ::GetCurrentThreadId();

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
			m_hook_list.push_back(info);

			HookInfo info2;
			info2.module = handle;
			info2.dll_name = "Kernel32.dll";
			info2.api_name = "WaitForSingleObject";
			info2.real = base::hook::iat(info2.module, info2.dll_name.c_str(), info2.api_name.c_str(), (uintptr_t)&fakeWaitForSingleObject);
			m_hook_list.push_back(info2);
		}
	}
	modules = {
		"luacore.dll",
		"lua53.dll",
	};
	for (auto& name : modules) {
		auto handle = GetModuleHandleA("event.dll");
		if (handle && GetModuleHandleA(name.c_str())) {
			HookInfo info;
			info.module = handle;
			info.dll_name = name.c_str();
			info.api_name = "lua_pushstring";
			info.real = base::hook::iat(info.module, info.dll_name.c_str(), info.api_name.c_str(), (uintptr_t)&fake_lua_pushstring);
			m_hook_list.push_back(info);
		}
	}


	if (m_hook_list.empty()) {
		return;
	}

	m_attach = true;

}

void YDPluginManager::detach() {
	if (!m_attach) {
		return;
	}
	m_attach = false;

	for (auto& info : m_hook_list) {
		base::hook::iat(info.module, info.dll_name.c_str(), info.api_name.c_str(), info.real);
	}
}
 


void YDPluginManager::extract() {
	if (m_extract) {
		return;
	}
	m_extract = true;

	std::map<std::string, int> files = {
		{"wave.exe", IDR_EXE1},
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

void YDPluginManager::on_save_event(const std::string& map_path, bool is_test) {
	if (g_lua_state == nullptr) {
		return;
	}

	lua_State* L = g_lua_state;

	const char script[] = R"(
local map_path, is_test = ...

log.info("on_save_event map_path", map_path)
log.info("on_save_event is_test", is_test)

	--草 acb的孤儿写法 拿不到event里的函数 操他妈的 写个破工具还怕被人调用
	--local func = event.EVENT_SAVE_MAP or event.EVENT_NEW_SAVE_MAP

	local func
	for k, v in pairs(debug.getregistry()) do
		if type(v) == 'function' then 
			local info = debug.getinfo(v, "S")
			if info.short_src and info.short_src:find("ydwe_on_save") and info.linedefined and info.linedefined < 188 then 
				func = v
				log.info("file", info.short_src, info.linedefined)
				break
			end 
		end 
	end
	if func then 
		func({ map_path = map_path, test = is_test})
	end 
)";

	if (LUA_OK != luaL_loadstring(L, script)) {
		printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}
	lua_pushlstring(L, map_path.c_str(), map_path.size());
	lua_pushboolean(L, is_test);
	if (LUA_OK != lua_pcall(L, 2, 0, 0)) {
		printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

YDPluginManager& get_ydplugin_manager() {
	static YDPluginManager instance;
	return instance;
}
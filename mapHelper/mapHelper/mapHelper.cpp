// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "mapHelper.h"
#include "singleton.h"

#pragma warning(disable:4996)

const char* g_path;
uintptr_t g_object;
uintptr_t g_addr;
uintptr_t g_convertAddr;

Helper::Helper()
	: m_bAttach(false)
{
}

Helper::~Helper()
= default;

//Helper* Helper::getInstance()
//{
//	static Helper instance;
//	return &instance;
//}


static void __declspec(naked) insertSaveMapData()
{
	
	__asm
	{
		mov g_object, esi
		lea eax, dword ptr ss : [ebp - 0x10c];
		mov g_path, eax
		pushad
		pushfd
		call get_helper
		mov ecx, eax
	
		call Helper::onSaveMap
		mov g_addr, eax
		popfd
		popad
		jmp g_addr
	}
}

static void __declspec(naked) insertConvertTrigger()
{
	__asm
	{
		mov g_object, ecx
		call get_helper
		mov ecx, eax
		call Helper::onSelectConvartMode
		test eax,eax 
		je pos

		mov ecx,g_object
		jmp g_convertAddr
		
	pos:

		pushad
		pushfd 
		call get_helper
		mov ecx,eax
		push g_object
		call Helper::onConvertTrigger
		mov g_object,eax 
		popfd
		popad 
		mov eax,g_object
		ret 0x4
	}
	

}

uintptr_t Helper::onSaveMap()
{
	auto& editor = get_world_editor();
	editor.onSaveMap(g_path);
	return editor.getAddress(0x0055D175);
}



void Helper::attach()
{
	if (m_bAttach) return;

	char buffer[0x400];

	GetModuleFileNameA(nullptr, buffer, 0x400);

	std::string name = fs::path(buffer).filename().string();
	if (name.find("worldedit") == std::string::npos)
		return;
	
	GetModuleFileNameA(GetModuleHandleA("ydbase.dll"), buffer, 0x400);
	
	m_configPath = fs::path(buffer).remove_filename() / "EverConfig.cfg";

	m_bAttach = true;


	auto& editor = get_world_editor();

	uintptr_t addr = editor.getAddress(0x0055CDE6);

	hook::install(&addr, reinterpret_cast<uintptr_t>(&insertSaveMapData),m_hookSaveMap);

	addr = editor.getAddress(static_cast<uintptr_t>(0x005CB4C0));

	hook::install(&addr, reinterpret_cast<uintptr_t>(&insertConvertTrigger), m_hookConvertTrigger);
	g_convertAddr = addr;

#if !defined(EMBED_YDWE)
	if (getConfig() == -1)
	{
		enableConsole();
	}
#endif
}

int Helper::onSelectConvartMode()
{
#if defined(EMBED_YDWE)
	return 0;
#else
	int result = getConfig();
	if (result == -1)
	{
		int ret = MessageBoxA(0, "是否用新的保存模式保存?", "问你", MB_YESNO);

		if (ret == 6)
		{
			printf("自定义保存模式\n");
			return 0;
		}
		printf("原始保存模式\n");
		return 1;
	}
	else
	{
		if (result == 1)
		{
			return 0;
		}
		return 1;
	}
#endif
}

int Helper::onConvertTrigger(Trigger* trigger)
{
	auto& v_we = get_world_editor();
	TriggerData* data = v_we.getEditorData()->triggers;
	auto& editor = get_trigger_editor();
	editor.loadTriggers(data);

	return editor.onConvertTrigger(trigger) ? 1 : 0;
}

Helper& get_helper()
{
	return base::singleton<Helper>::instance();
}

void Helper::detach()
{
	if (!m_bAttach) return; 
	m_bAttach = false;

	hook::uninstall(m_hookSaveMap);
	hook::uninstall(m_hookConvertTrigger);
#if !defined(EMBED_YDWE)
	//释放控制台避免崩溃
	FreeConsole();
#endif
}


void Helper::enableConsole()
{
	HWND v_hwnd_console = ::GetConsoleWindow();

	if (nullptr != v_hwnd_console)
	{
		::ShowWindow(v_hwnd_console, SW_SHOW);

	}
	else
	{	
		auto v_is_ok = AllocConsole();
		if (v_is_ok)
		{
			FILE* fp;
			//freopen_s(&fp, "CONOUT$", "r", stdin);
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONOUT$", "w", stderr);
			fclose(fp);
			std::cout.clear();
		}
	}
	v_hwnd_console = ::GetConsoleWindow();
	if (v_hwnd_console)
	{
		::DeleteMenu(::GetSystemMenu(v_hwnd_console, FALSE), SC_CLOSE, MF_BYCOMMAND);
		::DrawMenuBar(v_hwnd_console);
		::SetWindowTextA(v_hwnd_console, "ydwe保存加速插件 1.0a");
		std::cout
			<< "用来加速ydwe保存地图的插件，对地形装饰物，触发编辑器极速优化\n"
			<< "参与开发者 ：w4454962、 神话、 actboy168\n"
			<< "参与测试人员： 幽影、夜夜、七罪、五爷、妖精\n"
			<< "排名不分先后，为魔兽地图社区的贡献表示感谢。\n"
			<< "                         ----2019/07/09\n";

	}
}

int Helper::getConfig() const
{
	return GetPrivateProfileIntA("ScriptCompiler", "EnableYDTrigger", -1, m_configPath.string().c_str());
}

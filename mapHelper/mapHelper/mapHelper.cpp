// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mapHelper.h"



const char* g_path;
uintptr_t g_object;
uintptr_t g_addr;
uintptr_t g_convertAddr;

Helper::Helper()
	: m_bAttach(0)
{
}

Helper::~Helper()
{
}

Helper* Helper::getInstance()
{
	static Helper instance;
	return &instance;
}


static void __declspec(naked) insertSaveMapData()
{
	__asm
	{
		mov g_object, esi
		lea eax, dword ptr ss : [ebp - 0x10c];
		mov g_path, eax
		pushad
		pushfd
		call Helper::getInstance
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
		call Helper::getInstance
		mov ecx, eax
		call Helper::onSelectConvartMode
		test eax,eax 
		je pos

		mov ecx,g_object
		jmp g_convertAddr
		
	pos:

		pushad
		pushfd 
		call Helper::getInstance
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
	WorldEditor* editor = WorldEditor::getInstance();
	editor->onSaveMap(g_path);
	return editor->getAddress(0x0055D175);
}



void Helper::attatch()
{
	if (m_bAttach) return;

	char buffer[0x400];

	GetModuleFileNameA(NULL, buffer, 0x400);

	std::string name = fs::path(buffer).filename().string();
	if (name.find("worldedit") == std::string::npos)
		return;
	
	GetModuleFileNameA(GetModuleHandleA("ydwar3.dll"), buffer, 0x400);
	
	m_configPath = fs::path(buffer).remove_filename() / "EverConfig.cfg";

	m_bAttach = true;


	WorldEditor* editor = WorldEditor::getInstance();

	uintptr_t addr = editor->getAddress(0x0055CDE6);

	hook::install(&addr, (uintptr_t)&insertSaveMapData,m_hookSaveMap);

	addr = editor->getAddress(0x005CB4C0);

	hook::install(&addr, (uintptr_t)&insertConvertTrigger, m_hookConvertTrigger);
	g_convertAddr = addr;

	if (getConfig() == -1)
	{
		enableConsole();
	}
	
}

int Helper::onSelectConvartMode()
{
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
}

int Helper::onConvertTrigger(Trigger* trigger)
{
	
	TriggerData* data = WorldEditor::getInstance()->getEditorData()->triggers;
	TriggerEditor* editor = TriggerEditor::getInstance();
	editor->loadTriggers(data);

	return editor->onConvertTrigger(trigger) ? 1 : 0;
}

void Helper::detach()
{
	if (!m_bAttach) return; 
	m_bAttach = false;

	hook::uninstall(m_hookSaveMap);
	hook::uninstall(m_hookConvertTrigger);
}


void Helper::enableConsole()
{
	HWND h = ::GetConsoleWindow();

	if (h)
	{
		::ShowWindow(h, SW_SHOW);
	}
	else
	{
		FILE* new_file;
		::AllocConsole();
		freopen_s(&new_file, "CONIN$", "r", stdin);
		freopen_s(&new_file, "CONOUT$", "w", stdout);
		freopen_s(&new_file, "CONOUT$", "w", stderr);
	}
}

int Helper::getConfig()
{
	return GetPrivateProfileIntA("ScriptCompiler", "EnableYDTrigger", -1, m_configPath.string().c_str());
}
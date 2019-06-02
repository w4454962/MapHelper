// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mapHelper.h"




const char* g_path;
uintptr_t g_object;
uintptr_t g_addr;

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

	enableConsole();

	m_bAttach = true;

	enableConsole();

	WorldEditor* editor = WorldEditor::getInstance();

	uintptr_t addr = editor->getAddress(0x0055CDE6);

	hook::install(&addr, (uintptr_t)&insertSaveMapData,m_hookSaveMap);

}


void Helper::detach()
{
	if (!m_bAttach) return; 
	m_bAttach = false;

	hook::uninstall(m_hookSaveMap);
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
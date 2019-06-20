// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include "mapHelper.h"
#include "WorldEditor.h"
#include "TriggerEditor.h"

Helper g_CHelper;
WorldEditor g_world_editor;
TriggerEditor g_trigger_editor;

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	auto& helper = get_helper();

	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		helper.attach();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		helper.detach();
	}
	return TRUE;
}


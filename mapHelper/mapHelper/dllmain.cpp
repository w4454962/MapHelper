// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "mapHelper.h"
#include "windows.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	Helper* helper = Helper::getInstance();

	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		helper->attatch();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		helper->detach();
	}
    return TRUE;
}


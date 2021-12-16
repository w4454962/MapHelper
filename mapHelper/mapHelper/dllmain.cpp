// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include "mapHelper.h"
#include "WorldEditor.h"
#include "TriggerEditor.h"
#include "Export.h"
#include <mmsystem.h>
#pragma comment(lib, "Winmm.lib")

Helper g_CHelper;
WorldEditor g_world_editor;
TriggerEditor g_trigger_editor;

MakeEditorData* g_make_editor_data = nullptr;

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


Action::Type get_action_type(Action* action)
{
	if (g_make_editor_data)
	{
		uint32_t type = (uint32_t)action->table;
		if (type < 5)
		{
			return static_cast<Action::Type>(type - 1);
		}
		//return static_cast<Action::Type>(g_make_editor_data->get_action_type(action));
	}
	return static_cast<Action::Type>(action->table->getType(action));
}


void ConverJassScript(MakeEditorData* data, const char* ouput_path)
{
	g_make_editor_data = data;

	auto& triggerEditor = get_trigger_editor();

	triggerEditor.loadTriggerConfig(data->config_data);
	triggerEditor.loadTriggers(data->editor_data->triggers);
	triggerEditor.saveSctipt(ouput_path);
}
 
int GetSoundPlayTime(const char* music_path)
{
	char str[0x100];
	char timebuffer[0x400];
	sprintf(timebuffer, "status %s position", music_path); 
	mciSendStringA(timebuffer, str, 0x100, NULL);
	return atoi(str);
}

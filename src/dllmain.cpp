// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include "MapHelper.h"
#include "WorldEditor.h"
#include "TriggerEditor.h"
#include "..\include\Export.h"


#include <libnyquist/Decoders.h>

#ifdef _DEBUG
#pragma comment(lib,"libnyquist_d.lib")
#pragma comment(lib,"libwavpack_d.lib")
#else 

#pragma comment(lib,"libnyquist.lib")
#pragma comment(lib,"libwavpack.lib")

#endif 


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
	//MessageBoxA(0, "A", "", MB_OK);

	g_make_editor_data = data;

	auto& triggerEditor = get_trigger_editor();

	triggerEditor.loadTriggerConfig(data->config_data);
	triggerEditor.loadTriggers(data->editor_data->triggers);
	triggerEditor.saveSctipt(ouput_path);
}
 
int GetSoundPlayTime(const char* path, const char* data, uint32_t size)
{

	nqr::NyquistIO loader;
	nqr::AudioData audio_data;
	std::vector<uint8_t> buffer(size);
	memcpy(buffer.data(), data, size);

	loader.Load(&audio_data, buffer);

	int time = (int)ceil(audio_data.lengthSeconds * 1000); //转毫秒

	return time;
}

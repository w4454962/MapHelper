#pragma once
#include "EditorData.h"

# define EXPORT extern "C" __declspec(dllexport)

extern "C" {
	//获取编辑器数据 
	typedef const char* (__stdcall* GetConfigData)(const char* parentKey, const char* childkey, int index);

	//获取技能物编数据 技能id 等级 字段 返回值
	typedef const char* (__stdcall* GetSkillObjectData)(uint32_t id, uint32_t level, const char* key);

	//获取声音文件时长
	typedef int(__stdcall* GetSoundDuration)(const char* path);


	typedef void(__stdcall* OutPrint)(const char* str);

	struct MakeEditorData
	{
		EditorData* editor_data;
		TriggerConfigData* config_data;
		GetConfigData get_config_data;
		GetSkillObjectData get_skill_data;
		GetSoundDuration get_sound_duration;
		OutPrint	out_print;
	};
}

EXPORT void ConverJassScript(MakeEditorData* data, const char* ouput_path);


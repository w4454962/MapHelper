#pragma once
#include "EditorData.h"

# define EXPORT __declspec(dllexport)

extern "C" {
	//获取编辑器数据 
	typedef const char* (__stdcall* GetConfigData)(const char* parentKey, const char* childkey, int index);

	//获取技能物编数据 技能id 等级 字段 返回值
	typedef const char* (__stdcall* GetSkillObjectData)(uint32_t id, uint32_t level, const char* key);

	//获取声音文件时长
	typedef int(__stdcall* GetSoundDuration)(const char* path);

	//判断单位是否拥有该技能 或该技能为模板的技能
	typedef bool(__stdcall* HasSkillByUnit)(uint32_t unit_id, uint32_t skill_id);

	//输出消息
	typedef void(__stdcall* OutPrint)(const char* str);

	struct MakeEditorData
	{
		EditorData*				editor_data;
		TriggerConfigData*		config_data;
		GetConfigData			get_config_data;
		GetSkillObjectData		get_skill_data;
		GetSoundDuration		get_sound_duration;
		HasSkillByUnit			has_skill_by_unit;
		OutPrint				out_print;
	};

	EXPORT void ConverJassScript(MakeEditorData* data, const char* ouput_path);

	EXPORT int GetSoundPlayTime(const char* path, const char* data, uint32_t size);
}







#pragma once

#include "stdafx.h"
#include "mapHelper.h"
#include "..\include\EditorData.h"

class TriggerEditor
{
public:
	TriggerEditor();
	~TriggerEditor();

	//static TriggerEditor* getInstance();

	void loadTriggers(TriggerData* data);
	void loadTriggerConfig(TriggerConfigData* data);

	void saveTriggers(const char* path); //生成wtg
	void saveScriptTriggers(const char* path);//生成 wct
	void saveSctipt(const char* path); //生成j

	std::string WriteRandomDisItem(const char* id); //处理物编掉落关于随机物品 随机组 四字id
	
	std::string convertTrigger(Trigger* trigger);

	TriggerType* getTypeData(const std::string& type);
	std::string getBaseType(const std::string& type) const;
	
	std::string getScriptName(Action* action);

	
	bool hasBlackAction(Trigger* trigger, bool* is_init, bool* is_disable);

	
	//we原版的t转j
	std::string originConvertTrigger(Trigger* trigger);

	//当触发编辑器转换单个触发为自定义脚本的时候
	bool onConvertTrigger(Trigger* trigger);

private:
	void writeCategoriy(BinaryWriter& writer);
	void writeVariable(BinaryWriter& writer);
	void writeTrigger(BinaryWriter& writer);
	void writeTrigger(BinaryWriter& writer,Trigger* trigger);
	void writeAction(BinaryWriter& writer, Action* action);
	void writeParameter(BinaryWriter& writer, Parameter* param);

protected:
	TriggerConfigData* m_configData;
	TriggerData* m_editorData;

	uint32_t m_version;

	const std::string seperator = "//===========================================================================\n";

	//变量类型默认的值
	std::unordered_map<std::string, TriggerType*> m_typesTable;

public:

	std::unordered_map<std::string, Variable*> variableTable;

	std::unordered_map<std::string, bool> m_initFuncTable;

	//ui黑名单 触发里有黑名单的ui 则使用原版转换
	std::unordered_map<std::string, bool> m_blacklist_map;

	std::string spaces[200];
};
extern TriggerEditor g_trigger_editor;
TriggerEditor& get_trigger_editor();
//所有物品类型的jass变量名
std::string randomItemTypes[];
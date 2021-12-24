#pragma once

#include "stdafx.h"
#include "mapHelper.h"
#include "..\include\EditorData.h"
#include "ActionNode.h"

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
	std::string convertAction(ActionNodePtr node, std::string& pre_actions, bool nested);

	std::string convertParameter(Parameter* parameter, ActionNodePtr node, std::string& pre_actions, bool add_call = false);
	std::string convertCall(ActionNodePtr node, std::string& pre_actions, bool add_call);
	
	TriggerType* getTypeData(const std::string& type);
	std::string getBaseType(const std::string& type) const;
	std::string getBaseName(ActionNodePtr node);

	std::string generate_function_name(std::shared_ptr<std::string> trigger_name) const;


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

	class YDTrigger* m_ydweTrigger;

	uint32_t m_version;

	const std::string seperator = "//===========================================================================\n";

	//变量类型默认的值
	std::unordered_map<std::string, TriggerType*> m_typesTable;
	std::unordered_map<std::string, bool> m_initFuncTable;

	std::unordered_map<Trigger*, bool> m_initTriggerTable;

public:

	std::map<std::string, Variable*> variableTable;

	std::string spaces[200];
	int space_stack;
};
extern TriggerEditor g_trigger_editor;
TriggerEditor& get_trigger_editor();
//所有物品类型的jass变量名
std::string randomItemTypes[];
#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"
#include "EditorData.h"
#include "ActionNode.h"

//专门负责重载ydwe的功能

struct LocalVar
{
	std::string name;
	std::string type;
	std::string value;
};

class YDTrigger
{
public:
	YDTrigger();
	~YDTrigger();
	static YDTrigger* getInstance();

	//引入头文件
	void onGlobals(BinaryWriter& writer);
	void onEndGlobals(BinaryWriter& writer);

	//实现任意玩家 跟触发休眠
	bool onRegisterEvent(std::string& events,ActionNodePtr node);
	void onRegisterEvent2(std::string& events,ActionNodePtr node);

	void onRegisterTrigger(std::string& output, const std::string& trigger_name, const std::string& var_name);
	//每条动作生成时
	bool onActionToJass(std::string& output,ActionNodePtr node, std::string& pre_actions, bool nested);

	//每条参数生成时
	bool onParamterToJass(Parameter* parameter, ActionNodePtr node, std::string& actions, std::string& pre_actions, bool nested);

	//当动作生成函数开始时 写入局部变量
	void onActionsToFuncBegin(std::string& funcCode, ActionNodePtr node);
	
	//当动作生成函数结束时 清除局部变量
	void onActionsToFuncEnd(std::string& funcCode, ActionNodePtr node);


	//判断触发是否休眠 true 为休眠状态
	bool hasDisableRegister(Trigger* trigger);

	bool isEnable();
private:

	
	std::string setLocal(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& value, bool add = false);
	std::string getLocal(ActionNodePtr node, const std::string& name, const std::string& type);

	std::string setLocalArray(ActionNodePtr node, const  std::string& name, const std::string& type, const std::string& index, const std::string& value);
	std::string getLocalArray(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& index);

	bool seachHashLocal(Parameter** parameters, uint32_t count, std::map<std::string, std::string>* mapPtr = NULL);
protected: 
	bool m_bEnable;
	int m_enumUnitStack;
	bool m_hasAnyPlayer;

	int m_funcStack;
	std::map<Trigger*, bool> m_triggerHasDisable;


};
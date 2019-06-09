#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"

//专门负责重载ydwe的功能

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
	bool onRegisterEvent(std::string& events, Trigger* trigger, Action* action, std::string& name);
	void onRegisterEvent2(std::string& events, Trigger* trigger, Action* action, std::string& name);

	//每条动作生成时
	bool onActionToJass(std::string& actions, Action* action, ActionNode* node, std::string& pre_actions, const std::string& trigger_name, bool nested);

	//每条参数生成时
	bool onParamterToJass(std::string& actions, Parameter* parameter, ActionNode* node, std::string& pre_actions, const std::string& trigger_name, bool nested);

	//当动作生成函数开始时 写入局部变量
	void onActionsToFuncBegin(std::string& funcCode, Trigger* trigger, ActionNode* node = NULL);
	
	//当动作生成函数结束时 清除局部变量
	void onActionsToFuncEnd(std::string& funcCode, Trigger* trigger, ActionNode* node = NULL);


	//判断触发是否休眠 true 为休眠状态
	bool hasDisableRegister(Trigger* trigger);

	bool isEnable();
private:

	void addLocalVar(std::string name,std::string type, std::string value = std::string());

	ActionNode getRootNode(ActionNode* node);

	std::string setLocal(ActionNode* node, const std::string& name, const std::string& type, const std::string& value, bool add = false);
	std::string getLocal(ActionNode* node, const std::string& name, const std::string& type);

	std::string setLocalArray(ActionNode* node, const  std::string& name, const std::string& type, const std::string& index, const std::string& value);
	std::string getLocalArray(ActionNode* node, const std::string& name, const std::string& type, const std::string& index);

	bool seachHashLocal(Parameter** parameters, uint32_t count, std::map<std::string, std::string>* mapPtr = NULL);
protected: 
	bool m_bEnable;
	bool m_isInYdweEnumUnit;
	bool m_hasAnyPlayer;
	bool m_isInMainProc;

	bool m_isFuncBegin;

	bool m_hasYdweLocal;

	int m_funcStack;
	std::map<Trigger*, bool> m_triggerHasDisable;

	struct LocalVar
	{
		std::string name;
		std::string type;
		std::string value;
	};
	//局部变量表 名字,类型,默认值
	std::vector<LocalVar> m_localTable;
	std::map<std::string, bool> m_localMap;
	


	//哈希局部变量表 名字,类型
	std::map<std::string, std::string> m_HashLocalTable;



};
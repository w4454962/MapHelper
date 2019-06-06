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

	//判断触发是否休眠 true 为休眠状态
	bool hasDisableRegister(Trigger* trigger);

	bool isEnable();
private:


protected: 
	bool m_bEnable;
	bool m_hasAnyPlayer;
	
	std::map<Trigger*, bool> m_triggerHasDisable;
};
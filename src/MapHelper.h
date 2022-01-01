#pragma once
#include "stdafx.h"
#include <base\hook\inline.h>
#include <include\EditorData.h>
#include "WorldEditor.h"

template<typename dst_type, typename src_type>
dst_type union_cast(src_type src)
{
	union {
		src_type s;
		dst_type d;
	}u;
	u.s = src;
	return u.d;
}


struct ActionInfo
{
	int type_id;
	std::string name;
};

typedef std::vector<ActionInfo> ActionInfoList;
typedef std::map<std::string, ActionInfoList> ActionInfoMap;

extern ActionInfoMap g_actionInfoTable;

extern HMODULE g_hModule;

extern fs::path g_module_path;

class Helper
{
public:
	Helper();
	~Helper();
	
	//static Helper* getInstance();


	static bool IsEixt();

	void enableConsole();

	void attach();//附加

	void detach();//分离

	int getConfig() const;
private:

	//保存地图
	static uintptr_t onSaveMap();

	//选择转换模式
	int onSelectConvartMode();

	//当自定义转换触发时
	int onConvertTrigger(Trigger* trg);

public:
	fs::path ydwe_path;

protected:
	bool m_bAttach;

	//自定义jass生成器的hook
	hook::hook_t* m_hookSaveMap;
	hook::hook_t* m_hookConvertTrigger;

	//动态参数类型 返回类型的hook
	hook::hook_t* m_hookCreateUI;
	hook::hook_t* m_hookReturnTypeStrcmp;

	//自定义动作组的hook
	hook::hook_t* m_hookGetChildCount;
	hook::hook_t* m_hookGetString;
	hook::hook_t* m_hookGetActionType;
	
	//角度弧度互换的hook操作
	hook::hook_t* m_hookParamTypeStrncmp1;
	hook::hook_t* m_hookParamTypeStrncmp2;
	hook::hook_t* m_hookGetParamType;


	fs::path m_configPath;
};
extern Helper g_CHelper;
Helper& get_helper();

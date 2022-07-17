#pragma once
#include "stdafx.h"
#include <base\hook\inline.h>
#include <include\EditorData.h>
#include "WorldEditor.h"


struct ActionInfo
{
	int type_id;
	std::string name;
};


void SetActionToTextBufferSize(int size);


typedef std::vector<ActionInfo> ActionInfoList;
typedef std::map<std::string, ActionInfoList> ActionInfoMap;

extern ActionInfoMap g_actionInfoTable;

extern HMODULE g_hModule;

extern fs::path g_module_path;

extern std::vector<HWND> g_editor_windows;


class Helper
{
public:
	Helper();
	~Helper();
	
	//static Helper* getInstance();

	enum CONFIG :uint32_t {

		ENABLE_PLUGIN = 1 << 1,

		//加速保存
		SUPPER_SPEED_SAVE = 1 << 2,

		//增量资源
		INCRE_RESOURCE = 1 << 3,

		//显示控制台
		SHOW_CONSOLE = 1 << 4,
	};

	uint32_t getConfig();

	void setConfig(uint32_t config);

	void updateState();

	void enableConsole();


	void attach();//附加

	void detach();//分离


	void setMenuEnable(bool is_enable);
private:


	//保存地图
	static uintptr_t onSaveMap();

	//选择转换模式
	int onSelectConvartMode();

	//当自定义转换触发时
	int onConvertTrigger(Trigger* trg);



	uint32_t m_config = 0;
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

	//hook we的弹框 弹框的时候禁用所有菜单
	uintptr_t m_hookMessageBoxA;

	hook::hook_t* m_hookInsertActionToText;
	hook::hook_t* m_hookInitWindows;

	fs::path m_configPath;
};
extern Helper g_CHelper;
Helper& get_helper();

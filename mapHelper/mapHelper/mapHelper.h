#pragma once
#include "stdafx.h"
#include "inline.h"
#include "EditorData.h"
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


class Helper
{
public:
	Helper();
	~Helper();
	
	static Helper* getInstance();

	void enableConsole();

	void attach();//附加

	void detach();//分离

	int getConfig();
private: 

	//保存地图
	uintptr_t onSaveMap();

	//选择转换模式
	int onSelectConvartMode();

	//当自定义转换触发时
	int onConvertTrigger(Trigger* trg);

protected:
	bool m_bAttach;

	hook::hook_t* m_hookSaveMap;
	hook::hook_t* m_hookConvertTrigger;

	fs::path m_configPath;
};

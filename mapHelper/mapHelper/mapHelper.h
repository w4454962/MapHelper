#pragma once
#include "stdafx.h"
#include "inline.h"
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

	void attatch();//∏Ωº”

	void detach();//∑÷¿Î

private: 

	uintptr_t onSaveMap();

protected:
	bool m_bAttach;

	hook::hook_t* m_hookSaveMap;

};

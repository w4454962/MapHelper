#pragma once
#include "stdafx.h"

#include <base\hook\inline.h>


class YDPluginManager {
public:
	YDPluginManager(); 

	~YDPluginManager();


	void attach();

	void detach();

	void extract();


	std::map<std::wstring, std::wstring> m_plugins_path;;

	bool m_enable; 

private:
	bool m_attach;
	bool m_extract;

protected:

	struct HookInfo {
		uintptr_t real;
		HMODULE module;
		std::string dll_name;
		std::string api_name;
	};

	std::vector<HookInfo> m_hookCreateProcessW;
};


YDPluginManager& get_ydplugin_manager();

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

	void on_save_event(const std::string& map_path, bool is_test);

	std::map<std::wstring, std::wstring> m_plugins_path;;

	bool m_enable; 

private:
	bool m_attach;
	bool m_extract;

protected:

	struct HookInfo {
		uintptr_t real;
		std::string module_name;
		std::string dll_name;
		std::string api_name;
	};

	std::vector<HookInfo> m_hook_list;

	
};


YDPluginManager& get_ydplugin_manager();

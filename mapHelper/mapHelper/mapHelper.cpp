// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "mapHelper.h"
#include "singleton.h"

#include "json.hpp"
#include <sstream>

#pragma warning(disable:4996)

const char* g_path;
static uintptr_t g_object;
static uintptr_t g_addr;

namespace real
{
	uintptr_t convertTrigger;
	uintptr_t createUI;
	uintptr_t setParamType;
	uintptr_t getChildCount;
	uintptr_t getString;
	uintptr_t getWEString;
	uintptr_t getActionType;
}




struct ActionInfo
{
	int type_id;
	std::string name;
};

typedef std::vector<ActionInfo> ActionInfoList;
typedef std::map<std::string, ActionInfoList> ActionInfoMap;

ActionInfoMap g_actionInfoTable;



Helper::Helper()
	: m_bAttach(false)
{
}

Helper::~Helper()
= default;

//Helper* Helper::getInstance()
//{
//	static Helper instance;
//	return &instance;
//}


static void __declspec(naked) insertSaveMapData()
{
	
	__asm
	{
		mov g_object, esi
		lea eax, dword ptr ss : [ebp - 0x10c];
		mov g_path, eax
		pushad
		pushfd
		call get_helper
		mov ecx, eax
	
		call Helper::onSaveMap
		mov g_addr, eax
		popfd
		popad
		jmp g_addr
	}
}

static void __declspec(naked) insertConvertTrigger()
{
	__asm
	{
		mov g_object, ecx
		call get_helper
		mov ecx, eax
		call Helper::onSelectConvartMode
		test eax,eax 
		je pos

		mov ecx,g_object
		jmp real::convertTrigger
		
	pos:

		pushad
		pushfd 
		call get_helper
		mov ecx,eax
		push g_object
		call Helper::onConvertTrigger
		mov g_object,eax 
		popfd
		popad 
		mov eax,g_object
		ret 0x4
	}
	

}


//修改特定UI的子动作数量
static int __fastcall fakeGetChildCount(Action* action)
{
	auto it = g_actionInfoTable.find(std::string(action->name));
	if (it != g_actionInfoTable.end())
	{
		return it->second.size();
	}
	return this_call<int>(real::getChildCount, action);
}

//修改特定的UI名字
static int __fastcall fakeGetString(Action* action, uint32_t edx,int index, char* buffer, int len)
{
	auto it = g_actionInfoTable.find(std::string(action->name));
	if (it == g_actionInfoTable.end())
	{
		return fast_call<int>(real::getString, action, edx, index, buffer, len);
		
	}
	return fast_call<int>(real::getWEString, it->second[index].name.c_str(), buffer, len, 0);
}


static int __fastcall fakeGetActionType(Action* action, uint32_t edx, int index)
{
	auto it = g_actionInfoTable.find(std::string(action->name));
	if (it == g_actionInfoTable.end())
	{
		return fast_call<int>(real::getActionType, action, edx, index);
	}
	index = (std::min)(index, (int)it->second.size() - 1);
	return it->second[index].type_id;
}

//根据指定参数值 修改目标参数类型
static void setParamerType(Action* action, int flag, int type_param_index, int target_param_index)
{
	Parameter* param = action->parameters[type_param_index];
	if (!param || strncmp(param->value,"typename_",8) != 0)
		return;
	const char* type = param->value + 11; //typename_01_integer  + 11 = integer

	//printf("将 %s 第%i个参数类型修改为 %s\n",action->name, target_param_index, type);
	this_call<int>(real::setParamType, action, target_param_index, type, flag);
}

//插入到创建UI中 根据动作参数改变对应UI类型
static void __fastcall insertCreateUI(Action* action,uint32_t edx, int flag)
{
	
	switch (hash_(action->name))
	{
	case "YDWESetAnyTypeLocalVariable"s_hash:
		setParamerType(action, flag, 0, 2);
		break;
	case "YDWESetAnyTypeLocalArray"s_hash:
		setParamerType(action, flag, 0, 3);
		break;
	case "YDWESaveAnyTypeDataByUserData"s_hash:
		setParamerType(action, flag, 0, 1);
		setParamerType(action, flag, 3, 4);

		break;
	case "YDWELoadAnyTypeDataByUserData"s_hash:
	case "YDWEHaveSavedAnyTypeDataByUserData"s_hash:
	case "YDWEFlushAnyTypeDataByUserData"s_hash:
	case "YDWEFlushAllByUserData"s_hash:
		setParamerType(action, flag, 0, 1);
		break;
	case "YDWEGetObjectPropertyInteger"s_hash:
	case "YDWEGetObjectPropertyReal"s_hash:
	case "YDWEGetObjectPropertyString"s_hash:
		setParamerType(action, flag, 0, 1);
		break;
	}

	this_call<int>(real::createUI, action, flag);
}

uintptr_t Helper::onSaveMap()
{
	auto& editor = get_world_editor();
	editor.onSaveMap(g_path);
	return editor.getAddress(0x0055D175);
}


void Helper::attach()
{
	if (m_bAttach) return;

	char buffer[0x400];

	GetModuleFileNameA(nullptr, buffer, 0x400);

	std::string name = fs::path(buffer).filename().string();
	if (name.find("worldedit") == std::string::npos)
		return;
	
	GetModuleFileNameA(GetModuleHandleA("ydbase.dll"), buffer, 0x400);
	
	m_configPath = fs::path(buffer).remove_filename() / "EverConfig.cfg";

	m_bAttach = true;


	auto& editor = get_world_editor();

	//当保存或测试地图时保存生成数据
	uintptr_t addr = editor.getAddress(0x0055CDE6);
	hook::install(&addr, reinterpret_cast<uintptr_t>(&insertSaveMapData),m_hookSaveMap);


	//当t转j时 生成脚本
	real::setParamType = editor.getAddress(0x005D7C00);
	real::convertTrigger = editor.getAddress(0x005CB4C0);
	hook::install(&real::convertTrigger, reinterpret_cast<uintptr_t>(&insertConvertTrigger), m_hookConvertTrigger);


	//当创建触发器UI时 修改指定类型
	real::createUI = editor.getAddress(0x005D82C0);
	hook::install(&real::createUI, reinterpret_cast<uintptr_t>(&insertCreateUI), m_hookCreateUI);

	
	
	real::getChildCount = editor.getAddress(0x005DAE20);
	hook::install(&real::getChildCount, reinterpret_cast<uintptr_t>(&fakeGetChildCount), m_hookGetChildCount);
	

	real::getWEString = editor.getAddress(0x004EEC00);
	real::getString = editor.getAddress(0x005DAEE0);
	hook::install(&real::getString, reinterpret_cast<uintptr_t>(&fakeGetString), m_hookGetString);


	real::getActionType = editor.getAddress(0x005DAE70);
	hook::install(&real::getActionType, reinterpret_cast<uintptr_t>(&fakeGetActionType), m_hookGetActionType);

#if !defined(EMBED_YDWE)
	if (getConfig() == -1)
	{
		enableConsole();
	}
#endif


	//std::ifstream file("D:\\war3\\test.json",std::ios::binary);
	//
	//if (!file.is_open())
	//{
	//	return;
	//}
	//
	//
	//std::stringstream stream;
	std::string text;
	std::string error;
	//
	//stream << file.rdbuf();
	//
	//text = stream.str();
	//
	//file.close();

	text = R"(
{
    "YDWETimerStartMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],

    "YDWERegisterTriggerMultiple" : [
        { "Event" : "WESTRING_EVENTS" },
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],

    "YDWEEnumUnitsInRangeMultiple" : [
        { "Action" : "WESTRING_ACTIONS" }
    ],

    "YDWEForLoopLocVarMultiple" : [
        { "Action" : "WESTRING_TRIGSUBFUNC_FORLOOPACTIONS" }
    ],

    "YDWERegionMultiple" : [
        { "Action" : "WESTRING_ACTIONS" }
    ],

    "YDWEExecuteTriggerMultiple" : [
        { "Action" : "WESTRING_ACTIONS" }
    ]
}
)";

	using json::Json;

	Json json = Json::parse(text, error);

	auto items = json.object_items();

	for (auto&[str, child] : items)
	{

		if (child.is_array())
		{
			if (g_actionInfoTable.find(str) == g_actionInfoTable.end())
				g_actionInfoTable[str] = ActionInfoList();

			auto& mul_list = g_actionInfoTable[str];

			auto list = child.array_items();
			for (int i = 0; i < list.size(); i++)
			{
				auto& item = list[i].object_items();

				for (auto& [key, value] : item)
				{
					std::string s = key;
					transform(s.begin(), s.end(), s.begin(), ::tolower);
					int type_id = -1;
					switch (hash_(s.c_str()))
					{
					case "event"s_hash:
						type_id = Action::Type::event;
						break;
					case "condition"s_hash:
						type_id = Action::Type::condition;
						break;
					case "action"s_hash:
						type_id = Action::Type::action;
						break;
					}
					if (type_id != -1)
					{
						mul_list.push_back(ActionInfo({ type_id,value.string_value() }));
					}
				}

			}
		}
	}

	for (auto& [name,list] : g_actionInfoTable)
	{
		for (auto& value : list)
		{
			std::cout << name << "  :  " << value.type_id << "  " << value.name << "\n";
		}
	}
}

int Helper::onSelectConvartMode()
{
#if defined(EMBED_YDWE)
	return 0;
#else
	int result = getConfig();
	if (result == -1)
	{
		int ret = MessageBoxA(0, "是否用新的保存模式保存?", "问你", MB_YESNO);

		if (ret == 6)
		{
			printf("自定义保存模式\n");
			return 0;
		}
		printf("原始保存模式\n");
		return 1;
	}
	else
	{
		if (result == 1)
		{
			return 0;
		}
		return 1;
	}
#endif
}


void Helper::detach()
{
	if (!m_bAttach) return;
	m_bAttach = false;

	hook::uninstall(m_hookSaveMap);
	hook::uninstall(m_hookConvertTrigger);
	hook::uninstall(m_hookCreateUI);
	hook::uninstall(m_hookGetChildCount);
	hook::uninstall(m_hookGetString);
	hook::uninstall(m_hookGetActionType);
	

#if !defined(EMBED_YDWE)
	//释放控制台避免崩溃
	FreeConsole();
#endif
}



int Helper::onConvertTrigger(Trigger* trigger)
{
	auto& v_we = get_world_editor();
	TriggerData* data = v_we.getEditorData()->triggers;
	auto& editor = get_trigger_editor();
	editor.loadTriggers(data);

	return editor.onConvertTrigger(trigger) ? 1 : 0;
}

Helper& get_helper()
{
	return base::singleton<Helper>::instance();
}


void Helper::enableConsole()
{
	HWND v_hwnd_console = ::GetConsoleWindow();

	if (nullptr != v_hwnd_console)
	{
		::ShowWindow(v_hwnd_console, SW_SHOW);

	}
	else
	{	
		auto v_is_ok = AllocConsole();
		if (v_is_ok)
		{
			FILE* fp;
			//freopen_s(&fp, "CONOUT$", "r", stdin);
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONOUT$", "w", stderr);
			fclose(fp);
			std::cout.clear();
		}
	}
	v_hwnd_console = ::GetConsoleWindow();
	if (v_hwnd_console)
	{
		::DeleteMenu(::GetSystemMenu(v_hwnd_console, FALSE), SC_CLOSE, MF_BYCOMMAND);
		::DrawMenuBar(v_hwnd_console);
		::SetWindowTextA(v_hwnd_console, "ydwe保存加速插件 1.0b");
		std::cout
			<< "用来加速ydwe保存地图的插件，对地形装饰物，触发编辑器极速优化\n"
			<< "参与开发者 ：w4454962、 神话、 actboy168\n"
			<< "参与测试人员： 幽影、夜夜、七罪、五爷、妖精\n"
			<< "排名不分先后，为魔兽地图社区的贡献表示感谢。\n"
			<< "                         ----2019/07/09\n";

	}
}

int Helper::getConfig() const
{
	return GetPrivateProfileIntA("ScriptCompiler", "EnableYDTrigger", -1, m_configPath.string().c_str());
}

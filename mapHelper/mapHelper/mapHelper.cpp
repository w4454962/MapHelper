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
static uintptr_t g_object{};
static uintptr_t g_addr;

namespace real
{
	uintptr_t ConvertTrigger;

	uintptr_t CreateUI;
	uintptr_t SetParamType;

	uintptr_t ReturnTypeStrcmp;
	uintptr_t ReturnTypeStrcmpEnd;

	uintptr_t GetChildCount;
	uintptr_t GetString;
	uintptr_t GetWEString;
	uintptr_t GetActionType;
	
	uintptr_t ParamTypeStrncmp1;
	uintptr_t ParamTypeStrncmpEnd1;

	uintptr_t ParamTypeStrncmp2;
	uintptr_t ParamTypeStrncmpEnd2;

	uintptr_t GetParamType;

	uintptr_t ParamTypeId = 0;
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
		jmp real::ConvertTrigger
		
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
	return this_call<int>(real::GetChildCount, action);
}

//修改特定的UI名字
static int __fastcall fakeGetString(Action* action, uint32_t edx,int index, char* buffer, int len)
{
	auto it = g_actionInfoTable.find(std::string(action->name));
	if (it == g_actionInfoTable.end())
	{
		return fast_call<int>(real::GetString, action, edx, index, buffer, len);
		
	}
	return fast_call<int>(real::GetWEString, it->second[index].name.c_str(), buffer, len, 0);
}

//返回动作组的动作类型
static int __fastcall fakeGetActionType(Action* action, uint32_t edx, int index)
{
	auto it = g_actionInfoTable.find(std::string(action->name));
	if (it == g_actionInfoTable.end())
	{
		return fast_call<int>(real::GetActionType, action, edx, index);
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
	this_call<int>(real::SetParamType, action, target_param_index, type, flag);
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

	this_call<int>(real::CreateUI, action, flag);
}

//任意返回类型的操作
static int __fastcall fakeReturnTypeStrcmp(const char* type1,const char* type2)
{
	//类型相等 或者type1 是任意类型 即返回字符串相同的结果
	if (strcmp(type1, type2) == 0 || strcmp(type1, "AnyReturnType") == 0)
	{
		return 0;
	}
	return 1;
}

static int __declspec(naked) insertReturnTypeStrcmp()
{
	__asm
	{
		lea  edx, [esi + 64h]
		mov  ecx, eax
		call fakeReturnTypeStrcmp
		jmp real::ReturnTypeStrcmpEnd
	}
}

static int __stdcall fakeParamTypeStrncmp1(const char* type1, const char* type2, size_t size)
{
	if (strcmp(type1, "degree") == 0)
	{
		real::ParamTypeId = 1;
		return 0;
	}
	else if (strcmp(type1, "radian") == 0)
	{
		real::ParamTypeId = 2;
		return 0;
	}
	return strcmp(type1, type2);
}
static int __stdcall fakeParamTypeStrncmp2(const char* type1, const char* type2, size_t size)
{
	if (real::ParamTypeId != 0)
	{
		real::ParamTypeId = 0;
		return 0;
	}
	return strcmp(type1, type2);
}
static void __declspec(naked) insertParamTypeStrncmp1()
{
	__asm
	{
		call fakeParamTypeStrncmp1
		jmp real::ParamTypeStrncmpEnd1
	}
}
static void __declspec(naked) insertParamTypeStrncmp2()
{
	__asm
	{
		call fakeParamTypeStrncmp2
		jmp real::ParamTypeStrncmpEnd2
	}
}

static int __fastcall fakeGetParamType(
	uintptr_t pThis,
	uint32_t edx,
	const char* type,
	uint32_t unk1,
	uint32_t unk2,
	uint32_t unk3)
{
#define get(str) fast_call<int>(real::GetParamType,pThis,edx,str,unk1,unk2,unk3)

	if (real::ParamTypeId == 1)
	{
		return get("degree");
	}
	else if (real::ParamTypeId == 2)
	{
		return get("radian");
	}
	return get(type);
#undef get
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

	//------------------自定义脚本生成器的操作----------------

	//当保存或测试地图时保存生成数据
	uintptr_t addr = editor.getAddress(0x0055CDE6);
	hook::install(&addr, reinterpret_cast<uintptr_t>(&insertSaveMapData),m_hookSaveMap);


	//当t转j时 生成脚本
	real::SetParamType = editor.getAddress(0x005D7C00);
	real::ConvertTrigger = editor.getAddress(0x005CB4C0);
	hook::install(&real::ConvertTrigger, reinterpret_cast<uintptr_t>(&insertConvertTrigger), m_hookConvertTrigger);

	//---------------------end-----------------------------



	//------------------动态参数类型 以及 返回类型的操作----------------

	//当创建触发器UI时 修改指定类型
	real::CreateUI = editor.getAddress(0x005D82C0);
	hook::install(&real::CreateUI, reinterpret_cast<uintptr_t>(&insertCreateUI), m_hookCreateUI);


	real::ReturnTypeStrcmp = editor.getAddress(0x00688B0D);
	real::ReturnTypeStrcmpEnd = editor.getAddress(0x00688B1C);
	hook::install(&real::ReturnTypeStrcmp, reinterpret_cast<uintptr_t>(&insertReturnTypeStrcmp), m_hookReturnTypeStrcmp);

	//---------------------end-----------------------------



	//-----------自定义动作组 所需的 hook操作---------------

	//hook获取子动作数量
	real::GetChildCount = editor.getAddress(0x005DAE20);
	hook::install(&real::GetChildCount, reinterpret_cast<uintptr_t>(&fakeGetChildCount), m_hookGetChildCount);
	
	//hook获取动作组的名字
	real::GetWEString = editor.getAddress(0x004EEC00);
	real::GetString = editor.getAddress(0x005DAEE0);
	hook::install(&real::GetString, reinterpret_cast<uintptr_t>(&fakeGetString), m_hookGetString);

	//hook获取动作组类型 事件 条件 动作
	real::GetActionType = editor.getAddress(0x005DAE70);
	hook::install(&real::GetActionType, reinterpret_cast<uintptr_t>(&fakeGetActionType), m_hookGetActionType);

	//-------------------end-----------------------------


	//-----------弧度角度之间互相转换的操作---------------
	real::ParamTypeStrncmp1 = editor.getAddress(0x0068C391);
	real::ParamTypeStrncmpEnd1 = editor.getAddress(0x0068C396);
	hook::install(&real::ParamTypeStrncmp1, reinterpret_cast<uintptr_t>(&insertParamTypeStrncmp1), m_hookParamTypeStrncmp1);

	real::ParamTypeStrncmp2 = editor.getAddress(0x0068778E);
	real::ParamTypeStrncmpEnd2 = editor.getAddress(0x00687793);
	hook::install(&real::ParamTypeStrncmp2, reinterpret_cast<uintptr_t>(&insertParamTypeStrncmp2), m_hookParamTypeStrncmp2);

	real::GetParamType = editor.getAddress(0x00687650);
	hook::install(&real::GetParamType, reinterpret_cast<uintptr_t>(&fakeGetParamType), m_hookGetParamType);

	//-------------------end-----------------------------
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
    ],

    "DzTriggerRegisterMouseEventMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
	],

    "DzTriggerRegisterKeyEventMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],
    "DzTriggerRegisterMouseMoveEventMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],
    "DzTriggerRegisterMouseWheelEventMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],
    "DzTriggerRegisterWindowResizeEventMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],
    "DzFrameSetUpdateCallbackMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
        { "Action" : "WESTRING_ACTIONS" }
    ],
    "DzFrameSetScriptMultiple" : [
        { "Action" : "WESTRING_PARAMETERS" },
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
			//std::cout << name << "  :  " << value.type_id << "  " << value.name << "\n";
		}
	}
}



void Helper::detach()
{
	if (!m_bAttach) return;
	m_bAttach = false;

	hook::uninstall(m_hookSaveMap);
	hook::uninstall(m_hookConvertTrigger);

	hook::uninstall(m_hookCreateUI);
	hook::uninstall(m_hookReturnTypeStrcmp);

	hook::uninstall(m_hookGetChildCount);
	hook::uninstall(m_hookGetString);
	hook::uninstall(m_hookGetActionType);

	hook::uninstall(m_hookParamTypeStrncmp1);
	hook::uninstall(m_hookParamTypeStrncmp2);
	hook::uninstall(m_hookGetParamType);
	


#if !defined(EMBED_YDWE)
	//释放控制台避免崩溃
	FreeConsole();
#endif
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
		::SetWindowTextA(v_hwnd_console, "ydwe保存加速插件 1.0C");
		std::cout
			<< "用来加速ydwe保存地图的插件，对地形装饰物，触发编辑器极速优化\n"
			<< "参与开发者 ：w4454962、 神话、 actboy168、月升朝霞 \n"
			<< "参与测试人员： 幽影、夜夜、七罪、五爷、妖精、tom、迷失\n"
			<< "感谢7佬的最初版本\n"
			<< "排名不分先后，为魔兽地图社区的贡献表示感谢。\n"
			<< "                         ----2021/01/05\n"
			<< "修复逆天类型检测没有的bug\n"
			<< "修复播放装饰物动作，装饰物未加上 '' 的bug\n"
			<< "修复地形装饰物掉落含空物品 会导致保存错误的bug\n"
			<< "修复部分电脑上保存不会生成地图的bug\n"
			<< "修复t中字符串 \"\" 保存未生成转义符号\"\n"
			<< "修复布尔值表达式未加上Condition的bug？\n"
			<< "修复逆天触发器不论在任何情况都会自动传递获取触发单位等变量的bug\n"
			<< "修复bj优化失效的bug\n"
			<< "修复特定情况下逆天触发器自动传参失败的bug\n"
			<< "修复未设置过地图镜头范围会导致保存设置镜头范围为0的bug\n"
			<< "修复we随机组，物品组生成错误代码的bug\n"
			<< "修复了字符串类型StringExt传参的bug(这个和string一样的东西意义到底在哪里啊)\n"
			<< "类型检测有点问题，果然不能偷懒，类型检测有点严格，懒得修成yd那样了\n"
			<< "修复创建可破坏未转化弧度为角度的bug\n"
			<< "!!!!\n"
			<< "因为不确定bug修没修完，保存请另存为改名字。或先备份一份地图\n"
			<< "如需关闭控制台，请在ydwe目录下的 bin\\EverConfig.cfg 中修改[ScriptCompiler]项下加入EnableYDTrigger = 1\n"
			<< "EnableYDTrigger = -1 为默认开启控制台与对话框\n"
			<< "EnableYDTrigger = 0 为使用原本的保存方式\n"
			<< "EnableYDTrigger = 1 默认开启加速保存\n";

	}
}

int Helper::getConfig() const
{
	return GetPrivateProfileIntA("ScriptCompiler", "EnableYDTrigger", -1, m_configPath.string().c_str());
}

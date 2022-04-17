// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "mapHelper.h"


#include <sstream>
#include <base\hook\fp_call.h>
#include <base\util\json.hpp>
#include <include\Export.h>
#include "..\resource.h"
#include <YDPluginManager.h>
#include <HashTable.hpp>
#include <Commctrl.h>
#include <base\hook\iat.h>
#include <base/util/colored_cout.h>
extern MakeEditorData* g_make_editor_data;

#pragma warning(disable:4996)
//extern YDJassHelperPatch* g_vj_patch;

const char* g_path;
static uintptr_t g_object = 0;
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

	uintptr_t MessageBoxA;

	uintptr_t SaveMapState;

	uintptr_t ParamToTextAppend;
}


ActionInfoMap g_actionInfoTable = {
	{"YDWETimerStartMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"YDWERegisterTriggerMultiple" , {
		{ Action::Type::event , "WESTRING_EVENTS" },
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"YDWEEnumUnitsInRangeMultiple" , {
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"YDWEForLoopLocVarMultiple" , {
		{ Action::Type::action , "WESTRING_TRIGSUBFUNC_FORLOOPACTIONS" }
	},},

   {"YDWERegionMultiple" , {
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"YDWEExecuteTriggerMultiple" , {
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"DzTriggerRegisterMouseEventMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"DzTriggerRegisterKeyEventMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},
   {"DzTriggerRegisterMouseMoveEventMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},
   {"DzTriggerRegisterMouseWheelEventMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},
   {"DzTriggerRegisterWindowResizeEventMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},
   {"DzFrameSetUpdateCallbackMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},

   {"DzFrameSetScriptMultiple" , {
		{ Action::Type::action , "WESTRING_PARAMETERS" },
		{ Action::Type::action , "WESTRING_ACTIONS" }
	},},
};


std::unordered_map<std::string, std::string> g_typenames;

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
int __fastcall fakeGetChildCount(Action* action)
{
	if (g_make_editor_data)
	{
		return action->fake_child_group_count;
	}

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


	if (!param || !param->value) {
		return;
	}
		
	const char* pos = strchr(param->value, '_');
	if (!pos) {
		return;
	}
	const char* type = pos + 1;

	//print("将 %s 第%i个参数类型修改为 %s\n",action->name, target_param_index, type);
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
	if (strcmp(type1, type2) == 0 || (strcmp(type1, "AnyReturnType") == 0 && g_typenames.find(type2) != g_typenames.end()))
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


static int WINAPI fakeMessageBoxA(HWND hwnd, const char* message, const char* title, int flag)
{
	auto& helper = get_helper();

	helper.setMenuEnable(false);
	int ret = base::std_call<int>(real::MessageBoxA, hwnd, message, title, flag);
	helper.setMenuEnable(true);
	return ret;
}


static int buffer_size = 0x104;
void SetParamToTextBufferSize(int size) {
	auto addr_list = {
		0x005D7105,
		0x005D71DE,
		0x005D71E4,
		0x0065AC85,
		0x0065ACCF,
		0x0065ACD5,
		0x0065ACE1,
		0x0065AE26,
		0x0065AE2C,
		0x0065AE3B,
		0x0065AE41,
		0x0065AE57,
		0x005d71fa,
	};

	for (auto addr : addr_list) {
		auto ptr = WorldEditor::getAddress(addr);
		DWORD null;
		DWORD old_value;

		VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, (DWORD*)&old_value);

		int& num = *(int*)ptr;
		if (num < 0) {
			num = num + buffer_size - size;
		} else {
			num = num - buffer_size + size;
		}
		VirtualProtect((void*)addr, 4, old_value, (DWORD*)&null);
	}
	buffer_size = size;
}

static char* WINAPI BlzStrncat(char* dest, char* src, int size) {
	size_t dest_len = strlen(dest);
	size_t append_len = strlen(src);
	if (dest_len + append_len > size) {
		append_len = size - dest_len;
	}
	return strncat(dest, src, append_len);
}


static int WINAPI fakeParamToTextAppend(Action* action, char* buffer, char* str, int size) {
	
	auto& editor = get_trigger_editor();
	auto& world = get_world_editor();
	
	if (!editor.is_convert) {
		BlzStrncat(buffer, str, size);
		return 1;
	}
	std::string head = buffer;
	 
	if (head == "loc_") {
		auto it = editor.m_param_action_parent_map.find(action);
		if (it != editor.m_param_action_parent_map.end()) {
			auto parent_parameter = it->second;
			auto type = parent_parameter->type_name;
			auto base = editor.getBaseType(type);
			auto type_name = world.getConfigData("TriggerTypes", type, 3);
			if (editor.action_to_text_key != str) {
				return snprintf(buffer, size, "<cyan>[%s]%s%s</cyan>", type_name.c_str(), head.c_str(), str);
			}
			return snprintf(buffer, size, "<red bd='blue'>[%s]</red><red>%s%s</red>", type_name.c_str(), head.c_str(), str);
		} 
	} else if (head.length() > 4 && head.substr(head.length() - 4, 4) == "loc_") {
		switch (hash_(action->name)) {
		case "YDWESetAnyTypeLocalVariable"s_hash:
		case "YDWESetAnyTypeLocalArray"s_hash:
		{
			auto type = action->parameters[0]->value + 11;
			auto base = editor.getBaseType(type);
			auto type_name = world.getConfigData("TriggerTypes", type, 3);
			buffer = buffer + head.length() - 4;
			size = size - head.length() - 4;
			head = "loc_";
			if (editor.action_to_text_key != str) {
				return snprintf(buffer, size, "<cyan>[%s]%s%s</cyan>", type_name.c_str(), head.c_str(), str);
			}
			return snprintf(buffer, size, "<red bd='blue'>[%s]</red><red>%s%s</red>", type_name.c_str(), head.c_str(), str);
		}
		default:
			break;
		}
	}
	
	BlzStrncat(buffer, str, size);
	return 1;
}

static void __declspec(naked) insertParamToTextAppend() {
	__asm
	{
		mov eax, dword ptr ss : [ebp - 0xC]
		push eax 
		call fakeParamToTextAppend

		jmp real::ParamToTextAppend
	
	}
}

//迭代器 遍历TriggerParams 
static void initTypeName() {

	auto config = mh::get_config_table();
	//遍历所有配置数据
	//for (auto& node : *config) {
	//	for (auto& subnode : node.subtable) {
	//		printf("[%s][%s]\n", node.key, subnode.key);
	//	}
	//}

	auto trigger_params = config->find("TriggerParams");
	
	auto& editor = get_trigger_editor();

	//遍历 TriggerParams 里面 关于 typename objecttype 的动态类型
	for (auto& subnode : *trigger_params) {
		
		if (!subnode.data) {
			continue;
		}

		const char* pos = strchr(subnode.key, '_');
		if (!pos) {
			continue;
		}
		const char* type = pos + 1;

		if (!editor.getTypeData(type)) {
			continue;
		}
		g_typenames.emplace(type, subnode.key);
		//printf("[%s] = %s\n", subnode.key, subnode.data->value);
	}
}






uintptr_t Helper::onSaveMap()
{
	auto& editor = get_world_editor();

	editor.onSaveMap(g_path, (EditorData*)g_object);
	return editor.getAddress(0x0055D175);
}


void Helper::attach()
{
	if (m_bAttach) return;

	char buffer[MAX_PATH];

	GetModuleFileNameA(nullptr, buffer, MAX_PATH);

	std::string name = fs::path(buffer).filename().string();

	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	if (std::string::npos == name.find("worldedit")) {

		return;
	}
		
		
	
	GetModuleFileNameA(GetModuleHandleA("ydbase.dll"), buffer, MAX_PATH);
	
	m_configPath = fs::path(buffer).remove_filename() / "EverConfig.cfg";

	ydwe_path = m_configPath.parent_path().parent_path();

	m_bAttach = true;

	GetModuleFileNameA(g_hModule, buffer, MAX_PATH);

	g_module_path = fs::path(buffer);



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


	real::MessageBoxA = base::hook::iat(GetModuleHandleA(nullptr), "user32.dll", "MessageBoxA", (uintptr_t)&fakeMessageBoxA);

	real::ParamToTextAppend = editor.getAddress(0x005d7200);
	hook::install(&real::ParamToTextAppend, reinterpret_cast<uintptr_t>(&insertParamToTextAppend), m_hookInsertParamToText);
	real::ParamToTextAppend = editor.getAddress(0x005d7205);
	//-------------------end-----------------------------
#if !defined(EMBED_YDWE)
	if (getConfig() == -1)
	{
		enableConsole();
	}
#endif


	editor.loadConfigData();
	
	initTypeName();

	auto& manager = get_ydplugin_manager();

	manager.extract();

	
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
	
	base::hook::iat(GetModuleHandleA(nullptr), "kernel32.dll", "MessageBoxA", real::MessageBoxA);
	
	hook::uninstall(m_hookInsertParamToText);

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

		setMenuEnable(false);

		int ret = MessageBoxA(0, "是否用新的保存模式保存?", "七佬的加速器", MB_SYSTEMMODAL | MB_YESNO);

		setMenuEnable(true);

		if (ret == 6)
		{
			print("自定义保存模式\n");
			return 0;
		}
		print("原始保存模式\n");
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
	static Helper instance;
	return instance;
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
		
		  
		DWORD mode;
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		GetConsoleMode(hStdin, &mode);
		mode &= ~ENABLE_QUICK_EDIT_MODE;  //移除快速编辑模式
		mode &= ~ENABLE_INSERT_MODE;      //移除插入模式
		mode &= ~ENABLE_MOUSE_INPUT;
		SetConsoleMode(hStdin, mode);
		::DeleteMenu(::GetSystemMenu(v_hwnd_console, FALSE), SC_CLOSE, MF_BYCOMMAND);
		::DrawMenuBar(v_hwnd_console);
		::SetWindowTextA(v_hwnd_console, "ydwe保存加速插件 2.2m");

		
		std::string text = R"(
<while>
用来加速ydwe保存地图的插件，对地形装饰物，触发编辑器极速优化
参与开发者 ：<yellow>w4454962</yellow>、 神话、 actboy168、月升朝霞、白喵、裂魂
感谢7佬的最初版本
排名不分先后，为魔兽地图社区的贡献表示感谢。
bug反馈：魔兽地图编辑器吧 -> @<yellow>w4454962</yellow> 加速器bug反馈群 -> <green>724829943</green>   lua技术交流群 -> <blue>1019770872</blue>。
						----2022/1/26 新年好哇
version 2.2m update:

<green>2.2m: 更精准的中文逆天类型错误提示</green>
<grey>
2.2l: 修复个别条件表达式报错的bug
2.2k: 修复读取物体数据japi的bug 加强清除计时器 触发器的警报检测，可以在删除ydtrigger.dll的情况下正常显示编译逆天UI。
2.2j: 修复设置逆天局部变量 里读局部变量ui不显示的问题
2.2i: 修复多开关闭地图提示保存,关闭编辑器提示保存会导致地图错乱的bug
2.2h: 修复实数变量变化事件缺少双引号的bug
修复了 迷雾、天气、光照、环境音效、金矿失效的问题。
加强了预处理器#include 支持中文路径。 
新增了MapHelper.json配置文件 如果有修改ydtrigger.dll的特殊动作可以在里面配置黑名单
重构了大部分代码， 源码更清晰，缩进跟函数名更精确的版本。

当前插件仍在测试中，推荐自己测试时使用新的保存模式提升速度，发布正式版时使用旧的保存模式保证稳定

如需关闭控制台，请在ydwe目录下的 bin\\EverConfig.cfg 中修改[ScriptCompiler]项下加入EnableYDTrigger = 1
EnableYDTrigger = -1 为默认开启控制台与对话框
EnableYDTrigger = 0 为使用原本的保存方式
EnableYDTrigger = 1 默认开启加速保存
</grey>
</while>
)";
	
		
		console_color_output(text);



		HICON hIcon = LoadIcon(g_hModule, MAKEINTRESOURCE(IDI_ICON2));
		if (hIcon) {

			SendMessage(v_hwnd_console, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		}

	}


}

int Helper::getConfig() const
{
	return GetPrivateProfileIntA("ScriptCompiler", "EnableYDTrigger", -1, m_configPath.string().c_str());
}






static BOOL __stdcall enumWindowProc(HWND hwnd, LPARAM lparam) {
	DWORD processId = 0;
	if (GetWindowThreadProcessId(hwnd, &processId) && processId == GetCurrentProcessId()) {
		auto list_ptr = (std::vector<HWND>*)lparam;
		list_ptr->push_back(hwnd);
	}
	return true;
}

void Helper::setMenuEnable(bool is_enable) {
	std::vector<HWND> list;

	EnumWindows(enumWindowProc, (LPARAM)&list);
	if (list.size() == 0)
		return;

	for (auto& hwnd : list) {
		HMENU menu = GetMenu(hwnd);
		
		if (menu == NULL) {
			continue;
		}
		int count = GetMenuItemCount(menu);
		if (count == -1) {
			continue;
		}

		char buffer[256] = { 0 };
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = MIIM_TYPE;
		info.dwTypeData = buffer;

		std::string name;
		for (INT i = 0; i < count; i++) {
			info.cch = _countof(buffer);	// GetMenuItemInfo 之后 cch 变化，必须重复对 cch 赋值，否则将导致 dwTypeData 被截断
			if (GetMenuItemInfo(menu, i, TRUE, &info) == 0)
				continue;
			name = info.dwTypeData;
			if (name.size() > 0) {
				info.fMask = MIIM_STATE;
				info.fState = is_enable ? MFS_ENABLED : MFS_GRAYED;
				SetMenuItemInfo(menu, i, true, &info); //修改菜单状态
			}
		}

		HWND toolbar = FindWindowExA(hwnd, 0, "ToolbarWindow32", 0);
		if (toolbar) {
			for (int i = 0; i < 100; i++) {
				::SendMessage(toolbar, TB_HIDEBUTTON, i, MAKELPARAM(is_enable? FALSE:TRUE, 0));
			}
		}
	}
}
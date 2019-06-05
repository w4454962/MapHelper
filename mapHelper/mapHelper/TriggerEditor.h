#pragma once

#include "stdafx.h"
#include "mapHelper.h"

struct TriggerType
{
	uint32_t flag;//0x0
	const char type[0x8c];//0x4
	const char value[0x110];//0x90 配置在TriggerData文件里的TriggerTypeDefaults的默认值
};//size 0x1a0

struct TriggerConfigData
{
	char unknow[0x1c];
	uint32_t type_count; //0x1c
	TriggerType* array;//0x20
};

struct Parameter
{
	enum Type {
		invalid = -1,
		preset,
		variable,
		function,
		string
	};
	uint32_t table; //0x0
	uint32_t unknow2; //0x4
	uint32_t type;		  //0x8
	char unknow3[0x40];//0xc
	const char value[0x12c]; //0x4c
	struct Action* funcParam;//0x178 非0时表示 此参数包含参数
	Parameter* arrayParam;//0x17c 非0时 表示该参数是数组变量 并且拥有子参数
};

struct Action
{
	enum Type {
		event,
		condition,
		action
	};

	struct VritualTable
	{
		uint32_t unknow1;
		uint32_t unknow2;
		uint32_t (__thiscall* getType)(void* pThis);
	};
	VritualTable* table; //0x0
	char unknow1[0x8];	 //0x4
	uint32_t child_count;	//0xc
	Action** child_actions;//0x10
	char unknow2[0xc];	 //0x14
	const char name[0x100]; //0x20
	uint32_t unknow3;//0x120;
	uint32_t unknow32;//0x124;
	uint32_t param_count; // 0x128
	Parameter** parameters;//0x12c
	char unknow4[0xC];//0x130
	uint32_t enable;//0x13c 
	char unknow5[0x14];//0x140
	uint32_t group_id;//0x154 当该条动作是子动作时为0 否则是-1
};

struct Trigger
{
	char unknow1[0xc];
	uint32_t line_count; //0xc
	Action** actions;	//0x10
	char unknow2[0x4];//0x14
	uint32_t is_comment; //0x18
	uint32_t unknow3; //0x1c
	uint32_t is_enable;  //0x20 
	uint32_t is_disable_init; //0x24
	uint32_t is_custom_srcipt;//0x28
	uint32_t is_initialize;//0x2c 应该默认都是1
	uint32_t unknow7;//0x30
	uint32_t custom_jass_size;//0x34
	const char* custom_jass_script;//0x38
	char unknow4[0x10]; //0x3c
	const char name[0x100];//0x4c
	uint32_t unknow5;//0x14c
	struct Categoriy* parent;//0x150 //该触发所在的文件夹
	const char text[0x1000];//0x154 触发文本注释 这里未知长度 随便填了个size

};

struct Categoriy
{
	uint32_t categoriy_id;
	const char categoriy_name[0x10C];
	uint32_t has_change; // 0x110
	uint32_t unknow2; // 0x114
	uint32_t is_comment; // 0x118
	char unknow[0x14];// 0x11c
	uint32_t trigger_count;//0x130 当前别类中的触发器数量
	Trigger** triggers;		//0x134
};

struct VariableData
{
	uint32_t unknow1;	//0x0 未知 总是1
	uint32_t is_array;	//0x4
	uint32_t array_size;//0x8
	uint32_t is_init;	//0xc
	const char type[0x1E];//0x10
	const char name[0x64];//0x2e
	const char value[0x12e];//0x92
};

struct Variable 
{
	char unknow1[0x8];		//0x0
	uint32_t globals_count;//0x8 包含gg_ 触发器 地形预设数据的全局变量
	VariableData* array; //0xc
};

struct TriggerData
{
	uint32_t unknow1;		//0x0
	uint32_t trigger_count; // 0x4	所有触发器数量
	char unknow2[0xC];		// 0x8
	uint32_t categoriy_count; //0x14 
	Categoriy** categories;	  //0x18
	uint32_t unknow3;		 //0x1c
	Variable* variables;    //0x20
	char unknow4[0x10]; // 0x24
	const char global_jass_comment[0x800];//0x34
	uint32_t unknow5; //0x834
	uint32_t globals_jass_size; //0x838
	const char* globals_jass_script;//0x83c
};


class TriggerEditor
{
public:
	TriggerEditor();
	~TriggerEditor();

	static TriggerEditor* getInstance();

	void loadTriggers(TriggerData* data);
	void loadTriggerConfig(TriggerConfigData* data);

	void saveTriggers(const char* path); //生成wtg
	void saveScriptTriggers(const char* path);//生成 wct
	void saveSctipt(const char* path); //生成j

	
private: 
	std::string convert_gui_to_jass(Trigger* trigger, std::vector<std::string>& initializtions);
	std::string resolve_parameter(Parameter* parameter, const std::string& trigger_name, std::string& pre_actions, const std::string& type, bool add_call = false) const;
	std::string testt(const std::string& trigger_name, const std::string& parent_name, Parameter** parameters, std::string& pre_actions, bool add_call) const;


	void writeCategoriy(BinaryWriter& writer);
	void writeVariable(BinaryWriter& writer);
	void writeTrigger(BinaryWriter& writer);
	void writeTrigger(BinaryWriter& writer,Trigger* trigger);
	void writeAction(BinaryWriter& writer, Action* action);
	void writeParameter(BinaryWriter& writer, Parameter* param);
protected:
	TriggerConfigData* m_configData;
	TriggerData* m_editorData;
	uint32_t m_version;
	bool is_ydwe;

	const std::string seperator = "//===========================================================================\n";

	//变量类型默认的值
	std::map<std::string, std::string> m_typeDefaultValues;
};
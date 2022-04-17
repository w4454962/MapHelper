#include "stdafx.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <regex>
#include "Nodes\Node.h"
#include <base\util\json.hpp>

//所有物品类型的jass变量名
std::string randomItemTypes[] = {
	"ITEM_TYPE_ANY",
	"ITEM_TYPE_PERMANENT",
	"ITEM_TYPE_CHARGED",
	"ITEM_TYPE_POWERUP",
	"ITEM_TYPE_ARTIFACT",
	"ITEM_TYPE_PURCHASABLE",
	"ITEM_TYPE_CAMPAIGN",
	"ITEM_TYPE_MISCELLANEOUS"
};

TriggerEditor::TriggerEditor()
	:m_editorData(nullptr),
	m_configData(nullptr),
	m_version(7)
{ 
	for (int i = 0; i < 200; i++)
	{
		for (int k = 0; k < i; k++)
		{
			spaces[i] += '\t';
		}
	}
	
}

TriggerEditor::~TriggerEditor()
= default;

//TriggerEditor* TriggerEditor::getInstance()
//{
//	static TriggerEditor instance; 
//	return &instance;
//}

void TriggerEditor::loadTriggers(TriggerData* data)
{
	m_editorData = data;

}

void TriggerEditor::loadTriggerConfig(TriggerConfigData* data)
{
	//if (m_configData) return;

	m_configData = data;
	//std::cout<<"cout读取配置文件"<<std::endl;

	for (size_t i = 0; i < data->type_count; i++)
	{
		TriggerType* type_data = &data->array[i];
		m_typesTable[type_data->type] = type_data;
	}


	using json::Json;
	fs::path path = g_module_path.parent_path() / "MapHelper.json";
	std::ifstream config(path, std::ios::binary);

	if (config.is_open()) {
		m_blacklist_map.clear();

		std::stringstream ss;

		ss << config.rdbuf();


		std::string text = ss.str();
		std::string error;

		config.close();

		Json json = Json::parse(text, error);

		auto items = json.object_items();

		if (!error.empty()) {
			print("json error : %s\n", error.c_str());
		}

		if (!items.empty()) {
			auto black = items["BlackList"].array_items();
			if (!black.empty()) {
				for (auto& name : black) {
					if (name.is_string()) {
						m_blacklist_map.emplace(name.string_value(), true);
					}
				}
			}
		}
	}
}

void TriggerEditor::saveTriggers(const char* path)
{
	if (!m_editorData)
	{
		//std::cout << "缺少触发器数据" << std::endl;
		return;
	}

	std::cout << "自定义保存wtg文件"<< std::endl;

	auto start = clock();


	BinaryWriter writer;


	writer.write_string("WTG!");

	m_version = 7;
	writer.write(m_version);
	
	writeCategoriy(writer);

	writeVariable(writer);

	writeTrigger(writer);

	std::ofstream out(std::string(path) + ".wtg", std::ios::binary);
	writer.finish(out);
	out.close();

	print("wtg 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}

void TriggerEditor::writeCategoriy(BinaryWriter& writer)
{
	auto categories = m_editorData->categories;

	const auto count = m_editorData->categoriy_count;

	writer.write(count);

	for (size_t i = 0; i < count; i++)
	{
		auto categoriy = m_editorData->categories[i];
		
		writer.write(categoriy->categoriy_id);

		auto categoriy_name = std::string(categoriy->categoriy_name);

		writer.write_c_string(categoriy_name);

		writer.write(categoriy->is_comment);

		categoriy->has_change = 0;

	}

}
void TriggerEditor::writeVariable(BinaryWriter& writer)
{
	auto variables = m_editorData->variables;


	uint32_t unknow = 2;
	//未知 总是写2
	writer.write(unknow);

	uint32_t variable_count = 0;

	for(size_t i = 0; i < variables->globals_count ; i++)
	{
		Variable* data = &variables->array[i];
		//名字非gg_开头的变量
		if (data && strncmp(data->name, "gg_", 3))
			variable_count++;
	}



	writer.write(variable_count);

	//将非gg_的变量数据写入
	for (size_t i = 0; i < variables->globals_count; i++)
	{
		auto data = &variables->array[i];
		if (data && strncmp(data->name, "gg_", 3))
		{


			writer.write_c_string(data->name);
			writer.write_c_string(data->type);

			uint32_t uknow2 = 1;//未知 总是写1
			writer.write(uknow2);

			writer.write(data->is_array);
			writer.write(data->array_size);
			writer.write(data->is_init);
			writer.write_c_string(data->value);
		}
	}
}

void TriggerEditor::writeTrigger(BinaryWriter& writer)
{
	const auto count = m_editorData->trigger_count;
	writer.write(count);

	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		auto categoriy = m_editorData->categories[i];
		auto trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			auto trigger = categoriy->triggers[n];
	
			writeTrigger(writer, trigger);
		}
	}

}

void TriggerEditor::writeTrigger(BinaryWriter& writer, Trigger* trigger)
{
	writer.write_c_string(trigger->name);
	writer.write_c_string(trigger->text);

	if (m_version > 5)
	{
		writer.write(trigger->is_comment);
	}

	writer.write(trigger->is_enable);

	writer.write(trigger->is_custom_srcipt);

	writer.write(trigger->is_disable_init);

	writer.write(trigger->is_initialize);

	uint32_t parent_id = -1;
	if (trigger->parent)
	{
		parent_id = trigger->parent->categoriy_id;
	}
	writer.write(parent_id);

	writer.write(trigger->line_count);

	for (size_t i = 0; i < trigger->line_count; i++)
	{
		auto action = trigger->actions[i];

		auto actionType = get_action_type(action);

		writer.write(actionType);

		writeAction(writer, action);
	}

}
void TriggerEditor::writeAction(BinaryWriter& writer, Action* action)
{

	writer.write_c_string(action->name);

	writer.write(action->enable);
	
	uint32_t count = action->param_count;

	//循环写参数
	for (size_t i = 0; i < count; i++)
	{
		auto param = action->parameters[i];
		writeParameter(writer, param);
	}

	const auto child_count = action->child_count;
	writer.write(child_count);
	//如果是 动作组 则循环将子动作写入
	for (size_t i = 0; i < child_count; i++)
	{
		const auto child = action->child_actions[i];

		const auto child_type = get_action_type(child);

		writer.write(child_type);

		writer.write(child->group_id);

		writeAction(writer, child);
		

	}
}

void TriggerEditor::writeParameter(BinaryWriter& writer, Parameter* param)
{
	const auto type = param->typeId;
	writer.write(type);

	writer.write_c_string(param->value);

	const uint32_t has_param = (type == Parameter::function && param->funcParam);

	writer.write(has_param);
	if (has_param)
	{
		Action* action = param->funcParam;

		const uint32_t actionType = get_action_type(action);

		writer.write(actionType);

		writeAction(writer, action);

	}

	const uint32_t is_array = param->arrayParam != nullptr;
	writer.write(is_array);

	//如果是数组 则写入数组中的参数
	if (is_array)
	{
		auto child = param->arrayParam;
		writeParameter(writer,child);
	}

}

TriggerEditor& get_trigger_editor()
{
	static TriggerEditor instance;
	return instance;
}


void TriggerEditor::saveScriptTriggers(const char* path)
{
	print("自定义保存wct文件\n");

	const auto start = clock();

	auto* data = m_editorData;

	BinaryWriter writer;

	const uint32_t version = 1;
	writer.write(version);

	writer.write_c_string(data->global_jass_comment);

	writer.write(data->globals_jass_size);

	const std::string_view jass(data->globals_jass_script, data->globals_jass_size);
	writer.write_string(jass);

	writer.write(data->trigger_count);

	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		const auto categoriy = m_editorData->categories[i];
		const auto trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			auto trigger = categoriy->triggers[n];
			size_t size = 0;
			if (trigger->is_custom_srcipt)
				size = trigger->custom_jass_size;
			writer.write(size);
			if (size > 0)
			{
				std::string script(trigger->custom_jass_script, size);
				writer.write_c_string(script);
			}
			
		}
	}

	std::ofstream out(std::string(path) + ".wct", std::ios::binary);
	writer.finish(out);
	out.close();

	print("wct 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}

/*
* 进制如果为 xx xx 00 51 这种形式为自定义组
* 第一个为数组索引 第二个数为 gg_rg_%03d 中的%03d 第三个应该只可能为 00 第四的默认为 0x51 ->81
* 传递 gg_rg_%03d[i]
* --------------------------------------------------------------------------
* 其他情况存在00 默认为空物品 
* 传递-1
* --------------------------------------------------------------------------
* 剩余特有id为
* 物品
* YYI/ -- YYI0 ---------- YYI8
* YiI/ -- YiI0 ---------- YiI8
* ----------------------------
* YoI/ -- YoI0 ---------- YoI8 
* 从上到下为 任何 永久 可充 力量提升 人造 可购买 战役 混杂 即 Y - o  -> 89 105 106 ---- 111
* 从做到右为物品等级 即 -1 - 8  -> 47 - 56
* 传递 ChooseRandomItemEx(ITEM_TYPE, num:[-1,8])
* --------------------------------------------------------------------------
* 单位
* YYU/ -- YYU0 ---------- YYUD
* 从左到右为单位等级 即 -1 20 -> 47 - 68
* 传递 ChooseRandomCreep(num:[-1,20])
* --------------------------------------------------------------------------
* 剩余为正常id
* 传递 'name'
* 
* 记录一下容易忘记的
* Y 为 89
* I 为 73
* U 为 85
*/
std::string TriggerEditor::WriteRandomDisItem(const char *id) {
	int num[4];
	char buffer[0x400];
	std::string name = std::string(id, id + 0x4);
	for (int i = 0; i < 4; i++) {
		num[i] = (int)id[i];
	}
	// 如果是随机单位
	if (std::string(id, id + 0x3) == "YYU") {
		sprintf(buffer, "ChooseRandomCreep(%d)", num[3] - 48);
		return std::string(buffer);
	}//如果是随机物品
	else if (id[0] == 'Y' && id[2] == 'I') {
		int random_item = (num[1] == 89) ? 0 : (num[1] - 104);
		sprintf(buffer, "ChooseRandomItemEx(%s, %d)", randomItemTypes[random_item].c_str(), num[3] - 48);
		return std::string(buffer);
	}//如果是随机组
	else if (num[2] == 0 && id[3] == 'Q') {
		sprintf(buffer, "gg_rg_%03d[%d]", num[1], num[0]);
		return std::string(buffer);
	}
	else if (num[1] && num[2] && num[3] && num[0]) {
		return "'" + name + "'";
	}

	return "-1";
}

void TriggerEditor::saveSctipt(const char* path)
{

	print("自定义保存jass文件\n");

	auto start = clock();

	auto data = m_editorData;


	BinaryWriter writer,writer2;

	auto& worldEditor = get_world_editor();
	auto worldData = worldEditor.getEditorData();

	char buffer[0x400];
	variableTable.clear();

	writer.write_string(seperator);
	writer.write_string("//*\n");
	writer.write_string("//*  Global variables\n");
	writer.write_string("//*\n");
	writer.write_string(seperator);

	
	writer.write_string("#define USE_BJ_ANTI_LEAK\n");
	writer.write_string("#include <YDTrigger/Import.h>\n");
	writer.write_string("#include <YDTrigger/YDTrigger.h>\n");
	writer.write_string("#include <YDTrigger/ImportSaveLoadSystem.h>\n");
	writer.write_string("#include <YDTrigger/Hash.h>\n");

	
	writer.write_string("globals\n");

	for (size_t i = 0; i < data->variables->globals_count; i++)
	{
		Variable* var = &data->variables->array[i];
		auto name{ std::string(var->name) };
		auto type{ std::string(var->type) };
		auto base{ std::string(getBaseType(type)) };

		variableTable[name] = var;

		if (var->is_array)
		{
			writer.write_string("\t" + base + " array udg_" + name + "\n");
		}
		else
		{
			//非gg_开头的自定义变量
			if (strncmp(var->name, "gg_", 3))
				name = "udg_" + name;
			std::string value;
			if (base == "integer" || base == "real")
				value = "0";
			else if (base == "boolean")
				value = "false";
			else
				value = "null";
			if (base == "string")
				writer.write_string("\t" + base + " " + name + "\n");
			else
				writer.write_string("\t" + base + " " + name + " = " + value + "\n");
		}
	}

	//申明随机组的全局变量
	if (worldData->random_group_count > 0)
	{

		for (size_t i = 0; i < worldData->random_group_count; i++)
		{
			sprintf(buffer, "%03d", i);
			writer.write_string("\tinteger array gg_rg_" + std::string(buffer) + "\n");
		}
	}

	writer.write_string("#include <YDTrigger/Globals.h>\n");
	writer.write_string("endglobals\n");
	writer.write_string("#include <YDTrigger/Function.h>\n");

	//开始初始化全局变量
	writer.write_string("function InitGlobals takes nothing returns nothing\n");
	writer.write_string("\tlocal integer i = 0\n");

	for (size_t i = 0; i < data->variables->globals_count; i++)
	{
		Variable* var = &data->variables->array[i];
		std::string name = var->name;
		std::string type = var->type;
		std::string base = getBaseType(type);
		std::string value = var->value;
		auto& world = get_world_editor();
		//获取默认值
		std::string defaultValue;
		auto it = m_typesTable.find(type);
		if (it != m_typesTable.end())
			defaultValue = it->second->value;


		//跳过默认初始值为空的变量
		if (!var->is_init && base != "string" && defaultValue.empty())
			continue;
		//跳过gg_开头的变量，会在其他地方初始化
		if (strncmp(name.c_str(), "gg_", 3) == 0)
			continue;
		//将变量的值转译一遍
		if (!world.getConfigData("TriggerParams", value, 1).empty())
			value = world.getConfigData("TriggerParams", value, 2);
		if (base == "integer" && !value.empty() && std::regex_match(type, std::regex("^\\w+code$")))
			value = "'" + value + "'";
		if (var->is_array )
		{
			writer.write_string("\tset i = 0\n");
			writer.write_string("\tloop\n");
			writer.write_string("\t\texitwhen(i > " + std::to_string(var->array_size) + ")\n");
			
			if (var->is_init)
				if (base == "string")
					if (value.empty())
						writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
					else
						writer.write_string("\t\tset udg_" + name + "[i] = \"" + value + "\"\n");
				else 
					writer.write_string("\t\tset udg_" + name + "[i] = " + value + "\n");
			else
				if (base == "string")
					writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
				else 
					writer.write_string("\t\tset udg_" + name + "[i] = " + defaultValue + "\n");
			writer.write_string("\t\tset i = i + 1\n");
			writer.write_string("\tendloop\n");
		}
		else
		{
			if (var->is_init)
				if (base == "string")
					if (value.empty())
						writer.write_string("\tset udg_" + name + " = \"\"\n");
					else
						writer.write_string("\tset udg_" + name + " = \"" + value + "\"\n");
				else
					writer.write_string("\tset udg_" + name + " = " + value + "\n");
			else
				if (base == "string")
					writer.write_string("\tset udg_" + name + " = \"\"\n");
				else
					writer.write_string("\tset udg_" + name + " = " + defaultValue + "\n");
		}
	}
	writer.write_string("endfunction\n\n");


	writer.write_string("function InitRandomGroups takes nothing returns nothing\n");
	writer.write_string("\tlocal integer curset\n");

	uint32_t count = worldData->random_group_count;

	for (size_t i = 0; i < count; i++)
	{
		RandomGroupData* groupData = &worldData->random_groups[i];
		if (groupData->group_count == 0)
			continue;
		writer.write_string("\t// Group " + std::to_string(i) + " - " + groupData->name + "\n");
		writer.write_string("\tcall RandomDistReset()\n");
		for (size_t a = 0; a < groupData->group_count; a++)
		{
			RandomGroup* group = &groupData->groups[a];
			writer.write_string("\tcall RandomDistAddItem(" + std::to_string(a) + "," + std::to_string(group->rate) + ")\n");
		}
		writer.write_string("\tset curset=RandomDistChoose()\n");
		
		for (size_t a = 0; a < groupData->group_count; a++)
		{
			RandomGroup* group = &groupData->groups[a];
			std::string head = "\tif";
			if (a > 0) head = "\telseif";
			writer.write_string( head + " ( curset == " + std::to_string(a) + " ) then\n");

			for (uint32_t b = 0; b < groupData->param_count; b++)
			{
				const char* ptr = group->names[b];
				
				sprintf(buffer, "\t\tset gg_rg_%03d[%i] = %s\n", i, b, WriteRandomDisItem(ptr).c_str());
				writer.write_string(std::string(buffer));
			}
		}
		writer.write_string("\telse\n");
		for (uint32_t b = 0; b < groupData->param_count; b++)
		{
			sprintf(buffer, "\t\tset gg_rg_%03d[%i] = -1\n", i, b);
			writer.write_string(std::string(buffer));
		}
		writer.write_string("\tendif\n");
	}
	writer.write_string("endfunction\n\n");

	

	
	for (size_t i = 0; i < worldData->item_table_count; i++)
	{
		sprintf(buffer, "%06d",i);

		writer.write_string("function ItemTable_" + std::string(buffer) + "_DropItems takes nothing returns nothing\n");
		writer.write_string(R"(
	local widget trigWidget= null
	local unit trigUnit= null
	local integer itemID= 0
	local boolean canDrop= true

	set trigWidget=bj_lastDyingWidget
	if ( trigWidget == null ) then
		set trigUnit=GetTriggerUnit()
	endif

	if ( trigUnit != null ) then
		set canDrop=not IsUnitHidden(trigUnit)
		if ( canDrop and GetChangingUnit() != null ) then
			set canDrop=( GetChangingUnitPrevOwner() == Player(PLAYER_NEUTRAL_AGGRESSIVE) )
		endif
	endif

	if ( canDrop ) then
		)");

		writer.write_string("\n");
		ItemTable* itemTable = &worldData->item_table[i];

		for (size_t a = 0; a < itemTable->setting_count; a++)
		{
			ItemTableSetting* itemSetting = &itemTable->item_setting[a];

			writer.write_string("\t\tcall RandomDistReset()\n");
			for (uint32_t b = 0; b < itemSetting->info_count; b++)
			{
				ItemTableInfo* info = &itemSetting->item_infos[b];
				writer.write_string("\t\tcall RandomDistAddItem(" + WriteRandomDisItem(info->name) + ", " + std::to_string(info->rate) + ")\n");
			}
			writer.write_string(R"(
		set itemID=RandomDistChoose()
		if ( trigUnit != null ) then
			call UnitDropItem(trigUnit, itemID)
		else
			call WidgetDropItem(trigWidget, itemID)
		endif
)");

		}

		writer.write_string(R"(
	endif

	set bj_lastDyingWidget=null
	call DestroyTrigger(GetTriggeringTrigger())
endfunction
		)");

		writer.write_string("\n");
	}

	for (size_t i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];
		uint32_t size = 0;
		for (size_t a = 0; a < unit->item_setting_count; a++)
		{
			ItemTableSetting* itemSetting = &unit->item_setting[a];
			size += itemSetting->info_count;
		}

		if (size > 0) 
		{
			sprintf(buffer, "%06d", unit->index);

			writer.write_string("function Unit" + std::string(buffer) + "_DropItems takes nothing returns nothing\n");

			writer.write_string(R"(
	local widget trigWidget= null
	local unit trigUnit= null
	local integer itemID= 0
	local boolean canDrop= true

	set trigWidget=bj_lastDyingWidget
	if ( trigWidget == null ) then
		set trigUnit=GetTriggerUnit()
	endif

	if ( trigUnit != null ) then
		set canDrop=not IsUnitHidden(trigUnit)
		if ( canDrop and GetChangingUnit() != null ) then
			set canDrop=( GetChangingUnitPrevOwner() == Player(PLAYER_NEUTRAL_AGGRESSIVE) )
		endif
	endif

	if ( canDrop ) then
		)");

			writer.write_string("\n");

			

			for (size_t a = 0; a < unit->item_setting_count; a++)
			{
				ItemTableSetting* itemSetting = &unit->item_setting[a];

				writer.write_string("\t\tcall RandomDistReset()\n");
				
				for (uint32_t b = 0; b < itemSetting->info_count; b++)
				{
					ItemTableInfo* info = &itemSetting->item_infos[b];
					writer.write_string("\t\tcall RandomDistAddItem(" + WriteRandomDisItem(info->name) + ", " + std::to_string(info->rate) + ")\n");
				}

				writer.write_string(R"(
		set itemID=RandomDistChoose()
		if ( trigUnit != null ) then
			call UnitDropItem(trigUnit, itemID)
		else
			call WidgetDropItem(trigWidget, itemID)
		endif
)");

			}

			writer.write_string(R"(
	endif

	set bj_lastDyingWidget=null
	call DestroyTrigger(GetTriggeringTrigger())
endfunction
		)");

			writer.write_string("\n");
		}
	}

	for (size_t i = 0; i < worldData->doodas->unit_count; i++)
	{
		Unit* unit = &worldData->doodas->array[i];
		if (unit->item_setting_count > 0) 
		{
			sprintf(buffer, "%06d", unit->index);

			writer.write_string("function Doodad" + std::string(buffer) + "_DropItems takes nothing returns nothing\n");

			writer.write_string(R"(
	local widget trigWidget= null
	local unit trigUnit= null
	local integer itemID= 0
	local boolean canDrop= true

	set trigWidget=bj_lastDyingWidget
	if ( trigWidget == null ) then
		set trigUnit=GetTriggerUnit()
	endif

	if ( trigUnit != null ) then
		set canDrop=not IsUnitHidden(trigUnit)
		if ( canDrop and GetChangingUnit() != null ) then
			set canDrop=( GetChangingUnitPrevOwner() == Player(PLAYER_NEUTRAL_AGGRESSIVE) )
		endif
	endif

	if ( canDrop ) then
		)");

			writer.write_string("\n");

			

			for (size_t a = 0; a < unit->item_setting_count; a++)
			{
				ItemTableSetting* itemSetting = &unit->item_setting[a];

				writer.write_string("\t\tcall RandomDistReset()\n");
				
				for (uint32_t b = 0; b < itemSetting->info_count; b++)
				{
					ItemTableInfo* info = &itemSetting->item_infos[b];
					writer.write_string("\t\tcall RandomDistAddItem(" + WriteRandomDisItem(info->name) + ", " + std::to_string(info->rate) + ")\n");
				}

				writer.write_string(R"(
		set itemID=RandomDistChoose()
		if ( trigUnit != null ) then
			call UnitDropItem(trigUnit, itemID)
		else
			call WidgetDropItem(trigWidget, itemID)
		endif
)");

			}

			writer.write_string(R"(
	endif

	set bj_lastDyingWidget=null
	call DestroyTrigger(GetTriggeringTrigger())
endfunction
		)");

			writer.write_string("\n");
		}
	}
	

	writer.write_string("function InitSounds takes nothing returns nothing\n");

	for (size_t i = 0; i < worldData->sounds->sound_count; i++)
	{
		Sound* sound = &worldData->sounds->array[i];

		std::string sound_name = sound->name;

		// Replace spaces by underscores
		std::replace(sound_name.begin(), sound_name.end(), ' ', '_');

		auto it = variableTable.find(sound_name);
		if (it != variableTable.end() && getBaseType(it->second->type) == "string")
		{
			writer.write_string("\tset " + sound_name + " = \"" + string_replaced(sound->file, "\\", "\\\\") + "\"\n");
			continue;
		}
	
		writer.write_string("\tset " + sound_name + " = CreateSound(\"" +
			string_replaced(sound->file, "\\", "\\\\") + "\", " +
			(sound->flag & 1 ? "true" : "false") + ", " +
			(sound->flag & 2 ? "true" : "false") + ", " +
			(sound->flag & 4 ? "true" : "false") + ", " +
			std::to_string(sound->fade_in_rate) + ", " +
			std::to_string(sound->fade_out_rate) + ", " +
			"\"" + sound->effect + "\"" +
			")\n");
		auto& v_we = get_world_editor();
		int duration = v_we.getSoundDuration(sound->file);
		if (duration > 0)
		{
			
			writer.write_string("\tcall SetSoundDuration(" + sound_name + ", " + std::to_string(duration) + ")\n"); // Sound duration
			writer.write_string("\tcall SetSoundChannel(" + sound_name + ", " + std::to_string(sound->channel) + ")\n");
			writer.write_string("\tcall SetSoundVolume(" + sound_name + ", " + std::to_string(sound->volume) + ")\n");

			sprintf(buffer, "%.1f", sound->pitch);
			writer.write_string("\tcall SetSoundPitch(" + sound_name + ", " + std::string(buffer) + ")\n");

			//is 3d 
			if (sound->flag & 2)
			{
				sprintf(buffer,"%.1f,%.1f", sound->min_range, sound->max_range);
				writer.write_string("\tcall SetSoundDistances(" + sound_name + ", " + std::string(buffer) + ")\n");

				sprintf(buffer, "%.1f", sound->distance_cutoff);
				writer.write_string("\tcall SetSoundDistanceCutoff(" + sound_name + ", " + std::string(buffer) + ")\n");
				
				sprintf(buffer, "%.1f,%.1f,%d", sound->inside, sound->outside, sound->outsideVolume);
				writer.write_string("\tcall SetSoundConeAngles(" + sound_name + ", " + std::string(buffer) + ")\n");

				sprintf(buffer, "%.1f,%.1f,%.1f", sound->x,sound->y,sound->z);
				writer.write_string("\tcall SetSoundConeOrientation(" + sound_name + ", " + std::string(buffer) + ")\n");
			}

		}
	}

	writer.write_string("endfunction\n");


	writer.write_string("function CreateDestructables takes nothing returns nothing\n");
	writer.write_string("\tlocal destructable d\n");
	writer.write_string("\tlocal trigger t\n");
	writer.write_string("\tlocal real life\n");

	writer2.clear();

	for (size_t i = 0; i < worldData->doodas->unit_count; i++)
	{
		Unit* unit = &worldData->doodas->array[i];
		sprintf(buffer, "gg_dest_%.04s_%04d", unit->name, unit->index);

		std::string id = buffer;

		//可能也许大概貌似这样就没事了
		if (variableTable.find(id) == variableTable.end()) 
			id = "d";

		if (id == "d" && unit->item_setting_count <= 0 && unit->item_table_index == -1)
			continue;
		
		sprintf(buffer, "'%.4s',%.1f,%.1f,%.1f,%.1f,%d", unit->name, unit->x, unit->y, unit->angle * 180.f / 3.1415926f, unit->scale_x, unit->variation);
		writer.write_string("\tset " + id + " = CreateDestructable(" + std::string(buffer) + ")\n");

		if (unit->doodas_life != 100) {
			sprintf(buffer, "%.2f", unit->doodas_life / 100.f);
			writer.write_string("\tset life = GetDestructableLife(" + id + ")\n");
			writer.write_string("\tcall SetDestructableLife(" + id + ", " + std::string(buffer) + " * life)\n");
		}

		if (unit->item_setting_count > 0) {
			sprintf(buffer, "%06d", unit->index);

			writer.write_string("\tset t = CreateTrigger()\n");
			writer.write_string("\tcall TriggerRegisterDeathEvent(t, " + id + ")\n");
			writer.write_string("\tcall TriggerAddAction(t, function SaveDyingWidget)\n");
			writer.write_string("\tcall TriggerAddAction(t, function Doodad" + std::string(buffer) + "_DropItems)\n");
		}
		else if (unit->item_table_index != -1) {
			sprintf(buffer, "%06d", unit->item_table_index);

			writer.write_string("\tset t = CreateTrigger()\n");
			writer.write_string("\tcall TriggerRegisterDeathEvent(t, " + id + ")\n");
			writer.write_string("\tcall TriggerAddAction(t, function SaveDyingWidget)\n");
			writer.write_string("\tcall TriggerAddAction(t, function ItemTable_" + std::string(buffer) + "_DropItems)\n");
		}

	}
	writer.write_bw(writer2);


	writer.write_string("endfunction\n");


	writer.write_string("function CreateItems takes nothing returns nothing\n");

	writer.write_string("\tlocal integer itemID\n");

	for (size_t i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];
		if (unit->type == 1) //遍历地形上的单位 判断这个单位是物品
		{
			bool b = strncmp(unit->name, "iDNR", 0x4) == 0;
			if (b)//判断是否是随机物品
			{
				if (unit->random_item_mode == 0)//从所有物品里随机
				{
					writer.write_string("\tset itemID=ChooseRandomItemEx(" + randomItemTypes[unit->random_item_type % 8] + ", " + std::to_string(unit->random_item_level) + ")\n");
				}
				else if (unit->random_item_mode == 1)
				{
					//随机组里面获取
					sprintf(buffer, "\t set itemID=gg_rg_%03d[%i]\n", unit->random_group_index,unit->random_group_child_index);

					writer.write_string(std::string(buffer, strlen(buffer)));
				}
				else if (unit->random_item_mode == 2)
				{
					//从自定义列表中获取
					writer.write_string("\tcall RandomDistReset()\n");
					for (size_t a = 0; a < unit->random_item_count; a++)
					{
						ItemTableInfo* info = &unit->random_items[a];
						std::string id = std::string(info->name, info->name + 0x4);
						writer.write_string("\tcall RandomDistAddItem('" + id + "', " + std::to_string(info->rate) + ")\n");
					}
					writer.write_string("\tset itemID = RandomDistChoose()\n");
				}
				else
				{
					writer.write_string("\tset itemID = -1\n");
				}

				writer.write_string("\tif ( itemID != -1 ) then\n\t");
		
			}
			
			//否则是一般物品 
			sprintf(buffer, "gg_item_%04s_%04d", unit->name, unit->index);
			std::string var_name = buffer;
			sprintf(buffer, "'%04s',%.1f,%.1f", unit->name, unit->x, unit->y);

			auto it = variableTable.find(var_name);
			if (it != variableTable.end())//判断是否有变量引用
			{
				writer.write_string("\tset " + var_name + " = CreateItem(" + std::string(buffer) + ")\n");
			}
			else
			{
				writer.write_string("\tcall CreateItem(" + std::string(buffer) + ")\n");
			}
			if (b)
			{
				writer.write_string("\tendif\n");
			}
		}
	}
	writer.write_string("endfunction\n");


	writer.write_string("function CreateUnits takes nothing returns nothing\n");

	writer.write_string("\tlocal unit u\n");
	writer.write_string("\tlocal integer unitID\n");
	writer.write_string("\tlocal trigger t\n");
	writer.write_string("\tlocal real life\n");


	for (size_t i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];

		//类型 不是单位 或者 是玩家开始点 则跳过
		if (unit->type != 0 || strncmp(unit->name,"sloc",4) == 0)
			continue;

		sprintf(buffer, "Player(%i),'%.4s',%.1f,%.1f,%.1f", unit->player_id, unit->name, unit->x, unit->y, unit->angle * 180.f / 3.1415926f);
		writer.write_string("\tset u = CreateUnit(" + std::string(buffer) + ")\n");

		sprintf(buffer, "gg_unit_%.04s_%04d", unit->name, unit->index);
		std::string name = buffer;
		if (variableTable.find(name) != variableTable.end())
		{
			writer.write_string("\tset " + name + " = u\n");
		}

		if (unit->health != -1) 
		{
			writer.write_string("\tset life = GetUnitState(u, UNIT_STATE_LIFE)\n");
			writer.write_string("\tcall SetUnitState(u, UNIT_STATE_LIFE, " + std::to_string(unit->health / 100.f) + "* life)\n");
		}

		if (unit->mana != -1) 
			writer.write_string("\tcall SetUnitState(u, UNIT_STATE_MANA, " + std::to_string(unit->mana) + ")\n");
		
		if (unit->level != 1) 
			writer.write_string("\tcall SetHeroLevel(u, " + std::to_string(unit->level) + ", false)\n");
		

		if (unit->state_str != 0) 
			writer.write_string("\tcall SetHeroStr(u, " + std::to_string(unit->state_str) + ", true)\n");
		

		if (unit->state_agi != 0) 
			writer.write_string("\tcall SetHeroAgi(u, " + std::to_string(unit->state_agi) + ", true)\n");
		

		if (unit->state_int != 0) 
			writer.write_string("\tcall SetHeroInt(u, " + std::to_string(unit->state_int) + ", true)\n");
		
		
		if (unit->passive_color_index != -1 && unit->passive_color_index < 16) 
			writer.write_string("\tcall SetUnitColor(u, ConvertPlayerColor(" + std::to_string(unit->passive_color_index) + "))\n");

		if (unit->pass_door_rect_index != -1 && unit->pass_door_rect_index < worldData->regions->region_count) {
			auto region = worldData->regions->array[unit->pass_door_rect_index];

			std::string region_name = std::string("gg_rct_") + region->name;
			convert_name(region_name);
			writer.write_string("\tcall WaygateSetDestination(u, GetRectCenterX(" + region_name + "), GetRectCenterY(" + region_name + "))\n");
			writer.write_string("\tcall WaygateActivate(u, true)\n");
		}


		//如果有金矿技能 
		uint32_t id = *(uint32_t*)unit->name;
		if (worldEditor.hasSkillByUnit(id, 'Agld') || worldEditor.hasSkillByUnit(id, 'Abgm') || worldEditor.hasSkillByUnit(id, 'Aegm')) {
			writer.write_string("\tcall SetResourceAmount(u, " + std::to_string(unit->gold_count) + ")\n");
		}

		float range;
		if (unit->warning_range != -1.f) 
		{
			if (unit->warning_range == -2.f) 
			{
				range = 200.f;
			}
			 else 
			 {
				range = unit->warning_range;
			 }
			writer.write_string("\tcall SetUnitAcquireRange(u, " + std::to_string(range) + ")\n");
		}

		for (size_t a = 0; a < unit->skill_count; a++)
		{
			UnitSkill* skill = &unit->skills[a];
			std::string skillId = std::string(skill->name, skill->name + 0x4);
			for (uint32_t b = 0; b < skill->level; b++) 
			{
				writer.write_string("\tcall SelectHeroSkill(u, \'" + std::string(skillId) + "\')\n");
			}
		
			std::string objectValue;
		
			
			if (skill->is_enable)
			{
				worldEditor.getSkillObjectData(*(uint32_t*)(skill->name), 0, "Orderon", objectValue);
				if (objectValue.empty()) 
					worldEditor.getSkillObjectData(*(uint32_t*)(skill->name), 0, "Order", objectValue);
				writer.write_string("\tcall IssueImmediateOrder(u, \"" + objectValue + "\")\n");
			} 
			else 
			{
				worldEditor.getSkillObjectData(*(uint32_t*)(skill->name), 0, "Orderoff", objectValue);
				if (!objectValue.empty()) 
					writer.write_string("\tcall IssueImmediateOrder(u, \"" + objectValue + "\")\n");
			}
		}
		
		for (size_t a = 0; a < unit->item_count; a++)
		{
			UnitItem* item = &unit->items[a];
			writer.write_string("\tcall UnitAddItemToSlotById(u, '" + std::string(item->name,item->name + 0x4) + "', " + std::to_string(item->slot_id) + ")\n");
		}

		std::string dropName;

		uint32_t size = 0;
		for (size_t a = 0; a < unit->item_setting_count; a++)
		{
			ItemTableSetting* itemSetting = &unit->item_setting[a];
			size += itemSetting->info_count;
		}

		if (size > 0)
		{
			sprintf(buffer, "Unit%06d_DropItems", unit->index);
			dropName = buffer;
		}
		else if(unit->item_table_index != -1)
		{
			sprintf(buffer, "ItemTable_%06d_DropItems", unit->item_table_index);
			dropName = buffer;
		}
		if (!dropName.empty())
		{
			
			writer.write_string("\tset t = CreateTrigger()\n");
			writer.write_string("\tcall TriggerRegisterUnitEvent(t, u, EVENT_UNIT_DEATH)\n");
			writer.write_string("\tcall TriggerRegisterUnitEvent(t, u, EVENT_UNIT_CHANGE_OWNER)\n");
			writer.write_string("\tcall TriggerAddAction(t, function " + dropName + ")\n");
		}
	}

	writer.write_string("endfunction\n");




	writer.write_string("function CreateRegions takes nothing returns nothing\n");
	writer.write_string("\tlocal weathereffect we\n\n");

	for (size_t i = 0; i < worldData->regions->region_count; i++)
	{
		Region* region = worldData->regions->array[i];
		std::string region_name = std::string("gg_rct_") + region->name;

		convert_name(region_name);


		int left = region->left * 32 + (int)region->info->minX;
		int right = region->right * 32 + (int)region->info->minX;
		int top = region->top * 32 + (int)region->info->minY;
		int bottom = region->bottom * 32 + (int)region->info->minY;

		int minX = min(left, right);
		int minY = min(bottom, top);
		int maxX = max(left, right);
		int maxY = max(bottom, top);

		writer.write_string("\tset " + region_name + "= Rect(" +
			std::to_string(minX) + "," +
			std::to_string(minY) + "," +
			std::to_string(maxX) + "," +
			std::to_string(maxY) + ")\n");

		if (*region->weather_id) 
		{
			std::string id = std::string(region->weather_id, region->weather_id + 0x4);
			writer.write_string("\tset we = AddWeatherEffect(" + region_name + ", '" + id + "')\n");
			writer.write_string("\tcall EnableWeatherEffect(we, true)\n");
		}

		if (strlen(region->sound_name) > 0)
		{
			int width = maxX - minX;
			int height = maxY - minY;
			int centerX = minX + width / 2;
			int centerY = minY + height / 2;
			sprintf(buffer, "%s,%i,%i,0.0", region->sound_name, centerX, centerY);
			writer.write_string("\tcall SetSoundPosition("  + std::string(buffer) + ")\n");

			sprintf(buffer, "%s,true,%i,%i", region->sound_name, width, height);
			writer.write_string("\tcall RegisterStackedSound(" + std::string(buffer) + ")\n");
		}

	}
	
	writer.write_string("endfunction\n");



	writer.write_string("function CreateCameras takes nothing returns nothing\n");

	for (size_t i = 0; i < worldData->cameras->camera_count; i++)
	{
		Camera* camera = &worldData->cameras->array[i];
		std::string camera_name = "gg_cam_" + std::string(camera->name);
		
		convert_name(camera_name);

		writer.write_string("\tset " + camera_name + " = CreateCameraSetup()\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_ZOFFSET, " + std::to_string(camera->z_offset)  + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_ROTATION, " + std::to_string(camera->rotation) + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_ANGLE_OF_ATTACK, " + std::to_string(camera->angle_of_attack) + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_TARGET_DISTANCE, " + std::to_string(camera->target_distance) + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_ROLL, " + std::to_string(camera->roll) + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_FIELD_OF_VIEW, " + std::to_string(camera->of_view) + ", 0.0)\n");
		writer.write_string("\tcall CameraSetupSetField(" + camera_name + ", CAMERA_FIELD_FARZ, " + std::to_string(camera->farz) + ", 0.0)\n");
		
		writer.write_string("\tcall CameraSetupSetDestPosition(" + camera_name + ", " + std::to_string(camera->x) + ", " + std::to_string(camera->y) + ", 0.0)\n");

	}
	
	writer.write_string("endfunction\n");




	TriggerData* trigger_data = worldData->triggers;


	std::regex reg("function\\s+(InitTrig_\\w+)\\s+takes");
	auto words_end = std::sregex_iterator();

	//写入全局jass
	if (trigger_data->globals_jass_size > 0)
	{
		std::string globals_jass = std::string(trigger_data->globals_jass_script, trigger_data->globals_jass_size - 1 );

		writer.write_string(globals_jass);

		auto words_begin = std::sregex_iterator(globals_jass.begin(), globals_jass.end(), reg);
	
		//正则表达式匹配 全局jass中 符合初始化触发器名字的函数
		for (; words_begin != words_end; ++words_begin)
		{
			m_initFuncTable[words_begin->str(1)] = true;
		}
	}



	

	writer.write_string("\n");


	std::string inits;


	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			Trigger* trigger = categoriy->triggers[n];

			if (trigger->is_comment || !trigger->is_enable) 
			{
				continue;
			}

			if (trigger->is_custom_srcipt)
			{
				std::string_view view = std::string_view(trigger->custom_jass_script);
				const std::string& script = std::string(view.begin(), view.end());
				writer.write_string(script);
				writer.write_string("\n");
				auto begin = std::sregex_iterator(script.begin(), script.end(), reg);
				for (; begin != words_end; ++begin)
				{
					m_initFuncTable[begin->str(1)] = true;
				}
			}
			else 
			{
				writer.write_string(convertTrigger(trigger));
			}

		}
	}


	writer.write_string(seperator);

	writer.write_string("function InitCustomTriggers takes nothing returns nothing\n");


	std::string initions;

	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			Trigger* trigger = categoriy->triggers[n];
			if (trigger->is_comment || !trigger->is_enable){
				continue;
			}
			std::string trigger_name = std::string(trigger->name);
			convert_name(trigger_name);

			bool has_init_event = mh::g_initTriggerMap.find(trigger) != mh::g_initTriggerMap.end();

			if (has_init_event && trigger->is_disable_init != 1) {
				initions += "\tcall ConditionalTriggerExecute(gg_trg_" + trigger_name + ")\n";
			}

			if (mh::g_disableTriggerMap.find(trigger) != mh::g_disableTriggerMap.end()) {
				continue;
			}

			std::string name = "InitTrig_" + trigger_name;
		
			bool has_init_func = m_initFuncTable.find(name) != m_initFuncTable.end();

			if (has_init_func) {
				writer.write_string("\tcall " + name + "()\n");

				if (trigger->is_custom_srcipt && trigger->is_initialize) {
					initions += "\tcall ConditionalTriggerExecute(gg_trg_" + trigger_name + ")\n";
				}
			}
			
		}
	}
	writer.write_string("endfunction\n");

	writer.write_string(seperator);
	writer.write_string("function RunInitializationTriggers takes nothing returns nothing\n");
	writer.write_string(initions);
	writer.write_string("endfunction\n");


	writer.write_string("function InitCustomPlayerSlots takes nothing returns nothing\n");

	const std::vector<std::string> players = { "MAP_CONTROL_USER", "MAP_CONTROL_COMPUTER", "MAP_CONTROL_NEUTRAL", "MAP_CONTROL_RESCUABLE" };
	const std::vector<std::string> races = { "RACE_PREF_RANDOM", "RACE_PREF_HUMAN", "RACE_PREF_ORC", "RACE_PREF_UNDEAD", "RACE_PREF_NIGHTELF" };


	std::vector<uint32_t> player_to_startloc(16,16);


	for (size_t i = 0; i < worldData->player_count; i++)
	{
		
		PlayerData* player_data = &worldData->players[i];

		std::string id = std::to_string(player_data->id);
		std::string player = "Player(" + id + "), ";
		writer.write_string("\tcall SetPlayerStartLocation(" + player + std::to_string(i) + ")\n");

		player_to_startloc[player_data->id % 16] = i;

		if (player_data->is_lock || player_data->race == 0) 
		{
			writer.write_string("\tcall ForcePlayerStartLocation(" + player + std::to_string(i) + ")\n");
		}

		writer.write_string("\tcall SetPlayerColor(" + player + "ConvertPlayerColor(" + id + "))\n");
		writer.write_string("\tcall SetPlayerRacePreference(" + player + races[static_cast<int>(player_data->race)] + ")\n");

		std::string selectable = "false";
		if (player_data->race == 0)
		{
			selectable = "true";
		}
		writer.write_string("\tcall SetPlayerRaceSelectable(" + player + selectable + ")\n");
		if (player_data->controller_id > 0)
		{
			writer.write_string("\tcall SetPlayerController(" + player + players[static_cast<int>(player_data->controller_id - 1)] + ")\n");
		}

		for (size_t a = 0; a < worldData->player_count; a++)
		{
			PlayerData* data = &worldData->players[a];

			if (player_data->controller_id == 0 && data->controller_id == 1) 
			{
				writer.write_string("\tcall SetPlayerAlliance(" + player + "Player(" + std::to_string(data->id) + "), ALLIANCE_RESCUABLE, true)\n");
			}
		}

		writer.write_string("\n");
	}

	writer.write_string("endfunction\n\n");

	writer.write_string("function InitCustomTeams takes nothing returns nothing\n");

	for (size_t i = 0; i < worldData->steam_count; i++) {
		TeamData* data = &worldData->teams[i];
		std::string index = std::to_string(i);

		uint32_t force_flags = data->force_flags;

		bool allied = force_flags & 0b00000001;
		bool allied_victory = force_flags & 0b00000010;
		bool share_vision = force_flags & 0b00001000;
		bool share_unit_control = force_flags & 0b00010000;
		bool share_advanced_unit_control = force_flags & 0b00100000;

		std::string post_state;
		writer.write_string("\t// Force: " + std::string(data->name) + "\n");

		for (size_t a = 0; a < worldData->player_count; a++) {
			PlayerData* player = &worldData->players[a];

			if (data->player_masks & (1 << player->id)) {
				std::string id = std::to_string(player->id);

				writer.write_string("\tcall SetPlayerTeam(Player(" + id + "), " + index + ")\n");

				if (allied_victory) {
					writer.write_string("\tcall SetPlayerState(Player(" + id + "), PLAYER_STATE_ALLIED_VICTORY, 1)\n");
				}

				for (size_t b = 0; b < worldData->player_count; b++) {
					PlayerData* p = &worldData->players[b];
					if (data->player_masks & (1 << p->id) && player->id != p->id) {
						std::string id2 = std::to_string(p->id);
						if (allied) {
							post_state += "\tcall SetPlayerAllianceStateAllyBJ(Player(" + id + "), Player(" + id2 + "), true)\n";
						}
						if (share_vision) {
							post_state += "\tcall SetPlayerAllianceStateVisionBJ(Player(" + id + "), Player(" + id2 + "), true)\n";
						}
						if (share_unit_control) {
							post_state += "\tcall SetPlayerAllianceStateControlBJ(Player(" + id + "), Player(" + id2 + "), true)\n";
						}
						if (share_advanced_unit_control) {
							post_state += "\tcall SetPlayerAllianceStateFullControlBJ(Player(" + id + "), Player(" + id2 + "), true)\n";
						}
					}
				}
			}
		}

		if (!post_state.empty()) {
			
			writer.write_string(post_state);
		}

		writer.write_string("\n");
	}
	writer.write_string("endfunction\n");


	writer.write_string("function InitAllyPriorities takes nothing returns nothing\n");



	for (size_t i = 0; i < worldData->player_count; i++) {
		PlayerData* player = &worldData->players[i];
		uint32_t id = player->id;
		uint32_t slot = player_to_startloc[id];

		std::string player_text;

		int current_index = 0;
		for (size_t j = 0; j < worldData->player_count; j++) {
			PlayerData* target = &worldData->players[j];
			uint32_t id2 = target->id;
			if (player->low_level & (1 << id2) && id != id2) {
				player_text += "\tcall SetStartLocPrio(" + std::to_string(slot) + ", " + std::to_string(current_index) + ", " + std::to_string(id2) + ", MAP_LOC_PRIO_LOW)\n";
				current_index++;
			}
			else if (player->height_level & (1 << id2) && id != id2) {
				player_text += "\tcall SetStartLocPrio(" + std::to_string(slot) + ", " + std::to_string(current_index) + ", " + std::to_string(id2) + ", MAP_LOC_PRIO_HIGH)\n";
				current_index++;
			}
		}
		if (current_index > 0)
		{
			writer.write_string("\tcall SetStartLocPrioCount(" + std::to_string(slot) + ", " + std::to_string(current_index) + ")\n");
		}
		writer.write_string(player_text);
	}
	writer.write_string("endfunction\n");




	writer.write_string(seperator);
	writer.write_string("//*\n");
	writer.write_string("//*  Main Initialization\n");
	writer.write_string("//*\n");
	writer.write_string(seperator);

	writer.write_string("function main takes nothing returns nothing\n");

	// 修正未设置地图镜头范围导致，获取数据均为0的bug
	if (worldData->camera_left_bottom_x == 0 && worldData->camera_left_bottom_y == 0
		&& worldData->camera_right_top_x == 0 && worldData->camera_right_top_y == 0
		&& worldData->camera_left_top_x == 0 && worldData->camera_left_top_y == 0
		&& worldData->camera_right_bottom_x == 0 && worldData->camera_right_bottom_y == 0
		) {
		worldData->camera_left_bottom_x = -3328.0;
		worldData->camera_left_bottom_y = -3584.0;
		worldData->camera_right_top_x = 3328.0;
		worldData->camera_right_top_y = 3072.0;
		worldData->camera_left_top_x = -3328.0;
		worldData->camera_left_top_y = 3072.0;
		worldData->camera_right_bottom_x = 3328.0;
		worldData->camera_right_bottom_y = -3584.0;
	}
	std::string soto = "\tcall SetCameraBounds(" +
		std::to_string(worldData->camera_left_bottom_x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT), " +
		std::to_string(worldData->camera_left_bottom_y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM), " +
	
		std::to_string(worldData->camera_right_top_x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT), " +
		std::to_string(worldData->camera_right_top_y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP), " +
	
		std::to_string(worldData->camera_left_top_x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT), " +
		std::to_string(worldData->camera_left_top_y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP), " +
	
		std::to_string(worldData->camera_right_bottom_x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT), " +
		std::to_string(worldData->camera_right_bottom_y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM))\n";
	
	writer.write_string(soto);
	
	const std::string tileset = std::string((char*)&worldData->tileset,1);
	
	std::string light = tileset;
	//如果有自定义光照
	if (worldData->light) {
		light = std::string((char*)&worldData->light, 1);
	}

	const std::string terrain_lights = string_replaced(worldEditor.getConfigData("TerrainLights", light), "\\", "\\\\");
	const std::string unit_lights = string_replaced(worldEditor.getConfigData("UnitLights", light), "\\", "\\\\");
	
	writer.write_string("\tcall SetDayNightModels(\"" + terrain_lights + "\", \"" + unit_lights + "\")\n");
	

	//如果有迷雾
	if (worldData->mapset_flag & 0x2000) {
		writer.write_string(std::format("\tcall SetTerrainFogEx({}, {:.2f}, {:.2f}, {:.3f}, {:.3f}, {:.3f}, {:.3f})\n",
			worldData->fog_type,
			worldData->fog_z_start,
			worldData->fog_z_end,
			worldData->fog_density,
			(float)worldData->fog_color[2] / 255.f,
			(float)worldData->fog_color[1] / 255.f,
			(float)worldData->fog_color[0] / 255.f
		));
	}
	//如果有设置水颜色
	if (worldData->mapset_flag & 0x10000) {
		writer.write_string(std::format("\tcall SetWaterBaseColor({}, {}, {}, {})\n",
			worldData->water_color[2],
			worldData->water_color[1],
			worldData->water_color[0],
			worldData->water_color[3]
		));
	}

	//如果有全局天气
	if (worldData->climate_id) {
		writer.write_string(std::format("\tcall EnableWeatherEffect(AddWeatherEffect(Rect({}, {}, {}, {}), '{}'), true)\n",
			worldData->terrain->map_rect_minx,
			worldData->terrain->map_rect_miny,
			worldData->terrain->map_rect_maxx,
			worldData->terrain->map_rect_maxy,
			std::string((const char*)&worldData->climate_id, 4)
		));
	}

	std::string sound_environment;
	//环境音效
	if (worldData->custorm_sound && strlen(worldData->custorm_sound) > 0) {
		sound_environment = worldData->custorm_sound;
	} else {
		sound_environment = worldEditor.getConfigData("SoundEnvironment", tileset);
	}

	writer.write_string("\tcall NewSoundEnvironment(\"" + sound_environment + "\")\n");
	
	const std::string ambient_day = worldEditor.getConfigData("DayAmbience", tileset);
	writer.write_string("\tcall SetAmbientDaySound(\"" + ambient_day + "\")\n");
	
	const std::string ambient_night = worldEditor.getConfigData("NightAmbience", tileset);
	writer.write_string("\tcall SetAmbientNightSound(\"" + ambient_night + "\")\n");
	
	writer.write_string("\tcall SetMapMusic(\"Music\", true, 0)\n");
	writer.write_string("\tcall InitSounds()\n");
	writer.write_string("\tcall InitRandomGroups()\n");
	writer.write_string("\tcall CreateRegions()\n");
	writer.write_string("\tcall CreateCameras()\n");
	writer.write_string("\tcall CreateDestructables()\n");
	writer.write_string("\tcall CreateItems()\n");
	writer.write_string("\tcall CreateUnits()\n");
	writer.write_string("\tcall InitBlizzard()\n");
	writer.write_string("\tcall InitGlobals()\n");
	writer.write_string("\tcall InitCustomTriggers()\n");
	writer.write_string("\tcall RunInitializationTriggers()\n");
	
	writer.write_string("endfunction\n");



	writer.write_string(seperator);
	writer.write_string("//*\n");
	writer.write_string("//*  Map Configuration\n");
	writer.write_string("//*\n");
	writer.write_string(seperator);

	writer.write_string("function config takes nothing returns nothing\n");

	writer.write_string("\tcall SetMapName(\"" + std::string(worldData->map_name) + "\")\n");
	writer.write_string("\tcall SetMapDescription(\"" + std::string(worldData->description) + "\")\n");
	writer.write_string("\tcall SetPlayers(" + std::to_string(worldData->player_count) + ")\n");
	writer.write_string("\tcall SetTeams(" + std::to_string(worldData->player_count) + ")\n");
	writer.write_string("\tcall SetGamePlacement(MAP_PLACEMENT_TEAMS_TOGETHER)\n");

	writer.write_string("\n");

	for (size_t i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];
		if (strncmp(unit->name,"sloc",4) == 0) 
		{
			uint32_t slot = player_to_startloc[unit->player_id % 16];
			if (slot < 16)
			{
				writer.write_string("\tcall DefineStartLocation(" + std::to_string(slot) + ", " + std::to_string(unit->x) + ", " + std::to_string(unit->y) + ")\n");
			}
		}
	}

	writer.write_string("\n");

	writer.write_string("\tcall InitCustomPlayerSlots()\n");
	writer.write_string("\tcall InitCustomTeams()\n");
	writer.write_string("\tcall InitAllyPriorities()\n");

	writer.write_string("endfunction\n");

	//std::cout << std::string_view((const char*)&writer.buffer[0],writer.buffer.size());

	std::string outpath = std::string(path);
	
	if (fs::path(outpath).extension().string() != ".j")
	{
		outpath = outpath + ".j";
	}
	std::ofstream out (outpath, std::ios::binary);
	writer.finish(out);
	out.close();


	print("自定义jass 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);



	m_initFuncTable.clear();

	mh::g_initTriggerMap.clear();

}


std::string TriggerEditor::convertTrigger(Trigger* trigger)
{
	std::string result;

	bool is_init = 0;
	bool is_disable = 0;
	bool has_custorm_code = 0;

	is_convert = true;
	if (hasBlackAction(trigger, &is_init, &is_disable) && g_make_editor_data == nullptr) {
		result += originConvertTrigger(trigger);
		if (is_init) {
			mh::g_initTriggerMap.emplace(trigger, true);
		}
		if (is_disable) {
			mh::g_disableTriggerMap.emplace(trigger, true);
		}
		std::string name = trigger->name;
		convert_name(name);
		std::string init_func = "InitTrig_" + name;
		m_initFuncTable[init_func] = true;
	} else {
		
		SetActionToTextBufferSize(0x808);
		mh::NodePtr node = mh::NodeFromTrigger(trigger);
		result += node->toString();
		
		m_param_action_parent_map.clear();


		SetActionToTextBufferSize(0x104);
	}
	is_convert = false;

	return result;
}


TriggerType* TriggerEditor::getTypeData(const std::string& type)
{
	auto it = m_typesTable.find(type);
	if (it != m_typesTable.end())
	{
		return it->second;
	}
	return nullptr;
}

std::string TriggerEditor::getBaseType(const std::string& type)
{
	auto it = m_typesTable.find(type);
	if (it != m_typesTable.end())
	{
		if (*it->second->base_type)
		{
			return it->second->base_type;
		}
	}
	return type;
}


std::string TriggerEditor::getScriptName(Action* action)
{
	std::string name = action->name;
	std::string key = "_" + name + "_ScriptName";
	std::string parent_key = "TriggerActions";
	if (get_action_type(action) == Action::event) {
		parent_key = "TriggerEvents";
	}
	auto& world = get_world_editor();

	std::string func_name = world.getConfigData(parent_key, key, 0);
	if (func_name.length() > 0) {
		return func_name;
	}
	return name;
}


bool TriggerEditor::hasBlackAction(Trigger* trigger, bool* is_init, bool* is_disable)
{
	//if (m_blacklist_map.empty())
	//	return false;

	m_param_action_parent_map.clear();

	std::function<void(Action* action)> action_has_black;
	std::function<void(Parameter** params, uint32_t count)> param_has_black;

	std::map<std::string, bool> black_actions;

	action_has_black = [&](Action* action) {
		if (!action->enable)
			return;

		std::string name = action->name;
		if (m_blacklist_map.find(name) != m_blacklist_map.end()) {
			black_actions.emplace(name, true);
		}
		param_has_black(action->parameters, action->param_count);

		for (size_t i = 0; i < action->child_count; i++) {
			Action* child = action->child_actions[i];
			if ((int)child->group_id < fakeGetChildCount(action)) {
				action_has_black(child);
			}
		}
	};

	param_has_black = [&](Parameter** params, uint32_t count) {
		for (size_t i = 0; i < count; i++) {
			Parameter* param = params[i];
			switch (param->typeId)
			{
			case Parameter::Type::variable:
				if (param->arrayParam) {
					param_has_black(&param->arrayParam, 1);
				}
				break;
			case Parameter::Type::function:
				if (param->funcParam) {
					m_param_action_parent_map[param->funcParam] = param;
				}
				action_has_black(param->funcParam);
				break;
			}
		}
	};

	for (size_t i = 0; i < trigger->line_count; i++)
	{
		Action* action = trigger->actions[i];


		if (action->enable && action->table->getType(action) == Action::Type::event) {
			int hash = hash_(action->name);
			if (hash == "YDWEDisableRegister"s_hash) {
				*is_disable = true;
			}else if (hash == "MapInitializationEvent"s_hash) {
				*is_init = true;
			}
		}
		action_has_black(action);
	}

	if (!black_actions.empty()) {
		auto& world = get_world_editor();

		print("Warning: 触发器[%s]: 使用了黑名单动作, 该触发将不会进行加速，尽量减少使用。\n", base::u2a(trigger->name).c_str());

		for (auto&& [name, v] : black_actions) {
			std::string ui_name = base::u2a(world.getConfigData("TriggerActionStrings", name, 0));
			if (ui_name.length() == 0) {
				ui_name = base::u2a(world.getConfigData("TriggerCallStrings", name, 0));
			}
			if (ui_name.length() == 0) {
				ui_name = base::u2a(world.getConfigData("TriggerEventStrings", name, 0));
			}

			std::string action_name = base::u2a(name);
			print("\t<%s> <%s>\n", ui_name.c_str(), action_name.c_str());
		}
		print("\n");
	}

	return !black_actions.empty();
}

std::string TriggerEditor::originConvertTrigger(Trigger* trigger)
{
	std::string result;

	if (trigger->is_custom_srcipt || trigger->is_comment) 
	{
		return result;
	}

	auto& world = get_world_editor();

	struct ConvertData
	{
		uintptr_t ptr = 0;
		const char* script = nullptr;
		uintptr_t unknow = 0;
		size_t buffer_size = 0;
		size_t script_len = 0;
		uint32_t unknow2 = -1;
	};

	uintptr_t convert = world.getAddress(0x005CA4C0);
	uintptr_t delete_buffer = world.getAddress(0x00425580);

	ConvertData data;
	data.ptr = world.getAddress(0x0075A3A8);
	this_call<uint32_t>(convert, trigger, &data);

	result = std::string(data.script, data.script_len);

	data.ptr = world.getAddress(0x0075A3A8);
	this_call<void>(delete_buffer, &data, &data.script, &data.unknow, &data.buffer_size);

	return result;
}


std::string TriggerEditor::originConvertActionText(Action* action) {
	std::string result;


	auto& world = get_world_editor();

	struct ConvertData {
		uintptr_t ptr = 0;
		const char* text = nullptr;
		uintptr_t unknow = 0;
		size_t buffer_size = 0;
		size_t text_len = 0;
		uint32_t unknow2 = -1;
	};

	uintptr_t convert = world.getAddress(0x005DBC40);
	uintptr_t delete_buffer = world.getAddress(0x00425580);

	ConvertData data;
	data.ptr = world.getAddress(0x0075A3A8);
	this_call<uint32_t>(convert, action, &data);

	result = std::string(data.text, data.text_len);

	data.ptr = world.getAddress(0x0075A3A8);
	this_call<void>(delete_buffer, &data, &data.text, &data.unknow, &data.buffer_size);

	return result;
}

bool TriggerEditor::onConvertTrigger(Trigger* trigger)
{
	if (trigger->is_custom_srcipt || trigger->is_comment)
		return false;

	std::string pre_actions;


	auto& world = get_world_editor();

	const auto script = convertTrigger(trigger);
	
	if (mh::g_initTriggerMap.find(trigger) != mh::g_initTriggerMap.end())
	{
		trigger->is_initialize = 1;
		mh::g_initTriggerMap.erase(trigger);
	}

	trigger->is_custom_srcipt = 1;

	//写入字符串到触发器中
	this_call<int>(world.getAddress(0x005CB280), trigger, script.c_str(), script.size());

	if (trigger->line_count > 0)
	{
		//遍历销毁所有动作
		for (size_t i = 0; i < trigger->line_count; i++)
		{
			const auto action = trigger->actions[i];
			if (action)
			{
				action->table->destroy(action, 1);
			}
		}

		if (trigger->actions)
		{
#define SMemFreeIndex 403
			//销毁动作容器
			const auto SMemFreeAddr = reinterpret_cast<uintptr_t>(GetProcAddress(
				GetModuleHandleW(L"Storm.dll"), reinterpret_cast<const char*>(SMemFreeIndex)));
			std_call<BOOL>(SMemFreeAddr, trigger->actions, ".PAVCWETriggerFunction@@", -0x2, 0);	
		}
		trigger->number = 0;
		trigger->line_count = 0;
		trigger->actions = 0;
	}
	return true;
}


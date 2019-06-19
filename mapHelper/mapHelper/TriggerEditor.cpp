#include "stdafx.h"
#include "YDTrigger.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <regex>

TriggerEditor::TriggerEditor()
	:m_editorData(NULL),
	m_version(7)
{ 
	space_stack = 0;
	for (int i = 0; i < 200; i++)
	{
		for (int k = 0; k < i; k++)
		{
			spaces[i] += '\t';
		}
	}
	
}

TriggerEditor::~TriggerEditor()
{ }

TriggerEditor* TriggerEditor::getInstance()
{
	static TriggerEditor instance; 
	return &instance;
}

void TriggerEditor::loadTriggers(TriggerData* data)
{
	m_editorData = data;

}

void TriggerEditor::loadTriggerConfig(TriggerConfigData* data)
{
	m_configData = data;
	printf("读取配置文件\n");
	for (size_t i = 0; i < data->type_count; i++)
	{
		TriggerType* type_data = &data->array[i];
		m_typesTable[type_data->type] = type_data;
	}
	m_ydweTrigger = YDTrigger::getInstance();
}

void TriggerEditor::saveTriggers(const char* path)
{
	if (!m_editorData)
	{
		printf("缺少触发器数据\n");
		return;
	}

	printf("自定义保存wtg文件\n");

	clock_t start = clock();




	BinaryWriter writer;


	uint32_t head = '!GTW';


	writer.write(head);

	m_version = 7;
	writer.write(m_version);
	
	writeCategoriy(writer);

	writeVariable(writer);

	writeTrigger(writer);

	std::ofstream out(std::string(path) + ".wtg", std::ios::binary);
	writer.finish(out);
	out.close();

	printf("wtg 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}

void TriggerEditor::writeCategoriy(BinaryWriter& writer)
{
	Categoriy** categories = m_editorData->categories;

	uint32_t count = m_editorData->categoriy_count;

	writer.write(count);

	for (size_t i = 0; i < count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		
		writer.write(categoriy->categoriy_id);

		std::string categoriy_name = std::string(categoriy->categoriy_name);

		writer.write_c_string(categoriy_name);

		writer.write(categoriy->is_comment);

		categoriy->has_change = 0;

	}

}
void TriggerEditor::writeVariable(BinaryWriter& writer)
{
	Variable* variables = m_editorData->variables;


	uint32_t unknow = 2;
	//未知 总是写2
	writer.write(unknow);

	uint32_t variable_count = 0;

	for(size_t i = 0; i < variables->globals_count ; i++)
	{
		VariableData* data = &variables->array[i];
		//名字非gg_开头的变量
		if (data && strncmp(data->name, "gg_", 3))
			variable_count++;
	}



	writer.write(variable_count);

	//将非gg_的变量数据写入
	for (size_t i = 0; i < variables->globals_count; i++)
	{
		VariableData* data = &variables->array[i];
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
	uint32_t count = m_editorData->trigger_count;
	writer.write(count);

	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{

			Trigger* trigger = categoriy->triggers[n];
	
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
		Action* action = trigger->actions[i];

		uint32_t actionType = action->table->getType(action);

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

		Parameter* param = action->parameters[i];
		writeParameter(writer, param);
	}

	uint32_t child_count = action->child_count;
	writer.write(child_count);
	//如果是 动作组 则循环将子动作写入
	for (size_t i = 0; i < child_count; i++)
	{
		Action* child = action->child_actions[i];

		uint32_t child_type = child->table->getType(child);

		writer.write(child_type);

		writer.write(child->child_flag);

		writeAction(writer, child);
	}
}

void TriggerEditor::writeParameter(BinaryWriter& writer, Parameter* param)
{
	uint32_t type = param->typeId;
	writer.write(type);

	writer.write_c_string(param->value);

	uint32_t has_param = (type == Parameter::function && param->funcParam);

	writer.write(has_param);
	if (has_param)
	{
		Action* action = param->funcParam;

		uint32_t actionType = action->table->getType(action);

		writer.write(actionType);

		writeAction(writer, action);

	}

	uint32_t is_array = param->arrayParam != NULL;
	writer.write(is_array);

	//如果是数组 则写入数组中的参数
	if (is_array)
	{
		Parameter* child = param->arrayParam;
		writeParameter(writer,child);
	}

}





void TriggerEditor::saveScriptTriggers(const char* path)
{
	printf("自定义保存wct文件\n");

	clock_t start = clock();

	TriggerData* data = m_editorData;

	BinaryWriter writer;

	uint32_t version = 1;
	writer.write(version);

	writer.write_c_string(data->global_jass_comment);

	writer.write(data->globals_jass_size);

	std::string_view jass(data->globals_jass_script, data->globals_jass_size);
	writer.write_string(jass);

	writer.write(data->trigger_count);

	for (size_t i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			Trigger* trigger = categoriy->triggers[n];
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

	printf("wct 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}


void TriggerEditor::saveSctipt(const char* path)
{

	printf("自定义保存jass文件\n");

	clock_t start = clock();

	TriggerData* data = m_editorData;



	BinaryWriter writer,writer2;

	WorldEditor* worldEditor = WorldEditor::getInstance();
	EditorData* worldData = worldEditor->getEditorData();

	char buffer[0x400];
	std::map<std::string, VariableData*> variableTable;

	writer.write_string(seperator);
	writer.write_string("//*\n");
	writer.write_string("//*  Global variables\n");
	writer.write_string("//*\n");
	writer.write_string(seperator);

	if (m_ydweTrigger->isEnable())
	{
		m_ydweTrigger->onGlobals(writer);
	}
	
	writer.write_string("globals\n");

	for (size_t i = 0; i < data->variables->globals_count; i++)
	{
		VariableData* var = &data->variables->array[i];
		std::string name = var->name;
		std::string type = var->type;
		std::string base = getBaseType(type);

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
			std::string value = var->value;
			if (value.length() == 0)
			{
				auto it = m_typesTable.find(type);
				if (it != m_typesTable.end())
					value = it->second->value;
				if (value.length() == 0)
				{
					if (base == "integer" || base == "real")
					{
						value = "0";
					}
					else
					{
						value = "null";
					}
					
				}
				
			}
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

	if (m_ydweTrigger->isEnable())
	{
		m_ydweTrigger->onEndGlobals(writer);
	}
	else
	{
		writer.write_string("endglobals\n");
	}

	//开始初始化全局变量
	writer.write_string("function InitGlobals takes nothing returns nothing\n");
	writer.write_string("\tlocal integer i = 0\n");

	for (size_t i = 0; i < data->variables->globals_count; i++)
	{
		VariableData* var = &data->variables->array[i];
		std::string name = var->name;
		 
		if (var->is_array )
		{
			//获取默认值
			std::string type = var->type;

			std::string defaultValue;
			auto it = m_typesTable.find(type);
			if (it != m_typesTable.end())
				defaultValue = it->second->value;
			if (!var->is_init && defaultValue.empty()) 
				continue;
			writer.write_string("\tset i = 0\n");
			writer.write_string("\tloop\n");
			writer.write_string("\t\texitwhen(i > " + std::to_string(var->array_size) + ")\n");

			if (var->is_init) 
			{
				std::string value = var->value;
				if (type == "string" && value.empty()) 
				{
					writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
				}
				else 
				{
					writer.write_string("\t\tset udg_" + name + "[i] = " + value + "\n");
				}
			}
			else 
			{
				if (type == "string") 
				{
					writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
				}
				else 
				{
					writer.write_string("\t\tset udg_" + name + "[i] = " + defaultValue + "\n");
				}
			}
			writer.write_string("\t\tset i = i + 1\n");
			writer.write_string("\tendloop\n");
		}
		else if (var->is_init) 
		{
			writer.write_string("\tset udg_" + name + " = " + var->value + "\n");
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
				std::string value = "-1";
				if (*ptr) value = "'" + std::string(ptr, ptr + 0x4) + "'";
				
				sprintf(buffer, "\t\tset gg_rg_%03d[%i] = %s\n", i, b, value.c_str());
				writer.write_string(buffer);
			}
		}
		writer.write_string("\telse\n");
		for (uint32_t b = 0; b < groupData->param_count; b++)
		{
			sprintf(buffer, "\t\tset gg_rg_%03d[%i] = -1\n", i, b);
			writer.write_string(buffer);
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
				std::string id = std::string(info->name, info->name + 0x4);

				writer.write_string("\t\tcall RandomDistAddItem('" + id + "', " + std::to_string(info->rate) + ")\n");
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
					std::string id = std::string(info->name, info->name + 0x4);
					writer.write_string("\t\tcall RandomDistAddItem('" + id + "', " + std::to_string(info->rate) + ")\n");
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
					std::string id = std::string(info->name, info->name + 0x4);
					writer.write_string("\t\tcall RandomDistAddItem('" + id + "', " + std::to_string(info->rate) + ")\n");
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

		int duration = WorldEditor::getInstance()->getSoundDuration(sound->file);
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
				writer.write_string("\tcall SetSoundConeAngles(gg_snd_UndeadVictory," + std::string(buffer) + ")\n");

				sprintf(buffer, "%.1f,%.1f,%.1f", sound->x,sound->y,sound->z);
				writer.write_string("\tcall SetSoundConeOrientation(gg_snd_UndeadVictory," + std::string(buffer) + ")\n");
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

		if (variableTable.find(id) == variableTable.end()) 
			continue;
		
		sprintf(buffer, "'%.4s',%.1f,%.1f,%.1f,%.1f,%d", unit->name, unit->x, unit->y, unit->angle, unit->scale_x, unit->variation);
		writer.write_string("\tset " + id + " = CreateDestructable(" + std::string(buffer) + ")\n");

		if (unit->doodas_life != 100) {
			sprintf(buffer, "%.2f", unit->doodas_life / 100.f);
			writer2.write_string("\tset life = GetDestructableLife(" + id + ")\n");
			writer2.write_string("\tcall SetDestructableLife(" + id + ", " + std::string(buffer) + " * life)\n");
		}

		if (unit->item_setting_count > 0) {
			sprintf(buffer, "%06d", unit->index);

			writer2.write_string("\tset t = CreateTrigger()\n");
			writer2.write_string("\tcall TriggerRegisterDeathEvent(t, " + id + ")\n");
			writer2.write_string("\tcall TriggerAddAction(t, function SaveDyingWidget)\n");
			writer2.write_string("\tcall TriggerAddAction(t, function Doodad" + std::string(buffer) + "_DropItems)\n");
		}
		else if (unit->item_table_index != -1) {
			sprintf(buffer, "%06d", unit->item_table_index);

			writer2.write_string("\tset t = CreateTrigger()\n");
			writer2.write_string("\tcall TriggerRegisterDeathEvent(t, " + id + ")\n");
			writer2.write_string("\tcall TriggerAddAction(t, function SaveDyingWidget)\n");
			writer2.write_string("\tcall TriggerAddAction(t, function ItemTable_" + std::string(buffer) + "_DropItems)\n");
		}

	}
	writer.write_bw(writer2);


	writer.write_string("endfunction\n");


	writer.write_string("function CreateItems takes nothing returns nothing\n");

	writer.write_string("\tlocal integer itemID\n");


	//所有物品类型的jass变量名
	std::string randomTypes[] = {
		"ITEM_TYPE_ANY",
		"ITEM_TYPE_PERMANENT",
		"ITEM_TYPE_CHARGED",
		"ITEM_TYPE_POWERUP",
		"ITEM_TYPE_ARTIFACT",
		"ITEM_TYPE_PURCHASABLE",
		"ITEM_TYPE_CAMPAIGN",
		"ITEM_TYPE_MISCELLANEOUS"
	};

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
					writer.write_string("\tset itemID=ChooseRandomItemEx(" + randomTypes[unit->random_item_type % 8] + ", " + std::to_string(unit->random_item_level) + ")\n");
				}
				else if (unit->random_item_mode == 1)
				{
					//随机组里面获取
					sprintf(buffer, "\t set itemID=gg_rg_%03d[%i]\n", unit->random_group_index,unit->random_group_child_index);

					writer.write_string(buffer);
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

		std::string unit_reference = "u";


		sprintf(buffer, "Player(%i),'%.4s',%.1f,%.1f,%.1f", unit->player_id, unit->name, unit->x, unit->y, unit->angle * 180.f / 3.141592f);
		writer.write_string("\tset " + unit_reference + " = CreateUnit(" + std::string(buffer) + ")\n");

		sprintf(buffer, "gg_unit_%.04s_%04d", unit->name, unit->index);
		std::string name = buffer;
		if (variableTable.find(name) != variableTable.end())
		{
			writer.write_string("\tset " + name + " = u\n");
		}

		if (unit->health != -1) 
		{
			writer.write_string("\tset life = GetUnitState(" + unit_reference + ", UNIT_STATE_LIFE)\n");
			writer.write_string("\tcall SetUnitState(" + unit_reference + ", UNIT_STATE_LIFE, " + std::to_string(unit->health / 100.f) + "* life)\n");
		}

		if (unit->mana != -1) 
			writer.write_string("\tcall SetUnitState(" + unit_reference + ", UNIT_STATE_MANA, " + std::to_string(unit->mana) + ")\n");
		
		if (unit->level != 1) 
			writer.write_string("\tcall SetHeroLevel(" + unit_reference + ", " + std::to_string(unit->level) + ", false)\n");
		

		if (unit->state_str != 0) 
			writer.write_string("\tcall SetHeroStr(" + unit_reference + ", " + std::to_string(unit->state_str) + ", true)\n");
		

		if (unit->state_agi != 0) 
			writer.write_string("\tcall SetHeroAgi(" + unit_reference + ", " + std::to_string(unit->state_agi) + ", true)\n");
		

		if (unit->state_int != 0) 
			writer.write_string("\tcall SetHeroInt(" + unit_reference + ", " + std::to_string(unit->state_int) + ", true)\n");
		
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
			writer.write_string("\tcall SetUnitAcquireRange(" + unit_reference + ", " + std::to_string(range) + ")\n");
		}

		for (size_t a = 0; a < unit->skill_count; a++)
		{
			UnitSkill* skill = &unit->skills[a];
			std::string skillId = std::string(skill->name, skill->name + 0x4);
			for (uint32_t b = 0; b < skill->level; b++) 
			{
				writer.write_string("\tcall SelectHeroSkill(" + unit_reference + ", \'" + std::string(skillId) + "\')\n");
			}
		
			std::string objectValue;
		
			
			if (skill->is_enable)
			{
				worldEditor->getSkillObjectData(*(uint32_t*)(skill->name), 0, "Orderon", objectValue);
				if (objectValue.empty()) 
					worldEditor->getSkillObjectData(*(uint32_t*)(skill->name), 0, "Order", objectValue);
				writer.write_string("\tcall IssueImmediateOrder(" + unit_reference + ", \"" + objectValue + "\")\n");
			} 
			else 
			{
				worldEditor->getSkillObjectData(*(uint32_t*)(skill->name), 0, "Orderoff", objectValue);
				if (!objectValue.empty()) 
					writer.write_string("\tcall IssueImmediateOrder(" + unit_reference + ", \"" + objectValue + "\")\n");
			}
		}
		
		for (size_t a = 0; a < unit->item_count; a++)
		{
			UnitItem* item = &unit->items[a];
			writer.write_string("\tcall UnitAddItemToSlotById(" + unit_reference + ", '" + std::string(item->name,item->name + 0x4) + "', " + std::to_string(item->slot_id) + ")\n");
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
			sprintf(buffer, "ItemTable%06d_DropItems", unit->item_table_index);
			dropName = buffer;
		}
		if (!dropName.empty())
		{
			
			writer.write_string("\tset t = CreateTrigger()\n");
			writer.write_string("\tcall TriggerRegisterUnitEvent(t, " + unit_reference + ", EVENT_UNIT_DEATH)\n");
			writer.write_string("\tcall TriggerRegisterUnitEvent(t, " + unit_reference + ", EVENT_UNIT_CHANGE_OWNER)\n");
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
			if (trigger->is_comment || !trigger->is_enable)
			{
				continue;
			}

			if (m_initTriggerTable.find(trigger) != m_initTriggerTable.end() || trigger->is_initialize)
			{
				std::string trigger_name = std::string(trigger->name);
				convert_name(trigger_name);

				std::string trigger_variable_name = "gg_trg_" + trigger_name;

				initions += "\tcall ConditionalTriggerExecute(" + trigger_variable_name + ")\n";
				
			}

			if (m_ydweTrigger->isEnable() && m_ydweTrigger->hasDisableRegister(trigger))
			{
				continue;
			}

			std::string name = "InitTrig_" + std::string(trigger->name);
			convert_name(name);
			if (m_initFuncTable.find(name) == m_initFuncTable.end())
			{
				continue;
			}
			writer.write_string("\tcall " + name + "()\n");

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

	for (size_t i = 0; i < worldData->player_count; i++)
	{
		PlayerData* player_data = &worldData->players[i];
		std::string id = std::to_string(i);
		std::string player = "Player(" + id + "), ";
		writer.write_string("\tcall SetPlayerStartLocation(" + player + id + ")\n");
		if (player_data->is_lock || player_data->race == 0) 
		{
			writer.write_string("\tcall ForcePlayerStartLocation(" + player + id + ")\n");
		}

		writer.write_string("\tcall SetPlayerColor(" + player + "ConvertPlayerColor(" + id + "))\n");
		writer.write_string("\tcall SetPlayerRacePreference(" + player + races[static_cast<int>(player_data->race)] + ")\n");
		writer.write_string("\tcall SetPlayerRaceSelectable(" + player + "true" + ")\n");
		writer.write_string("\tcall SetPlayerController(" + player + players[static_cast<int>(player_data->controller_id)] + ")\n");

		for (size_t a = 0; a < worldData->player_count; a++)
		{
			PlayerData* data = &worldData->players[a];

			if (player_data->controller_id == 0 && data->controller_id == 1) 
			{
				writer.write_string("\tcall SetPlayerAlliance(" + player + "Player(" + std::to_string(a) + "), ALLIANCE_RESCUABLE, true)\n");
			}
		}

		writer.write_string("\n");
	}

	writer.write_string("endfunction\n\n");

	writer.write_string("function InitCustomTeams takes nothing returns nothing\n");

	for (size_t i = 0; i < worldData->steam_count; i++) {
		SteamData* data = &worldData->steams[i];
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

			if (data->player_masks & (1 << a)) {
				std::string id = std::to_string(a);

				writer.write_string("\tcall SetPlayerTeam(Player(" + id + "), " + index + ")\n");

				if (allied_victory) {
					writer.write_string("\tcall SetPlayerState(Player(" + id + "), PLAYER_STATE_ALLIED_VICTORY, 1)\n");
				}

				for (size_t b = 0; b < worldData->player_count; b++) {
					PlayerData* p = &worldData->players[b];
					if (data->player_masks & (1 << b) && a != b) {
						std::string id2 = std::to_string(b);
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

		std::string player_text;

		int current_index = 0;
		for (size_t j = 0; j < worldData->player_count; j++) {
			PlayerData* target = &worldData->players[j];
			if (player->low_level & (1 << j) && i != j) {
				player_text += "\tcall SetStartLocPrio(" + std::to_string(i) + ", " + std::to_string(current_index) + ", " + std::to_string(j) + ", MAP_LOC_PRIO_LOW)\n";
				current_index++;
			}
			else if (player->height_level & (1 << j) && i != j) {
				player_text += "\tcall SetStartLocPrio(" + std::to_string(i) + ", " + std::to_string(current_index) + ", " + std::to_string(j) + ", MAP_LOC_PRIO_HIGH)\n";
				current_index++;
			}
		}

		player_text = "\tcall SetStartLocPrioCount(" + std::to_string(i) + ", " + std::to_string(current_index) + ")\n" + player_text;
		writer.write_string(player_text);
	}
	writer.write_string("endfunction\n");




	writer.write_string(seperator);
	writer.write_string("//*\n");
	writer.write_string("//*  Main Initialization\n");
	writer.write_string("//*\n");
	writer.write_string(seperator);

	writer.write_string("function main takes nothing returns nothing\n");
	
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
	
	const std::string terrain_lights = string_replaced(worldEditor->getConfigData("TerrainLights", tileset), "\\", "\\\\");
	const std::string unit_lights = string_replaced(worldEditor->getConfigData("UnitLights",tileset), "\\", "\\\\");
	
	writer.write_string("\tcall SetDayNightModels(\"" + terrain_lights + "\", \"" + unit_lights + "\")\n");
	
	const std::string sound_environment = worldEditor->getConfigData("SoundEnvironment",tileset);
	writer.write_string("\tcall NewSoundEnvironment(\"" + sound_environment + "\")\n");
	
	const std::string ambient_day = worldEditor->getConfigData("DayAmbience", tileset);
	writer.write_string("\tcall SetAmbientDaySound(\"" + ambient_day + "\")\n");
	
	const std::string ambient_night = worldEditor->getConfigData("NightAmbience", tileset);
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
	writer.write_string("\tcall SetTeams(" + std::to_string(worldData->steam_count) + ")\n");
	writer.write_string("\tcall SetGamePlacement(MAP_PLACEMENT_TEAMS_TOGETHER)\n");

	writer.write_string("\n");

	for (size_t i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];
		if (strncmp(unit->name,"sloc",4) == 0) 
		{
			writer.write_string("\tcall DefineStartLocation(" + std::to_string(unit->player_id) + ", " + std::to_string(unit->x) + ", " + std::to_string(unit->y) + ")\n");
		}
	}

	writer.write_string("\n");

	writer.write_string("\tcall InitCustomPlayerSlots()\n");
	writer.write_string("\tcall InitCustomTeams()\n");
	writer.write_string("\tcall InitAllyPriorities()\n");

	writer.write_string("endfunction\n");

	//std::cout << std::string_view((const char*)&writer.buffer[0],writer.buffer.size());

	std::ofstream out(std::string(path) + ".j", std::ios::binary);
	writer.finish(out);
	out.close();


	printf("自定义jass 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);


	m_initFuncTable.clear();

	m_initTriggerTable.clear();

}


std::string TriggerEditor::convertTrigger(Trigger* trigger) 
{
	

	ActionNodePtr root(new ActionNode(trigger));

	std::string trigger_name = *root->getTriggerNamePtr();

	std::string trigger_variable_name = "gg_trg_" + trigger_name;
	std::string trigger_action_name = "Trig_" + trigger_name + "_Actions";


	std::string events;
	std::string conditions;
	std::string pre_actions;
	std::string actions;
	std::string action_code;

	events += "function InitTrig_" + trigger_name + " takes nothing returns nothing\n";
	events += "\tset " + trigger_variable_name + " = CreateTrigger()\n";
	if (trigger->is_disable_init)
	{
		events += "\tcall DisableTrigger(" + trigger_variable_name + ")\n";
	}

	space_stack++;

	m_initFuncTable["InitTrig_" + trigger_name] = true;

	actions += "function " + trigger_action_name + " takes nothing returns nothing\n";

	if (m_ydweTrigger->isEnable())
	{
		m_ydweTrigger->onActionsToFuncBegin(actions, root);

		m_ydweTrigger->onRegisterTrigger(events, root->getName(), trigger_variable_name);
	}

	std::vector<ActionNodePtr> list;
	root->getChildNodeList(list);

	for (auto& node : list)
	{
		Action* action = node->getAction();
		std::string name = action->name;

		switch (node->getActionType())
		{
		case Action::Type::event:
			if (m_ydweTrigger->isEnable())
			{
				//返回false 跳过注册事件
				if (!m_ydweTrigger->onRegisterEvent(events,node))
					continue;

			}
			if (node->getNameId() == "MapInitializationEvent"s_hash )
			{
				m_initTriggerTable[trigger] = true;
				continue;
			}
			events += "\tcall " + name + "(" + trigger_variable_name;

			for (size_t k = 0; k < action->param_count; k++)
			{
				Parameter* param = action->parameters[k];


				std::string type = param->type_name;

				events += ", ";
				events += convertParameter(param, node, pre_actions);
			}
			events += ")\n";
			if (m_ydweTrigger->isEnable())
			{
				m_ydweTrigger->onRegisterEvent2(events,node);
			}

			break;
		case Action::Type::condition:
			conditions += "\tif (not (" + convertAction(node, pre_actions, true) + ")) then\n";
			conditions += "\treturn false\n";
			conditions += "\tendif\n";
			break;
		case Action::Type::action:
			space_stack = 1;
			action_code += spaces[space_stack];

			action_code += convertAction(node, pre_actions, false) + "\n";
			break;
		}
	}

	if (m_ydweTrigger->isEnable())
	{
		actions += action_code;
		m_ydweTrigger->onActionsToFuncEnd(actions,root);
	}
	else
	{
		actions += action_code;
	}
	

	actions += "endfunction\n\n";

	if (!conditions.empty()) {
		conditions = "function Trig_" + trigger_name + "_Conditions takes nothing returns boolean\n" + conditions;
		conditions += "\treturn true\n";
		conditions += "endfunction\n\n";

		events += "\tcall TriggerAddCondition(" + trigger_variable_name + ", Condition(function Trig_" + trigger_name + "_Conditions))\n";
	}

	events += "\tcall TriggerAddAction(" + trigger_variable_name + ", function " + trigger_action_name + ")\n";
	events += "endfunction\n\n";


	return seperator + "// Trigger: " + trigger_name + "\n" + seperator + pre_actions + conditions + actions + seperator + events;
}


std::string TriggerEditor::convertAction(ActionNodePtr node, std::string& pre_actions, bool nested)
{
	Action* action = node->getAction();

	if (!action->enable)
		return "";

	std::string output;

	bool is_loopa = false;
	bool flag = false;

	Parameter** parameters = action->parameters;

	std::vector<ActionNodePtr> list;

	switch (node->getNameId())
	{

	case "WaitForCondition"s_hash:
	{
		output += "loop\n";
		output += spaces[++space_stack];
		output += "exitwhen (" + convertParameter(parameters[0], node, pre_actions) + ")\n";
		output += spaces[space_stack];
		output += "call TriggerSleepAction(RMaxBJ(bj_WAIT_FOR_COND_MIN_INTERVAL, " + convertParameter(parameters[1], node, pre_actions) + "))\n";
		output += spaces[--space_stack];
		output += "endloop";
		return output;
	}
	
	case "ForLoopAMultiple"s_hash:
		is_loopa = true;
	case "ForLoopBMultiple"s_hash:
	{
		std::string loop_index = is_loopa ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		std::string loop_index_end = is_loopa ? "bj_forLoopAIndexEnd" : "bj_forLoopBIndexEnd";

		output += "set " + loop_index + "=" + convertParameter(parameters[0], node, pre_actions) + "\n";
		output += spaces[space_stack];
		output += "set " + loop_index_end + "=" + convertParameter(parameters[1], node, pre_actions) + "\n";
		output += spaces[space_stack];
		output += "loop\n";
		output += spaces[++space_stack];
		output += "exitwhen " + loop_index + " > " + loop_index_end + "\n";
		
		node->getChildNodeList(list);

		for (auto& child : list)
		{
			output += spaces[space_stack];
			output += convertAction(child, pre_actions, false) + "\n";
		}
		output += spaces[space_stack];
		output += "set " + loop_index + " = " + loop_index + " + 1\n";
		output += spaces[--space_stack];
		output += "endloop\n";
		return output;
	}

	case "ForLoopVarMultiple"s_hash:
	{
		std::string variable = convertParameter(parameters[0], node, pre_actions);
		output += "set " + variable + " = ";
		output += convertParameter(parameters[1], node, pre_actions) + "\n";
		output += spaces[space_stack];
		output += "loop\n";
		output += spaces[++space_stack];
		output += "exitwhen " + variable + " > " + convertParameter(parameters[2], node, pre_actions) + "\n";
		
		node->getChildNodeList(list);

		for (auto& child : list)
		{
			output += spaces[space_stack];
			output += convertAction(child, pre_actions, false) + "\n";
		}
		output += spaces[space_stack];
		output += "set " + variable + " = " + variable + " + 1\n";
		output += spaces[--space_stack];
		output += "endloop\n";
		return output;
	}
	
	case "IfThenElseMultiple"s_hash:
	{
		std::string iftext;
		std::string thentext;
		std::string elsetext;


		bool firstBoolexper = true;

		space_stack++;

		node->getChildNodeList(list);

		for (auto& child : list)
		{
			switch (child->getActionType())
			{
			case Action::Type::condition:
			{
				if (firstBoolexper)
				{
					firstBoolexper = false;
				}
				else
				{
					iftext += " and ";
				}
				iftext += "(" + convertAction(child, pre_actions, true) + ")";
				break;
			}
			case Action::Type::action:
			{
				Action* action = child->getAction();
				if (action->child_flag == 1)
				{
					thentext += spaces[space_stack];
					thentext += convertAction(child, pre_actions, false) + "\n";
				}
				else
				{
					elsetext += spaces[space_stack];
					elsetext += convertAction(child, pre_actions, false) + "\n";
				}
				break;
			}
			default:
				break;
			}
		}
		if (iftext.empty())
		{
			iftext += "true";
		}
		space_stack--;
		output += "if (";
		output += iftext;
		output += ") then\n";
		output += thentext;
		output += spaces[space_stack];
		output += "else\n";
		output += elsetext;
		output += spaces[space_stack];
		output += "endif";

		return output;
	}

	case "ForForceMultiple"s_hash:
	case "ForGroupMultiple"s_hash:

	case "EnumDestructablesInRectAllMultiple"s_hash:
	case "EnumDestructablesInCircleBJMultiple"s_hash:
	case "EnumItemsInRectBJMultiple"s_hash:
	{
		const std::string function_name = generate_function_name(node->getTriggerNamePtr());

		std::string name = node->getName();

		// Remove multiple
		output += "call " + name.substr(0, name.length() - 8) + "(";
		
		for (size_t k = 0; k < action->param_count; k++)
		{
			Parameter* param = action->parameters[k];
			if (strcmp(param->type_name, "code") != 0)
			{
				output += convertParameter(param, node, pre_actions);
				output += ",";
			}
		}
		output += " function " + function_name + ")\n";


		std::string toto;

		int stack = space_stack;
		space_stack = 1;

		node->getChildNodeList(list);

		for (auto& child : list)
		{
			toto += spaces[space_stack];
			toto += convertAction(child, pre_actions, false) + "\n";
		}

		pre_actions += "function " + function_name + " takes nothing returns nothing\n";
		if (m_ydweTrigger->isEnable())
		{
			m_ydweTrigger->onActionsToFuncBegin(pre_actions, node);
			pre_actions += toto;
			m_ydweTrigger->onActionsToFuncEnd(pre_actions, node);
		}
		else
		{
			pre_actions += toto;
		}
	
		pre_actions += "\nendfunction\n";
		space_stack = stack;
		return output;
	}

	case "AndMultiple"s_hash:
	{

		std::string iftext;

		node->getChildNodeList(list);
		size_t i = 0;
		for (auto& child : list)
		{
			iftext += "(" + convertAction(child, pre_actions, true) + ")";
			if (++i < list.size())
			{
				iftext += " and ";
			}
		}
		if (i == 0)
		{
			return "true";
		}
		return iftext;
	}

	case "OrMultiple"s_hash:
	{

		std::string iftext;

		node->getChildNodeList(list);
		size_t i = 0;
		for (auto& child : list)
		{
			iftext += convertAction(child, pre_actions, true);
			if (++i < list.size())
			{
				iftext += " or ";
			}
		}
		if (i == 0)
		{
			return "true";
		}
		return iftext;
	}


	case "SetVariable"s_hash:
	{
		const std::string first = convertParameter(parameters[0], node, pre_actions);
		const std::string second = convertParameter(parameters[1], node, pre_actions);
		return "set " + first + " = " + second;
	}
	case "AddTriggerEvent"s_hash:
	{
		
		Action* event_action = parameters[1]->funcParam;
		std::string trg = convertParameter(parameters[0], node, pre_actions);
		std::string event_name = event_action->name;

		std::string events = "\tcall " + event_name + "(" + trg;

		ActionNodePtr temp(new ActionNode(event_action, node));
		for (size_t k = 0; k < event_action->param_count; k++)
		{
			Parameter* param = event_action->parameters[k];


			std::string type = param->type_name;

			events += ", ";
			events += convertParameter(param, temp, pre_actions);
		}
		events += ")\n";

		return events;
	}

	case "DzTriggerRegisterMouseEventMultiple"s_hash:
	case "DzTriggerRegisterKeyEventMultiple"s_hash:
	case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
	case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
	case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		flag = true;
	case "DzFrameSetUpdateCallbackMultiple"s_hash:
	case "DzFrameSetScriptMultiple"s_hash:
	{
		const std::string function_name = generate_function_name(node->getTriggerNamePtr());

		std::string name = node->getName();


		output += "if GetLocalPlayer() == ";
		output += convertParameter(parameters[0], node, pre_actions);
		output += " then\n";
		output += spaces[space_stack + 1];
		output += "call " + name.substr(0, name.length() - 8) + "ByCode(";
		if (flag)
		{
			output += "null,";
		}
		for (size_t k = 1; k < action->param_count; k++)
		{
			Parameter* param = action->parameters[k];
			if (strcmp(param->type_name, "code") != 0)
			{
				output += convertParameter(param, node, pre_actions);
				output += ",";
			}
		}
		if (flag)
		{
			output += "false,";
		}

		output += " function " + function_name;
		
		if (node->getNameId() == "DzFrameSetScriptMultiple"s_hash)
		{
			output += ",false";
		}

		output += ")\n";
		output += spaces[space_stack];
		output += "endif";

		std::string toto;

		int stack = space_stack;
		space_stack = 1;

		node->getChildNodeList(list);

		for (auto& child : list)
		{
			if (child->getActionId() == 1)
			{
				toto += spaces[space_stack];
				toto += convertAction(child, pre_actions, false) + "\n";
			}
		
		}

		pre_actions += "function " + function_name + " takes nothing returns nothing\n";
		if (m_ydweTrigger->isEnable())
		{
			m_ydweTrigger->onActionsToFuncBegin(pre_actions, node);
			pre_actions += toto;
			m_ydweTrigger->onActionsToFuncEnd(pre_actions, node);
		}
		else
		{
			pre_actions += toto;
		}

		pre_actions += "\nendfunction\n";
		space_stack = stack;
		return output;
	}

	}

	if (m_ydweTrigger->isEnable())
	{
		std::string actions;
		if (m_ydweTrigger->onActionToJass(actions, node, pre_actions, nested))
		{
			return actions;
		}
	}

	return convertCall(node, pre_actions, !nested);
}

std::string TriggerEditor::convertParameter(Parameter* parameter, ActionNodePtr node, std::string& pre_actions, bool add_call) 
{
	if (parameter == NULL)
	{
		return "";
	}
	if (m_ydweTrigger->isEnable())
	{
		std::string output;

		if (m_ydweTrigger->onParamterToJass(parameter, node, output, pre_actions, add_call))
		{
			return output;
		}
	}
	

	if (parameter->funcParam) 
	{
		ActionNodePtr childNode(new ActionNode(parameter->funcParam, parameter, node));
		return convertCall(childNode,pre_actions, add_call);
	}
	else {
	
		std::string value = std::string(parameter->value);

		switch (parameter->typeId)
		{
		case Parameter::Type::invalid:
	
			return "";
		case Parameter::Type::preset: 
		{
			auto world = WorldEditor::getInstance();
			const std::string preset_type = world->getConfigData("TriggerParams", value, 1);

			if (getBaseType(preset_type) == "string") {
				return string_replaced(world->getConfigData("TriggerParams",value, 2), "`", "\"");
			}
			return world->getConfigData("TriggerParams", value, 2);
		}
		case Parameter::Type::function:
			return value + "()";
		case Parameter::Type::variable:
		{
			
			std::string output = value;

			if (!output._Starts_with("gg_")) 
			{
				output = "udg_" + output;
			}

			if (value == "Armagedontimerwindow") 
			{
				puts("s");
			}
			if (parameter->arrayParam) {
				output += "[" + convertParameter(&parameter->arrayParam[0], node, pre_actions) + "]";
			}
			return output;
		}
		case Parameter::Type::string:
		{
			
			uint32_t is_import_path = 0;
			std::string type = parameter->type_name;

			auto it = m_typesTable.find(type);
			if (it != m_typesTable.end())
				is_import_path = it->second->is_import_path;;

			if (is_import_path || getBaseType(type) == "string") {
				return "\"" + string_replaced(value, "\\", "\\\\") + "\"";
			}
			else if (type == "abilcode" || // ToDo this seems like a hack?
				type == "buffcode" ||
				type == "destructablecode" ||
				type == "itemcode" ||
				type == "ordercode" ||
				type == "techcode" ||
				type == "unitcode") {
				return "'" + value + "'";
			}
			else 
			{
				return value;
			}
		}
			
		}
	}
	assert(false);
	return "";
}



std::string TriggerEditor::convertCall(ActionNodePtr node, std::string& pre_actions, bool add_call)
{
	std::string output;

	Action* action = node->getAction();
	Parameter** parameters = action->parameters;

	switch (hash_(action->name))
	{

	case "CommentString"s_hash:
	{
		return "//" + convertParameter(parameters[0], node, pre_actions);
	}

	case "CustomScriptCode"s_hash:
	{
		return parameters[0]->value;
	}

	case "GetTriggerName"s_hash:
	{
		return "\"" + *node->getTriggerNamePtr() + "\"";
	}
	case "OperatorString"s_hash:
	{
		output += "(" + convertParameter(parameters[0], node, pre_actions);
		output += " + ";
		output += convertParameter(parameters[1], node, pre_actions) + ")";
		return output;
	}

	case "ForLoopVar"s_hash:
	{
		//std::string variable = "udg_" + convertParameter(parameters[0], trigger_name, pre_actions, "integer");
		std::string variable = convertParameter(parameters[0], node, pre_actions);

		output += "set " + variable + " = ";
		output += convertParameter(parameters[1], node, pre_actions) + "\n";
		output += "loop\n";
		output += "exitwhen " + variable + " > " + convertParameter(parameters[2], node, pre_actions) + "\n";
		output += convertParameter(parameters[3], node, pre_actions, true) + "\n";
		output += "set " + variable + " = " + variable + " + 1\n";
		output += "endloop\n";
		return output;
	}

	case "IfThenElse"s_hash:
	{
		std::string thentext;
		std::string elsetext;
		

		output += "if (" + convertParameter(parameters[0], node, pre_actions) + ") then\n";
		ActionNodePtr child_then(new ActionNode(parameters[1]->funcParam, node));
		ActionNodePtr child_else(new ActionNode(parameters[2]->funcParam, node));

		output += spaces[space_stack + 1];
		output += convertAction(child_then, pre_actions, false) + "\n";
		output += spaces[space_stack];
		output += "else\n";
		output += spaces[space_stack + 1];
		output += convertAction(child_else, pre_actions, false) + "\n";
		output += spaces[space_stack];
		output += "endif";

		return output;
	}

	case "EnumItemsInRectBJ"s_hash:
	case "ForForce"s_hash:
	case "ForGroup"s_hash:
	{

		const std::string function_name = generate_function_name(node->getTriggerNamePtr());

		std::string name = node->getName();


		output += "call " + name + "(";

		uint32_t codeIndex = -1;

		for (size_t k = 0; k < action->param_count; k++)
		{
			Parameter* param = action->parameters[k];
			if (strcmp(param->type_name, "code") != 0)
			{
				output += convertParameter(param, node, pre_actions);
				output += ",";
			}
			else
			{
				codeIndex = k;
			}
		}
		if (codeIndex != -1)
		{
			output += " function " + function_name + ")\n";

			std::string tttt = convertParameter(parameters[codeIndex], node, pre_actions);

			pre_actions += "function " + function_name + " takes nothing returns nothing\n";
			pre_actions += "\tcall " + tttt + "\n";
			pre_actions += "endfunction\n\n";
		}
	
		return output;
	}

	case "GetBooleanAnd"s_hash:
	{

		std::string first_parameter = convertParameter(parameters[0], node, pre_actions);
		std::string second_parameter = convertParameter(parameters[1], node, pre_actions);

		return "(" + first_parameter + " and " + second_parameter + ")";
	}

	case "GetBooleanOr"s_hash:
	{
		std::string first_parameter = convertParameter(parameters[0], node, pre_actions);
		std::string second_parameter = convertParameter(parameters[1], node, pre_actions);

		return "(" + first_parameter + " or " + second_parameter + ")";
		
	}

	case "OperatorInt"s_hash:
	case"OperatorReal"s_hash:
	{
		output += "(" + convertParameter(parameters[0], node, pre_actions);
		output += " " + convertParameter(parameters[1], node, pre_actions) + " ";
		output += convertParameter(parameters[2], node, pre_actions) + ")";
		return output;
	}
	default:
		break;
	}

	if (node->getName().substr(0, 15) == "OperatorCompare") {
		output += convertParameter(parameters[0], node, pre_actions);
		output += " " + convertParameter(parameters[1], node, pre_actions) + " ";
		output += convertParameter(parameters[2], node, pre_actions);
		return output;
	}
	size_t size = action->param_count;

	for (size_t k = 0; k < size; k++) {
		Parameter* param = parameters[k];

		const std::string child_type = param->type_name;

		if (child_type == "boolexpr") {
			const std::string function_name = generate_function_name(node->getTriggerNamePtr());

			std::string tttt = convertParameter(param, node, pre_actions);

			pre_actions += "function " + function_name + " takes nothing returns boolean\n";
			pre_actions += "\treturn " + tttt + "\n";
			pre_actions += "endfunction\n\n";

			output += "function " + function_name;
		}
		else {
			output += convertParameter(param, node, pre_actions);
		}

		if (k < size - 1) {
			output += ", ";
		}
	}
	std::string name = "_" + node->getName() + "_ScriptName";

	std::string func_name = WorldEditor::getInstance()->getConfigData("TriggerActions", name, 0);

	if (func_name.length() == 0)
	{
		func_name = node->getName();
	}
	return (add_call ? "call " : "") + func_name + "(" + output + ")";
}


std::string TriggerEditor::getBaseType(const std::string& type) const
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

std::string TriggerEditor::generate_function_name(std::shared_ptr<std::string> trigger_name) const {
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return "Trig_" + *trigger_name + "_" + std::to_string(time & 0xFFFFFFFF);
}



bool TriggerEditor::onConvertTrigger(Trigger* trigger)
{
	if (trigger->is_custom_srcipt || trigger->is_comment)
		return false;
	WorldEditor* world = WorldEditor::getInstance();

	std::string script = convertTrigger(trigger);
	
	if (m_initTriggerTable.find(trigger) != m_initTriggerTable.end())
	{
		trigger->is_initialize = 1;
		m_initTriggerTable.erase(trigger);
	}

	trigger->is_custom_srcipt = 1;

	//写入字符串到触发器中
	this_call<int>(world->getAddress(0x005CB280), trigger, script.c_str(), script.size());

	if (trigger->line_count > 0)
	{
		//遍历销毁所有动作
		for (size_t i = 0; i < trigger->line_count; i++)
		{
			Action* action = trigger->actions[i];
			if (action)
			{
				action->table->destroy(action, 1);
			}
		}

		if (trigger->actions)
		{
			//销毁动作容器
			uint32_t addr = (uintptr_t)::GetProcAddress(::GetModuleHandleW(L"Storm.dll"), (const char*)403);
			std_call<int>(addr, trigger->actions, ".PAVCWETriggerFunction@@", -0x2, 0);
		}
		trigger->number = 0;
		trigger->line_count = 0;
		trigger->actions = 0;
	}
	return true;
}
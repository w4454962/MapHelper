#include "stdafx.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"
#include <algorithm>
#include <assert.h>
#include <iostream>

TriggerEditor::TriggerEditor()
	: m_editorData(NULL),
	m_version(7),
	is_ydwe(false)
{ }

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
	for (uint32_t i = 0; i < data->type_count; i++)
	{
		TriggerType* type_data = &data->array[i];
		std::string value = type_data->value;
		if (value.length() > 0)
		{
			m_typeDefaultValues[type_data->type] = value;
		}
		
	}
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
	out.write((const char*)&writer.buffer[0],writer.buffer.size());
	out.close();

	printf("wtg 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}

void TriggerEditor::writeCategoriy(BinaryWriter& writer)
{
	Categoriy** categories = m_editorData->categories;

	uint32_t count = m_editorData->categoriy_count;

	writer.write(count);

	for (uint32_t i = 0; i < count; i++)
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

	for(uint32_t i = 0; i < variables->globals_count ; i++)
	{
		VariableData* data = &variables->array[i];
		//名字非gg_开头的变量
		if (data && strncmp(data->name, "gg_", 3))
			variable_count++;
	}



	writer.write(variable_count);

	//将非gg_的变量数据写入
	for (uint32_t i = 0; i < variables->globals_count; i++)
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

	for (uint32_t i = 0; i < m_editorData->categoriy_count; i++)
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

	for (uint32_t i = 0; i < trigger->line_count; i++)
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
	for (uint32_t i = 0; i < count; i++)
	{

		Parameter* param = action->parameters[i];
		writeParameter(writer, param);
	}

	uint32_t child_count = action->child_count;
	writer.write(child_count);
	//如果是 动作组 则循环将子动作写入
	for (uint32_t i = 0; i < child_count; i++)
	{
		Action* child = action->child_actions[i];

		uint32_t child_type = child->table->getType(child);

		writer.write(child_type);

		writer.write(child->group_id);

		writeAction(writer, child);
	}
}

void TriggerEditor::writeParameter(BinaryWriter& writer, Parameter* param)
{
	uint32_t type = param->type;
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
	writer.write_c_string_view(jass);

	writer.write(data->trigger_count);

	for (uint32_t i = 0; i < m_editorData->categoriy_count; i++)
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
	out.write((const char*)&writer.buffer[0], writer.buffer.size());
	out.close();

	printf("wct 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}


void TriggerEditor::saveSctipt(const char* path)
{
	TriggerData* data = m_editorData;

	is_ydwe = false;

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

	if (is_ydwe)//这是ydwe的内容
	{
		writer.write_string("#include <YDTrigger/Import.h>\n");
		writer.write_string("#include <YDTrigger/YDTrigger.h>\n");
	}

	writer.write_string("globals\n");
	printf("开始写变量\n");
	for (uint32_t i = 0; i < data->variables->globals_count; i++)
	{
		VariableData* var = &data->variables->array[i];
		std::string name = var->name;
		std::string type = var->type;

		variableTable[name] = var;

		if (var->is_array)
		{
			writer.write_string("\t" + type + " array udg_" + name + "\n");
		}
		else
		{
			//非gg_开头的自定义变量
			if (strncmp(var->name, "gg_", 3))
				name = "udg_" + name;
			std::string value = var->value;
			if (value.length() == 0)
			{
				auto it = m_typeDefaultValues.find(type);
				if (it != m_typeDefaultValues.end())
					value = it->second;
				else 
					value = "null";;
			}
			writer.write_string("\t" + type + " " + name + " = " + value + "\n");
		}
	}

	//申明随机组的全局变量
	if (worldData->random_group_count > 0)
	{

		for (uint32_t i = 0; i < worldData->random_group_count; i++)
		{
			sprintf(buffer, "%03d", i);
			writer.write_string("\tinteger array gg_rg_" + std::string(buffer) + "\n");
		}
	}

	if (is_ydwe)
	{
		writer.write_string("#include <YDTrigger/Globals.h>\n");
		writer.write_string("endglobals\n");
		writer.write_string("#include <YDTrigger/Function.h>\n");
	}
	else
	{
		writer.write_string("endglobals\n");
	}

	//开始初始化全局变量
	printf("开始初始化变量\n");
	writer.write_string("function InitGlobals takes nothing returns nothing\n");
	writer.write_string("\tlocal integer i = 0\n");

	for (uint32_t i = 0; i < data->variables->globals_count; i++)
	{
		VariableData* var = &data->variables->array[i];
		std::string name = var->name;
		 
		if (var->is_array )
		{
			//获取默认值
			std::string type = var->type;

			std::string defaultValue;
			auto it = m_typeDefaultValues.find(type);
			if (it != m_typeDefaultValues.end())
				defaultValue = it->second;
			if (!var->is_init && defaultValue.empty()) 
				continue;
			writer.write_string("\tset i = 0\n");
			writer.write_string("\tloop\n");
			writer.write_string("\t\texitwhen(i > " + std::to_string(var->array_size) + ")\n");

			if (var->is_init) 
			{
				std::string value = var->value;
				if (type == "string" && value.empty()) 
					writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
				else 
					writer.write_string("\t\tset udg_" + name + "[i] = " + value + "\n");
			}
			else 
			{
				if (type == "string") 
					writer.write_string("\t\tset udg_" + name + "[i] = \"\"\n");
				else 
					writer.write_string("\t\tset udg_" + name + "[i] = " + defaultValue + "\n");
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


	printf("开始初始化随机组\n");
	writer.write_string("function InitRandomGroups takes nothing returns nothing\n");
	writer.write_string("\tlocal integer curset\n");

	uint32_t count = worldData->random_group_count;

	for (int i = 0; i < count; i++)
	{
		RandomGroupData* groupData = &worldData->random_groups[i];
		if (groupData->group_count == 0)
			continue;
		writer.write_string("\t// Group " + std::to_string(i) + " - " + groupData->name + "\n");
		writer.write_string("\tcall RandomDistReset()\n");
		for (int a = 0; a < groupData->group_count; a++)
		{
			RandomGroup* group = &groupData->groups[a];
			writer.write_string("\tcall RandomDistAddItem(" + std::to_string(a) + "," + std::to_string(group->rate) + ")\n");
		}
		writer.write_string("\tset curset=RandomDistChoose()\n");
		
		for (int a = 0; a < groupData->group_count; a++)
		{
			RandomGroup* group = &groupData->groups[a];
			std::string head = "\tif";
			if (a > 0) head = "\telseif";
			writer.write_string( head + " ( curset == " + std::to_string(a) + " ) then\n");

			for (int b = 0; b < groupData->param_count; b++)
			{
				const char* ptr = group->names[b];
				std::string value = "-1";
				if (*ptr) value = "'" + std::string(ptr, ptr + 0x4) + "'";
				
				sprintf(buffer, "\t\tset gg_rg_%03d[%i] = %s\n", i, b, value.c_str());
				writer.write_string(buffer);
			}
		}
		writer.write_string("\telse\n");
		for (int b = 0; b < groupData->param_count; b++)
		{
			sprintf(buffer, "\t\tset gg_rg_%03d[%i] = -1\n", i, b);
			writer.write_string(buffer);
		}
		writer.write_string("\tendif\n");
	}
	writer.write_string("endfunction\n\n");

	

	
	printf("加载物品列表 %i  \n", worldData->item_table_count);
	for (int i = 0; i < worldData->item_table_count; i++)
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

		for (int a = 0; a < itemTable->setting_count; a++)
		{
			ItemTableSetting* itemSetting = &itemTable->item_setting[a];

			writer.write_string("\t\tcall RandomDistReset()\n");
			for (int b = 0; b < itemSetting->info_count; b++)
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

	for (int i = 0; i < worldData->units->unit_count; i++)
	{
		Unit* unit = &worldData->units->array[i];
		uint32_t size = 0;
		for (int a = 0; a < unit->item_setting_count; a++)
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

			

			for (int a = 0; a < unit->item_setting_count; a++)
			{
				ItemTableSetting* itemSetting = &unit->item_setting[a];

				writer.write_string("\t\tcall RandomDistReset()\n");
				
				for (int b = 0; b < itemSetting->info_count; b++)
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

	for (int i = 0; i < worldData->doodas->unit_count; i++)
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

			

			for (int a = 0; a < unit->item_setting_count; a++)
			{
				ItemTableSetting* itemSetting = &unit->item_setting[a];

				writer.write_string("\t\tcall RandomDistReset()\n");
				
				for (int b = 0; b < itemSetting->info_count; b++)
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

	for (int i = 0; i < worldData->sounds->sound_count; i++)
	{
		Sound* sound = &worldData->sounds->array[i];

		std::string sound_name = sound->name;

		// Replace spaces by underscores
		std::replace(sound_name.begin(), sound_name.end(), ' ', '_');
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

	writer2.buffer.clear();

	for (int i = 0; i < worldData->doodas->unit_count; i++)
	{
		Unit* unit = &worldData->doodas->array[i];
		sprintf(buffer, "gg_dest_%.04s_%04d", unit->name, unit->index);

		std::string id = buffer;

		if (variableTable.find(id) == variableTable.end()) 
			continue;
		
		sprintf(buffer, "%.4s,%.1f,%.1f,%.1f,%.1f,%d", unit->name, unit->x, unit->y, unit->angle, unit->sacle_x, unit->variation);
		writer.write_string("\tset " + id + " = CreateDestructable('" + std::string(buffer) + ")\n");

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
	writer.write_vector(writer2.buffer);

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

	for (int i = 0; i < worldData->units->unit_count; i++)
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
					for (int a = 0; a < unit->random_item_count; a++)
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
				writer.write_string("\tset " + var_name + " = CreateItem(" + std::string(buffer) + ")\n");
			else
				writer.write_string("\tcall CreateItem(" + std::string(buffer) + ")\n");
			if (b)
				writer.write_string("\tendif\n");
		}
	}
	writer.write_string("endfunction\n");


	writer.write_string("function CreateUnits takes nothing returns nothing\n");

	writer.write_string("\tlocal unit u\n");
	writer.write_string("\tlocal integer unitID\n");
	writer.write_string("\tlocal trigger t\n");
	writer.write_string("\tlocal real life\n");


	for (int i = 0; i < worldData->units->unit_count; i++)
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
				range = 200.f;
			 else 
				range = unit->warning_range;
			writer.write_string("\tcall SetUnitAcquireRange(" + unit_reference + ", " + std::to_string(range) + ")\n");
		}

		for (int a = 0; a < unit->skill_count; a++)
		{
			UnitSkill* skill = &unit->skills[a];
			std::string skillId = std::string(skill->name, skill->name + 0x4);
			for (int b = 0; b < skill->level; b++) 
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
		
		for (int a = 0; a < unit->item_count; a++)
		{
			UnitItem* item = &unit->items[a];
			writer.write_string("\tcall UnitAddItemToSlotById(" + unit_reference + ", '" + std::string(item->name,item->name + 0x4) + "', " + std::to_string(item->slot_id) + ")\n");
		}

		std::string dropName;

		uint32_t size = 0;
		for (int a = 0; a < unit->item_setting_count; a++)
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

	for (int i = 0; i < worldData->regions->region_count; i++)
	{
		Region* region = worldData->regions->array[i];
		std::string region_name = std::string("gg_rct_") + region->name;

		replace_string(region_name.begin(), region_name.end());


		int left = region->left * 32 + region->info->minX;
		int right = region->right * 32 + region->info->minX;
		int top = region->top * 32 + region->info->minY;
		int bottom = region->bottom * 32 + region->info->minY;

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
			writer.write_string("\tcall call SetSoundPosition("  + std::string(buffer) + ")\n");

			sprintf(buffer, "%s,true,%i,%i", region->sound_name, width, height);
			writer.write_string("\tcall call RegisterStackedSound(" + std::string(buffer) + ")\n");
		}

	}
	
	writer.write_string("endfunction\n");



	writer.write_string("function CreateCameras takes nothing returns nothing\n");

	for (int i = 0; i < worldData->cameras->camera_count; i++)
	{
		Camera* camera = &worldData->cameras->array[i];
		std::string camera_name = "gg_cam_" + std::string(camera->name);
		
		replace_string(camera_name.begin(), camera_name.end());

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

	//写入全局jass
	std::string_view globals_jass = std::string_view(trigger_data->globals_jass_script, trigger_data->globals_jass_size - 1);

	writer.write_c_string_view(globals_jass);


	std::vector<std::string> initialization_triggers;

	writer.write_string("\n");

	for (uint32_t i = 0; i < m_editorData->categoriy_count; i++)
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
			if (trigger->custom_jass_size > 0) 
			{
				writer.write_c_string_view(std::string_view(trigger->custom_jass_script, trigger->custom_jass_size));
			}
			else 
			{
				writer.write_c_string_view(convert_gui_to_jass(trigger, initialization_triggers));
			}
		}
	}

	std::cout << std::string_view((const char*)&writer.buffer[0],writer.buffer.size());



}



std::string TriggerEditor::convert_gui_to_jass(Trigger* trigger, std::vector<std::string>& initializtions) 
{
	
	std::string trigger_name = std::string(trigger->name);

	replace_string(trigger_name.begin(), trigger_name.end());

	std::string trigger_variable_name = "gg_trg_" + trigger_name;
	std::string trigger_action_name = "Trig_" + trigger_name + "_Actions";


	std::string events = "";
	std::string conditions = "";
	std::string pre_actions = "";
	std::string actions = "";

	events += "function InitTrig_" + trigger_name + " takes nothing returns nothing\n";
	events += "\tset " + trigger_variable_name + " = CreateTrigger()\n";

	actions += "function " + trigger_action_name + " takes nothing returns nothing\n";

	for (int i = 0; i < trigger->line_count; i++)
	{
		Action* action = trigger->actions[i];
		if (action->enable) 
			continue;
		uint32_t type = action->table->getType(action);

		std::string name = action->name;

		switch (type) 
		{
		case Action::Type::event:
			
			if (name == "MapInitializationEvent") 
			{
				initializtions.push_back(trigger_variable_name);
				continue;
			}
			events += "\tcall " + name + "(" + trigger_variable_name + ", ";
			for (int k = 0; k < action->param_count; k++) 
			{
				Parameter* param = action->parameters[k];
				
				TriggerType* type_data = &m_configData->array[param->type];
				std::string type = type_data->type;

				events += resolve_parameter(param, trigger_name, pre_actions, type);

				if (k < action->param_count - 1) 
					events += ", ";
			}
			events += ")\n";

			break;
		case Action::Type::condition:
			//conditions += "\tif (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
			conditions += "\treturn false\n";
			conditions += "\tendif\n";
			break;
		case Action::Type::action:
			//actions += "\t" + convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
			break;
		}
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


std::string TriggerEditor::resolve_parameter(Parameter* parameter, const std::string& trigger_name, std::string& pre_actions, const std::string& type, bool add_call) const {
	if (parameter->funcParam) 
	{
		//return testt(trigger_name, parameter->funcParam->name, parameter->funcParam->parameters, pre_actions, add_call);
	}
	else {
	
		std::string value = std::string(parameter->value);

		switch (parameter->type) 
		{
		case Parameter::Type::invalid:
	
			return "";
		case Parameter::Type::preset: 
		{
			
			//const std::string preset_type = trigger_data.data("TriggerParams", parameter.value, 1);
			//
			//if (get_base_type(preset_type) == "string") {
			//	return string_replaced(trigger_data.data("TriggerParams", parameter.value, 2), "`", "\"");
			//}
			//
			//return trigger_data.data("TriggerParams", parameter.value, 2);
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
				output += "[" + resolve_parameter(&parameter->arrayParam[0], trigger_name, pre_actions, "integer") + "]";
			}
			return output;
		}
		case Parameter::Type::string:
		{

		}
			//std::string import_type = trigger_data.data("TriggerTypes", type, 5);
			//
			//if (not import_type.empty()) {
			//	return "\"" + string_replaced(parameter.value, "\\", "\\\\") + "\"";
			//}
			//else if (get_base_type(type) == "string") 
			//{
			//	return "\"" + value + "\"";
			//}
			//else if (type == "abilcode" || // ToDo this seems like a hack?
			//	type == "buffcode" ||
			//	type == "destructablecode" ||
			//	type == "itemcode" ||
			//	type == "ordercode" ||
			//	type == "techcode" ||
			//	type == "unitcode") {
			//	return "'" + value + "'";
			//}
			//else 
			//{
			//	return value;
			//}
		}
	}
	assert(false, "error");
	return "";
}
//
//
//
//std::string TriggerEditor::testt(const std::string& trigger_name, const std::string& parent_name, Parameter** parameters, std::string& pre_actions, bool add_call) const 
//{
//	std::string output;
//
//	if (parent_name == "CommentString") {
//		return "//" + resolve_parameter(parameters[0], trigger_name, pre_actions, "");
//	}
//
//	if (parent_name == "CustomScriptCode") {
//		return resolve_parameter(parameters[0], trigger_name, pre_actions, "");
//	}
//
//
//	if (parent_name.substr(0, 15) == "OperatorCompare") {
//		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		output += " " + resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + " ";
//		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2));
//		return output;
//	}
//
//	if (parent_name == "OperatorString") {
//		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		output += " + ";
//		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + ")";
//		return output;
//	}
//
//	if (parent_name == "ForLoopVar") {
//		//std::string variable = "udg_" + resolve_parameter(parameters[0], trigger_name, pre_actions, "integer");
//		std::string variable = resolve_parameter(parameters[0], trigger_name, pre_actions, "integer");
//
//		output += "set " + variable + " = ";
//		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + "\n";
//		output += "loop\n";
//		output += "exitwhen " + variable + " > " + resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2)) + "\n";
//		output += resolve_parameter(parameters[3], trigger_name, pre_actions, get_type(parent_name, 3), true) + "\n";
//		output += "set " + variable + " = " + variable + " + 1\n";
//		output += "endloop\n";
//		return output;
//	}
//
//	if (parent_name == "IfThenElse") {
//		std::string thentext;
//		std::string elsetext;
//
//		std::string function_name = generate_function_name(trigger_name);
//		std::string tttt = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//
//		output += "if (" + function_name + "()) then\n";
//		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1), true) + "\n";
//		output += "else\n";
//		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2), true) + "\n";
//		output += "endif";
//
//		pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//		pre_actions += "return " + tttt + "\n";
//		pre_actions += "endfunction\n";
//		return output;
//	}
//
//	if (parent_name == "ForForce" || parent_name == "ForGroup") {
//		std::string function_name = generate_function_name(trigger_name);
//
//		std::string tttt = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
//
//		output += parent_name + "(";
//		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		output += ", function " + function_name;
//		output += ")";
//
//		pre_actions += "function " + function_name + " takes nothing returns nothing\n";
//		pre_actions += "\tcall " + tttt + "\n";
//		pre_actions += "endfunction\n\n";
//		return (add_call ? "call " : "") + output;
//	}
//
//	if (parent_name == "GetBooleanAnd") {
//		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
//
//		std::string function_name = generate_function_name(trigger_name);
//		output += "GetBooleanAnd(" + function_name + "(), ";
//		pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//		pre_actions += "\t return ( " + first_parameter + ")\n";
//		pre_actions += "endfunction\n\n";
//
//		function_name = generate_function_name(trigger_name);
//		output += function_name + "())";
//		pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//		pre_actions += "\t return ( " + second_parameter + ")\n";
//		pre_actions += "endfunction\n\n";
//
//		return (add_call ? "call " : "") + output;
//	}
//
//	if (parent_name == "GetBooleanOr") {
//		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
//
//		std::string function_name = generate_function_name(trigger_name);
//		output += "GetBooleanOr(" + function_name + "(), ";
//		pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//		pre_actions += "\t return ( " + first_parameter + ")\n";
//		pre_actions += "endfunction\n\n";
//
//		function_name = generate_function_name(trigger_name);
//		output += function_name + "())";
//		pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//		pre_actions += "\t return ( " + second_parameter + ")\n";
//		pre_actions += "endfunction\n\n";
//
//		return (add_call ? "call " : "") + output;
//	}
//
//	if (parent_name == "OperatorInt" || parent_name == "OperatorReal") {
//		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
//		output += " " + resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + " ";
//		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2)) + ")";
//		return output;
//	}
//
//	for (int k = 0; k < parameters.size(); k++) {
//		const auto& i = parameters[k];
//
//		const std::string type = get_type(parent_name, k);
//
//		if (type == "boolexpr") {
//			const std::string function_name = generate_function_name(trigger_name);
//
//			std::string tttt = resolve_parameter(parameters[k], trigger_name, pre_actions, get_type(parent_name, k));
//
//			pre_actions += "function " + function_name + " takes nothing returns boolean\n";
//			pre_actions += "\treturn " + tttt + "\n";
//			pre_actions += "endfunction\n\n";
//
//			output += "function " + function_name;
//		}
//		else {
//			output += resolve_parameter(i, trigger_name, pre_actions, get_type(parent_name, k));
//		}
//
//		if (k < parameters.size() - 1) {
//			output += ", ";
//		}
//	}
//	return (add_call ? "call " : "") + parent_name + "(" + output + ")";
//}

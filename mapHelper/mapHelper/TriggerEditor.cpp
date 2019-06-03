#include "stdafx.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"


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

	writer.write(action->group_id);
	
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

		writer.write(child->child_flag);

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

	std::string jass(data->globals_jass_script, data->globals_jass_size);
	writer.write_c_string(jass);

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


	EditorData* worldData = WorldEditor::getInstance()->getEditorData();

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
		if (unit->item_setting_count > 0) 
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
		
		sprintf(buffer, "%.4s,%.1f,%.1f,%.1f,%.1f,%d", unit->name, unit->x, unit->y, unit->facing, unit->sacle_x, unit->variation);
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


	printf("脚本内容：\n%s\n", &writer.buffer[0]);


}
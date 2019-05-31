#include "stdafx.h"
#include "TriggerEditor.h"

//#define printf //

TriggerEditor::TriggerEditor()
	: m_editorData(NULL),
	m_version(7)
{

}

TriggerEditor::~TriggerEditor()
{

}

void TriggerEditor::loadTriggers(TriggerData* data)
{
	m_editorData = data;

}

void TriggerEditor::saveTriggers(const char* path)
{
	if (!m_editorData)
	{
		printf("缺少触发器数据\n");
		return;
	}
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
}

void TriggerEditor::writeCategoriy(BinaryWriter& writer)
{
	Categoriy** categories = m_editorData->categories;

	uint32_t count = m_editorData->categoriy_count;

	writer.write(count);

	for (int i = 0; i < count; i++)
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

	for(int i = 0; i < variables->globals_count ; i++)
	{
		VariableData* data = &variables->array[i];
		//名字非gg_开头的变量
		if (data && strncmp(data->name, "gg_", 3))
			variable_count++;
	}



	writer.write(variable_count);

	//将非gg_的变量数据写入
	for (int i = 0; i < variables->globals_count; i++)
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

	for (int i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (int n = 0; n < trigger_count; n++)
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

	for (int i = 0; i < trigger->line_count; i++)
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
	for (int i = 0; i < count; i++)
	{

		Parameter* param = action->parameters[i];
		writeParameter(writer, param);
	}

	uint32_t child_count = action->child_count;
	writer.write(child_count);
	//如果是 动作组 则循环将子动作写入
	for (int i = 0; i < child_count; i++)
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
	TriggerData* data = m_editorData;

	BinaryWriter writer;

	uint32_t version = 1;
	writer.write(version);

	writer.write_c_string(data->global_jass_comment);

	writer.write(data->globals_jass_size);

	std::string jass(data->globals_jass_script, data->globals_jass_size);
	writer.write_c_string(jass);

	writer.write(data->trigger_count);

	for (int i = 0; i < m_editorData->categoriy_count; i++)
	{
		Categoriy* categoriy = m_editorData->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (int n = 0; n < trigger_count; n++)
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
}
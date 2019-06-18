#include "stdafx.h"
#include "YDTrigger.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"


YDTrigger::YDTrigger()
	:m_bEnable(true),
	 m_funcStack(0)
{
	
}

YDTrigger::~YDTrigger()
= default;

YDTrigger* YDTrigger::getInstance()
{
	static YDTrigger instance;

	return &instance;
}

bool YDTrigger::isEnable() const
{
	return m_bEnable;
}

void YDTrigger::onGlobals(BinaryWriter& a_writer)
{
	a_writer.write_string("#include <YDTrigger/Import.h>\n");
	a_writer.write_string("#include <YDTrigger/ImportSaveLoadSystem.h>\n");
	a_writer.write_string("#include <YDTrigger/Hash.h>\n");
	a_writer.write_string("#include <YDTrigger/YDTrigger.h>\n");
}

void YDTrigger::onEndGlobals(BinaryWriter& a_writer)
{
	a_writer.write_string("#include <YDTrigger/Globals.h>\n");
	a_writer.write_string("endglobals\n");
	a_writer.write_string("#include <YDTrigger/Function.h>\n");
}

bool YDTrigger::onRegisterEvent(std::string& a_events,ActionNodePtr a_node)
{
	if (a_node->getParentNode()->isRootNode() && a_node->getNameId() == "YDWEDisableRegister"s_hash)
	{
		m_triggerHasDisable[a_node->getTrigger()] = true;
		return false;
	}
		

	m_hasAnyPlayer = false;
	//搜索事件中 所有子动作的 预设参数 是否有 任意玩家 
	std::function<bool(Parameter**,uint32_t)> seachAnyPlayer = [&](Parameter** al_params,uint32_t al_count)
	{
		for (size_t i = 0; i < al_count; i++)
		{
			Parameter* v_param = al_params[i];
			Parameter**  v_childs = nullptr;
			uint32_t v_child_count = 0;
			switch (v_param->typeId)
			{
			case Parameter::Type::preset:
				if (!strcmp(v_param->value, "PlayerALL"))
					return true;
				break;
			case Parameter::Type::variable:
				if (v_param->arrayParam)
				{
					v_child_count = 1;
					v_childs = &v_param->arrayParam;
				}
				break;
			case Parameter::Type::function:
				v_child_count = v_param->funcParam->param_count;
				v_childs = v_param->funcParam->parameters;
				break;
			}
			if (v_child_count > 0 && v_childs)
			{
				return seachAnyPlayer(v_childs, v_child_count);
			}
		}
		return false;
	};

	if (seachAnyPlayer(a_node->getAction()->parameters, a_node->getAction()->param_count))
	{
		a_events += "#define YDTRIGGER_COMMON_LOOP(n) ";
		m_hasAnyPlayer = true;
	}
	
	return true;
}

void YDTrigger::onRegisterEvent2(std::string& a_events,ActionNodePtr a_node)
{
	if (m_hasAnyPlayer)
	{
		a_events += "#define YDTRIGGER_COMMON_LOOP_LIMITS (0, 15)\n";
		a_events += "#include <YDTrigger/Common/loop.h>\n";
		m_hasAnyPlayer = false;
	}
}

bool YDTrigger::onActionToJass(std::string& a_output,ActionNodePtr a_node, std::string& a_pre_actions, bool a_nested)
{
	TriggerEditor* v_editor = TriggerEditor::getInstance();
	int& v_stack = v_editor->space_stack;

	std::vector<ActionNodePtr> v_list;
	Action* v_action = node->getAction();
	
	Parameter** v_parameters = action->parameters;

    switch (node->getNameId())
	{
	case "YDWEForLoopLocVarMultiple"s_hash:
	{
		std::string v_variable = std::string("ydul_") + action->parameters[0]->value;
		a_output += v_editor->spaces[v_stack];
		a_output += "set " + v_variable + " = ";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions) + "\n";
		a_output += v_editor->spaces[v_stack];
		a_output += "loop\n";
		a_output += v_editor->spaces[++v_stack];
		a_output += "exitwhen " + v_variable + " > " + v_editor->convertParameter(parameters[2], a_node, a_pre_actions) + "\n";
	
		a_node->getChildNodeList(v_list);
		for (auto& v_child : v_list)
		{
			a_output += v_editor->spaces[v_stack];
			//循环里的子动作 沿用外面相同的父节点
			a_output += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
		}

		a_output += v_editor->spaces[v_stack];
		a_output += "set " + v_variable + " = " + v_variable + " + 1\n";
		a_output += v_editor->spaces[--v_stack];
		a_output += "endloop\n";
		return true;
	}
	case "YDWEEnumUnitsInRangeMultiple"s_hash:
	{

		a_output += "set ydl_group = CreateGroup()\n";
		a_output += v_editor->spaces[v_stack];
		a_output += "call GroupEnumUnitsInRange(ydl_group,";
		a_output += v_editor->convertParameter(parameters[0], a_node, a_pre_actions);
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[2], a_node, a_pre_actions);
		a_output += ",null)\n";
		a_output += v_editor->spaces[v_stack];
		a_output += "loop\n";

		a_output += v_editor->spaces[++v_stack];
		a_output += "set ydl_unit = FirstOfGroup(ydl_group)\n";

		a_output += v_editor->spaces[v_stack];
		a_output += "exitwhen ydl_unit == null\n";

		a_output += v_editor->spaces[v_stack];
		a_output += "call GroupRemoveUnit(ydl_group, ydl_unit)\n";

		a_node->getChildNodeList(v_list);
		for (auto& v_child : v_list)
		{
			a_output += v_editor->spaces[v_stack];
			//循环里的子动作 沿用外面相同的父节点
			a_output += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
		}
		a_output += v_editor->spaces[--v_stack];
		a_output += "endloop\n";
		a_output += v_editor->spaces[v_stack];
		a_output += "call DestroyGroup(ydl_group)\n";

		return true;
	}
	case "YDWESaveAnyTypeDataByUserData"s_hash:
	{
		a_output += "call YDUserDataSet(";
		a_output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ",\"";
		a_output += parameters[2]->value;
		a_output += "\",";
		a_output += parameters[3]->value + 11;
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[4], a_node, a_pre_actions);
		a_output += ")\n";
		return true;
	}
	case "YDWEFlushAnyTypeDataByUserData"s_hash:
	{
		a_output += "call YDUserDataClear(";
		a_output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ",\"";
		a_output += parameters[3]->value;
		a_output += "\",";
		a_output += parameters[2]->value + 11;
		a_output += ")\n";

		return true;
	}
	case "YDWEFlushAllByUserData"s_hash:
	{
		a_output += "call YDUserDataClearTable(";
		a_output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ")\n";
		return true;
	}

	case "YDWEExecuteTriggerMultiple"s_hash:
	{
		a_output += "set ydl_trigger = ";
		a_output += v_editor->convertParameter(parameters[0], a_node, a_pre_actions);
		a_output += "\n" + v_editor->spaces[v_stack];
		a_output += "YDLocalExecuteTrigger(ydl_trigger)\n";
	
		a_node->getChildNodeList(v_list);
		for (auto& child : v_list)
		{
			a_output += v_editor->spaces[v_stack];
			a_output += v_editor->convertAction(child, a_pre_actions, false) + "\n";
		}
		a_output += v_editor->spaces[v_stack];
		a_output += "call YDTriggerExecuteTrigger(ydl_trigger,";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ")\n";
		return true;
		
	}
	
	case "YDWETimerStartMultiple"s_hash:
	{

		std::string v_param_text;
		std::string v_action_text;

		v_param_text += "set ydl_timer = ";
		v_param_text += v_editor->convertParameter(parameters[0], a_node, a_pre_actions) + "\n";


		std::map<std::string, std::string> v_hashVarTable;

		//当前这一层需要传参的变量表
		std::map<std::string, std::string> v_thisVarTable;

		
		//找到上一层函数的逆天局部变量表
		auto v_map_ptr = a_node->getLastVarTable();

		a_node->getChildNodeList(v_list);
		for (auto& v_child : v_list)
		{
			Action* childAction = v_child->getAction();
			if (v_child->getActionId() == 0)//如果是参数区
			{
				switch (v_child->getNameId())
				{
					//在逆天计时器参数中使用逆天局部变量
				case "YDWESetAnyTypeLocalArray"s_hash:
				case "YDWESetAnyTypeLocalVariable"s_hash:
				{
					std::string var_name = childAction->parameters[1]->value;
					std::string var_type = childAction->parameters[0]->value + 11;
					v_hashVarTable.emplace(var_name, var_type);
					break;
				}
				default: ;
				}
				v_param_text += v_editor->spaces[v_stack];
				v_param_text += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
			}
		}


		int v_tmp_s = v_stack;
		v_stack = 1;
		
		for (auto& v_child : v_list)
		{
			if (v_child->getActionId() != 0)//如果是动作区
			{
				Action* v_childAction = v_child->getAction();

				seachHashLocal(v_childAction->parameters, v_childAction->param_count, &v_thisVarTable);
				v_action_text += v_editor->spaces[v_stack];
				v_action_text += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
			}
		}

		v_stack = v_tmp_s;

		for (auto&[n, t] : v_hashVarTable)
		{	
			v_thisVarTable.erase(n);
			if (v_map_ptr)
			{
				v_map_ptr->erase(n);
			}
		}

		a_output += v_param_text;

		ActionNodePtr v_tmp_ptr = ActionNodePtr(new ActionNode(action, a_node));

		//如果当前这层有需要申请的变量
		if (v_map_ptr->size() > 0)
		{
			for (auto&[n, t] : *v_map_ptr)
			{
				a_output += v_editor->spaces[v_stack];
				a_output += setLocal(v_tmp_ptr, n, t, getLocal(a_node, n, t), true) + "\n";
			}
			v_map_ptr->clear();
		}
		
		//将这一层需要传参的变量 传递给上一层
		for (auto&[n, t] : v_thisVarTable)
		{
			a_output += v_editor->spaces[v_stack];
			a_output += setLocal(v_tmp_ptr, n, t, getLocal(a_node, n, t),true) + "\n";

			v_map_ptr->emplace(n, t);
		}

		std::string v_func_name = v_editor->generate_function_name(a_node->getTriggerNamePtr());
		a_pre_actions += "function " + v_func_name + " takes nothing returns nothing\n";


		onActionsToFuncBegin(a_pre_actions,a_node);
		a_pre_actions += v_action_text;
		onActionsToFuncEnd(a_pre_actions, a_node);
		a_pre_actions += "endfunction\n";

		a_output += v_editor->spaces[v_stack];
		a_output += "call TimerStart(ydl_timer,";
		a_output += v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
		a_output += ",";
		a_output += v_editor->convertParameter(parameters[2], a_node, a_pre_actions);
		a_output += ",function ";
		a_output += v_func_name;
		a_output += ")";

		return true;
		
	}


	case "YDWERegisterTriggerMultiple"s_hash:
	{
		std::string v_param_text;
		std::string v_action_text;

		v_param_text += "set ydl_trigger = ";
		v_param_text += v_editor->convertParameter(parameters[0], a_node, a_pre_actions) + "\n";


		std::map<std::string, std::string> v_hashVarTable;

		//当前这一层需要传参的变量表
		std::map<std::string, std::string> v_this_var_table;

		//找到上一层函数的逆天局部变量表
		auto v_map_ptr = a_node->getLastVarTable();

		a_node->getChildNodeList(v_list);

		for (auto& v_child : v_list)
		{
			Action* childAction = v_child->getAction();

			//如果是事件 则单独处理
			if (v_child->getActionType() == Action::Type::event)
			{
				if (v_child->getNameId() == "MapInitializationEvent"s_hash)
				{
					continue;
				}
				onRegisterEvent(v_param_text,v_child);
				v_param_text += v_editor->spaces[v_stack];
				
				v_param_text += "call " + v_child->getName() + "(ydl_trigger";

				for (size_t k = 0; k < childAction->param_count; k++)
				{
					v_param_text += ", ";
					v_param_text += v_editor->convertParameter(childAction->parameters[k], v_child, a_pre_actions);
				}
				v_param_text += ")\n";
				onRegisterEvent2(v_param_text, v_child);
			}
			else if (v_child->getActionId() == 1)//如果是参数区
			{
				switch (v_child->getNameId())
				{
					//在逆天计时器参数中使用逆天局部变量
				case "YDWESetAnyTypeLocalArray"s_hash:
				case "YDWESetAnyTypeLocalVariable"s_hash:
				{
					std::string var_name = childAction->parameters[1]->value;
					std::string var_type = childAction->parameters[0]->value + 11;
					v_hashVarTable.emplace(var_name, var_type);
					break;
				}
				}
				v_param_text += v_editor->spaces[v_stack];
				v_param_text += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
			}
		}

		int v_tmp_stack = v_stack;
		v_stack = 1;

		for (auto& v_child : v_list)
		{
			Action* v_child_action = v_child->getAction();

			if (v_child->getActionId() == 2)//如果是动作区
			{
				seachHashLocal(v_child_action->parameters, v_child_action->param_count, &v_this_var_table);
				v_action_text += v_editor->spaces[v_stack];
				v_action_text += v_editor->convertAction(v_child, a_pre_actions, false) + "\n";
			}
		}

		v_stack = v_tmp_stack;

		for (auto&[n, t] : v_hashVarTable)
		{
			v_this_var_table.erase(n);
			if (v_map_ptr)
			{
				v_map_ptr->erase(n);
			}
		}

		a_output += v_param_text;

		ActionNodePtr v_tmp_node_ptr = ActionNodePtr(new ActionNode(action, a_node));

		//如果当前这层有需要申请的变量
		if (v_map_ptr->size() > 0)
		{
			for (auto&[n, t] : *v_map_ptr)
			{
				a_output += v_editor->spaces[v_stack];
				a_output += setLocal(v_tmp_node_ptr, n, t, getLocal(a_node, n, t), true) + "\n";
			}
		}

		//将这一层需要传参的变量 传递给上一层
		for (auto&[n, t] : v_this_var_table)
		{
			a_output += v_editor->spaces[v_stack];
			a_output += setLocal(v_tmp_node_ptr, n, t, getLocal(a_node, n, t), true) + "\n";

			v_map_ptr->emplace(n, t);
		}



		std::string v_func_name = v_editor->generate_function_name(a_node->getTriggerNamePtr());
		a_pre_actions += "function " + v_func_name + " takes nothing returns nothing\n";
	
		onActionsToFuncBegin(a_pre_actions, a_node);
		a_pre_actions += v_action_text;
		onActionsToFuncEnd(a_pre_actions, a_node);
		a_pre_actions += "endfunction\n";

		a_output += v_editor->spaces[v_stack];
		a_output += "call TriggerAddCondition(ydl_trigger,Condition(function ";
		a_output += v_func_name;
		a_output += "))";

		return true;
	}

	case "YDWESetAnyTypeLocalVariable"s_hash:
	{
	
		std::string v_var_name = parameters[1]->value;
		std::string v_var_type= parameters[0]->value + 11;

		std::string v_var_value = v_editor->convertParameter(parameters[2], a_node, a_pre_actions);
	
		a_output +=setLocal(a_node,v_var_name, v_var_type, v_var_value);
		return true;
	}
	case "YDWESetAnyTypeLocalArray"s_hash:
	{

		std::string v_var_name = parameters[1]->value;
		std::string v_var_type = parameters[0]->value + 11;

		std::string v_index = v_editor->convertParameter(parameters[2], a_node, a_pre_actions);
		std::string v_var_value = v_editor->convertParameter(parameters[3], a_node, a_pre_actions);

		a_output += setLocalArray(a_node, v_var_name, v_var_type,v_index, v_var_value);
		return true;
	}



	case "YDWETimerStartFlush"s_hash:
	{
		ActionNodePtr v_action_node_ptr = a_node->getParentNode();
		bool v_is_in_timer = false;
		while (v_action_node_ptr.get())
		{
			if (v_action_node_ptr->getNameId() == "YDWETimerStartMultiple"s_hash)
			{
				v_is_in_timer = true;
				break;
			}
			v_action_node_ptr = v_action_node_ptr->getParentNode();
		}
		if (v_is_in_timer)
		{
			a_output += "call YDLocal3Release()\n";
			a_output += v_editor->spaces[v_stack];
			a_output += "call DestroyTimer(GetExpiredTimer())\n";
			return true;
		}
		else
		{
			a_output += "不要在逆天计时器的动作外使用<清除逆天计时器>";
			return true;
		}
	}
	case "YDWERegisterTriggerFlush"s_hash:
	{
		ActionNodePtr v_parent_node_ptr = a_node->getParentNode();

		ActionNodePtr v_node_ptr = a_node->getParentNode();
		bool v_is_in_trigger = false;
		while (v_node_ptr.get())
		{
			if (v_node_ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash)
			{
				v_is_in_trigger = true;
				break;
			}
			v_node_ptr = v_node_ptr->getParentNode();
		}
		if (v_is_in_trigger)
		{
			a_output += "call YDLocal4Release()\n";
			a_output += v_editor->spaces[v_stack];
			a_output += "call DestroyTrigger(GetTriggeringTrigger())\n";
			return true;
		}
		else
		{
			a_output += "不要在逆天触发器的动作外使用<清除逆天触发器>";
			return true;
		}
	}
	
	case "TriggerSleepAction"s_hash:
	case "PolledWait"s_hash:
	{

		
		a_output += v_editor->convertCall(a_node, a_pre_actions, !a_nested) + "\n";


		ActionNodePtr v_branch_node_ptr = a_node->getBranchNode();

		ActionNodePtr v_parent_node_ptr = v_branch_node_ptr->getParentNode();
		if (v_parent_node_ptr.get() && (
			v_parent_node_ptr->getNameId() == "YDWETimerStartMultiple"s_hash ||
			v_parent_node_ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash
			))
		{
			a_output += "不要在逆天计时器/逆天触发器内使用等待";
		}
		else
		{
			a_output += v_editor->spaces[v_stack];
			a_output += "call YDLocalReset()\n";
		}
			
		return true;
	}
	case "ReturnAction"s_hash:
	{
		a_output += editor->spaces[v_stack];
		onActionsToFuncEnd(a_output, a_node);
		a_output += "return\n";
		return true;
	}

	case "YDWEExitLoop"s_hash:
	{
		a_output += "exitwhen true\n";
		return true;
	}
	case "CustomScriptCode"s_hash:
	case "YDWECustomScriptCode"s_hash:
	{
		a_output += parameters[0]->value;
		return true;
	}
	case "YDWEActivateTrigger"s_hash:
	{
		a_output += "";
		Parameter* v_param = parameters[0];
		if (v_param->typeId == Parameter::Type::variable && v_param->arrayParam == NULL)
		{
			const char* ptr = v_param->value;
			if (ptr && strncmp(ptr, "gg_trg_", 7) == 0)
				ptr = ptr + 7;
			
			std::string v_func_name = std::string("InitTrig_") + ptr;
			convert_name(v_func_name);

			std::string v_ret = v_editor->convertParameter(parameters[1], a_node, a_pre_actions);
			if (v_ret.compare("true") == 0)
			{
				a_output += "call ExecuteFunc(\"" + v_func_name + "\")\n";
			}
			else
			{
				a_output += "call " + v_func_name + "()\n";
			}
		}
		return true;
	}
	}

	return false;
}

bool YDTrigger::onParamterToJass(Parameter* paramter, ActionNodePtr node, std::string& output, std::string& pre_actions, bool nested)
{
	TriggerEditor* editor = TriggerEditor::getInstance();

	switch (paramter->typeId)
	{
	case Parameter::Type::function:
	{
		Action* action = paramter->funcParam;
		if (!action) break;
		Parameter** parameters = action->parameters;
		switch (hash_(action->name))
		{
		case "YDWELoadAnyTypeDataByUserData"s_hash:
		{
			output += "YDUserDataGet(";
			output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
			output += ",";
			output += editor->convertParameter(parameters[1], node, pre_actions);
			output += ",\"";
			output += parameters[2]->value;
			output += "\",";
			output += paramter->type_name;
			output += ")";
			return true;
		}
		case "YDWEHaveSavedAnyTypeDataByUserData"s_hash:
		{
			output += "YDUserDataHas(";
			output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
			output += ",";
			output += editor->convertParameter(parameters[1], node, pre_actions);
			output += ",\"";
			output += parameters[3]->value;
			output += "\",";
			output += parameters[2]->value + 11;
			output += ")";
			return true;
		}
		case "GetEnumUnit"s_hash:
		{
			if (m_isInYdweEnumUnit)
			{
				output += "ydl_unit";
			}
			else
			{
				output += "GetEnumUnit()";
			}
			return true;
		}
		case "GetFilterUnit"s_hash:
		{
			if (m_isInYdweEnumUnit)
			{
				output += "ydl_unit";
			}
			else
			{
				output += "GetFilterUnit()";
			}
			return true;
		}
		case "YDWEForLoopLocVarIndex"s_hash:
		{
			std::string varname = action->parameters[0]->value;
			output = "ydul_" + varname;
			return true;
		}
		case "YDWEGetAnyTypeLocalVariable"s_hash:
		{
			std::string var_name = parameters[0]->value;
			std::string var_type = paramter->type_name;

			output += getLocal(node, var_name, var_type);
			return true;
		}
		case "YDWEGetAnyTypeLocalArray"s_hash:
		{
			std::string var_name = parameters[0]->value;
			std::string var_type = paramter->type_name;
			std::string index = editor->convertParameter(parameters[1], node, pre_actions);
	
			output += getLocalArray(node, var_name, var_type,index);
			return true;
		}
		case "CustomScriptCode"s_hash:
		case "YDWECustomScriptCode"s_hash:
		{
			output += parameters[0]->value;
			return true;
		}
		}
	}

	}
	return false;
}

bool YDTrigger::seachHashLocal(Parameter** parameters, uint32_t count, std::map<std::string, std::string>* mapPtr)
{

	for (size_t i = 0; i < count; i++)
	{
		Parameter* param = parameters[i];

		Parameter**  childs = NULL;
		uint32_t child_count = 0;
		switch (param->typeId)
		{
		case Parameter::Type::variable:
			if (param->arrayParam)
			{
				child_count = 1;
				childs = &param->arrayParam;
			}
			break;
		case Parameter::Type::function:
		{
			Action* action = param->funcParam;
			switch (hash_(action->name))
			{
			case "YDWEGetAnyTypeLocalVariable"s_hash:
			case "YDWEGetAnyTypeLocalArray"s_hash:
			{
				if (mapPtr)
				{
					std::string var_name = action->parameters[0]->value;
					std::string var_type = param->type_name;
					if (mapPtr->find(var_type) == mapPtr->end())
						mapPtr->emplace(var_name, var_type);
				}
				else
				{
					return true; //搜索到了就退出递归
				}
				break;
			}
			case "YDWESetAnyTypeLocalVariable"s_hash:
			case "YDWESetAnyTypeLocalArray"s_hash:
			{
				if (!mapPtr)
				{
					return true;//搜索到了就退出递归
				}
			}
			}
			child_count = action->param_count;
			childs = param->funcParam->parameters;
			break;
		}
		}
		if (child_count > 0 && childs)
		{
			return seachHashLocal(childs, child_count,mapPtr);
		}
	}
	if (mapPtr && mapPtr->size() > 0)
	{
		return true;
	}
	return false;
}
void YDTrigger::onActionsToFuncBegin(std::string& funcCode, ActionNodePtr node)
{
	bool isInMainProc = false;

	bool isSeachHashLocal = node->isRootNode();

	auto localTable = node->getLocalTable();

	std::function<void(std::string, std::string)> addLocalVar = [&](std::string name, std::string type)
	{
		if (localTable->find(name) == localTable->end())
		{
			localTable->emplace(name, type);
		}
	};

	//搜索需要注册的局部变量
	std::function<void(Action**, uint32_t,Action*,bool,bool)> seachLocal = [&](Action** actions, uint32_t count,Action* parent_action,bool isSeachChild,bool isTimer)
	{
		for (size_t i = 0; i < count; i++)
		{
			Action* action = actions[i];
			//如果不搜索子动作 and action是子动作则跳过
		if (!isSeachChild && action->child_flag != -1 )
				continue;
			
			uint32_t hash = hash_(action->name);
			if (isSeachHashLocal)
			{
				if (!isTimer)
				{
					//如果不在计时器里 则搜索参数
					if (hash == "YDWETimerStartMultiple"s_hash)
						isTimer = true;
				}
		
				//搜索参数中是否有引用到逆天局部变量
				uint32_t count = action->param_count;

				isInMainProc = isInMainProc || seachHashLocal(action->parameters,count );
		
			}

#define next(b) seachLocal(action->child_actions, action->child_count,action,b,isTimer);
			switch (hash)
			{

			case "IfThenElse"s_hash:
			{
				seachLocal(&action->parameters[1]->funcParam, 1, action, true, isTimer);
				seachLocal(&action->parameters[2]->funcParam, 1, action, true, isTimer);
				break;
			}
			case "IfThenElseMultiple"s_hash:
			case "ForLoopAMultiple"s_hash:
			case "ForLoopBMultiple"s_hash:
			case "ForLoopVarMultiple"s_hash:
			case "YDWERegionMultiple"s_hash:
			{
				next(true);
				break;
			}
			case "YDWEForLoopLocVarMultiple"s_hash:
			{
				std::string varname = action->parameters[0]->value;
				addLocalVar("ydul_" + varname, "integer");
				next(true);
				break;
			}
			case "EnumDestructablesInRectAllMultiple"s_hash:
			case "EnumDestructablesInCircleBJMultiple"s_hash:
			case "EnumItemsInRectBJMultiple"s_hash:
			{
				next(true);
				break;
			}
			case "ForGroupMultiple"s_hash:
			case "YDWEEnumUnitsInRangeMultiple"s_hash:
			{
				addLocalVar("ydl_group", "group");
				addLocalVar("ydl_unit", "unit");
				next(true);
				break;
			}

			case "YDWETimerStartMultiple"s_hash:
			{
				addLocalVar("ydl_timer", "timer");
				next(false);
				break;
			}
			case "YDWERegisterTriggerMultiple"s_hash:
			{
				addLocalVar("ydl_trigger", "trigger");
				break;
			}
			case "YDWEExecuteTriggerMultiple"s_hash:
			{
				addLocalVar("ydl_triggerstep", "integer");
				addLocalVar("ydl_trigger", "trigger");
				next(true);
				break;
			}

			case "YDWESaveAnyTypeDataByUserData"s_hash:
			case "YDWESetAnyTypeLocalVariable"s_hash:
			case "YDWESetAnyTypeLocalArray"s_hash:
			{
				isInMainProc = true;
				break;
			}

			}
#undef next(b)
		}

	};

	if (node->isRootNode())
	{
		Trigger* trigger = node->getTrigger();

		seachLocal(trigger->actions, trigger->line_count,NULL,true,false);
	}
	else
	{
		Action* action = node->getAction();

		seachLocal(action->child_actions,action->child_count,action,true,false);
	}

	for (auto&[name, type] : *localTable)
	{
		funcCode += "\tlocal " + type+ " " + name + "\n";
	}

	if (node->isRootNode() && isInMainProc)
	{
		funcCode += "\tYDLocalInitialize()\n";
	}
	node->m_haveHashLocal = isInMainProc;
	
}

void YDTrigger::onActionsToFuncEnd(std::string& funcCode, ActionNodePtr node)
{

	if (node->isRootNode() && node->m_haveHashLocal)
	{
		funcCode += "\tcall YDLocal1Release()\n";
	}
	auto localTable = node->getLocalTable();
	for (auto&[name,type] : *localTable)
	{
		switch (hash_(type.c_str()))
		{
		case "unit"s_hash:
		case "group"s_hash:
		case "timer"s_hash:
		case "trigger"s_hash:
		case "force"s_hash:
		{
			funcCode += "\tset " + name + " = null\n";
			break;
		}
		}
	}
}

bool YDTrigger::hasDisableRegister(Trigger* trigger)
{
	auto it = m_triggerHasDisable.find(trigger);
	return it != m_triggerHasDisable.end();
}



std::string YDTrigger::setLocal(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& value,bool add)
{

	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();
	//根据当前设置逆天局部变量的位置 来决定生成的代码

	std::string callname;
	std::string handle;

	if (parent.get() == NULL || parent->isRootNode())//如果是在触发中
	{
		callname = "YDLocal1Set";

	}
	else
	{
		switch (parent->getNameId())
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (branch->getActionId() == 0 || add)
			{
				handle = "ydl_timer";
			}
			else //否则是动作区
			{
				handle = "GetExpiredTimer()";
			}
			callname = "YDLocalSet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{
			if (branch->getActionId() == 0)
			{
				handle = "ydl_trigger";
			}
			else //否则是动作区
			{	
				handle = "GetTriggeringTrigger()";
			}
			callname = "YDLocalSet";
			break;
		}
		//如果是在 逆天运行动作传参里
		case "YDWEExecuteTriggerMultiple"s_hash:
		{
			callname = "YDLocal5Set";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			callname = "YDLocal2Set";
		}
		}
	}

	std::string output;

	output += "call " + callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += type + ",\"" + name + "\",";
	output += value;
	output += ")";

	

	return output;
}

std::string YDTrigger::getLocal(ActionNodePtr node, const std::string& name,const std::string& type)
{
	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();

	//根据当前设置逆天局部变量的位置 来决定生成的代码
	std::string callname;
	std::string handle;
	if (parent.get() == NULL || parent->isRootNode() || branch->isRootNode())//如果是在触发中
	{
		auto varTable = branch->getLastVarTable();
		if (varTable->find(name) != varTable->end())
		{
			callname = "YDLocal1Get";
		}
		else
		{
			callname = "YDLocal2Get";

		}
	}
	else
	{
		switch (parent->getNameId())
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (branch->getActionId() == 0) //0是参数区
			{
				return getLocal(parent, name, type);
			}
			else //否则是动作区
			{
				handle = "GetExpiredTimer()";
			}
			callname = "YDLocalGet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{
			if (branch->getActionId() < 2) //0是事件区 1是参数区
			{
				return getLocal(parent->getParentNode(), name, type);
			}
			else
			{//否则是动作区
				handle = "GetTriggeringTrigger()";
			}
			callname = "YDLocalGet";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			callname = "YDLocal2Get";
			break;
		}
		}
	}

	std::string output;

	output += callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += type + ",\"" + name + "\")";

	return output;
}


std::string YDTrigger::setLocalArray(ActionNodePtr node, const  std::string& name, const std::string& type, const  std::string& index, const  std::string& value)
{

	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();

	//根据当前设置逆天局部变量的位置 来决定生成的代码
	std::string callname;
	std::string handle;

	if (parent.get() == NULL || branch->isRootNode())//如果是在触发中
	{
		callname = "YDLocal1ArraySet";
	}
	else
	{
		switch (parent->getNameId())
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (branch->getNameId() == 0)
			{
				handle = "ydl_timer";
			}
			else //否则是动作区
			{
				handle = "GetExpiredTimer()";
			}
			callname = "YDLocalArraySet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{
			if (branch->getNameId() == 0)
			{
				handle = "ydl_trigger";
			}
			else //否则是动作区
			{
				handle = "GetTriggeringTrigger()";
			}

			callname = "YDLocalArraySet";
			break;
		}
		//如果是在 逆天运行动作传参里
		case "YDWEExecuteTriggerMultiple"s_hash:
		{
			callname = "YDLocal5ArraySet";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			callname = "YDLocal2ArraySet";
		}

		}
	}

	std::string output;

	output += "call " + callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += type;
	output += ",\"";
	output += name;
	output += "\",";
	output += index;
	output += ",";
	output += value;
	output += ")";


	return output;
}
std::string YDTrigger::getLocalArray(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& index)
{
	//根据当前设置逆天局部变量的位置 来决定生成的代码

	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();

	std::string callname;
	std::string handle;
	if (parent.get() == NULL || parent->isRootNode() || branch->isRootNode())//如果是在触发中
	{
		auto varTable = branch->getLastVarTable();
		if (varTable->find(name) != varTable->end())
		{
			callname = "YDLocal1ArrayGet";
		}
		else
		{
			callname = "YDLocal2ArrayGet";
		}
	}
	else
	{
		switch (parent->getNameId())
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (branch->getActionId() == 0) //0是参数区
			{
				return getLocal(parent, name, type);
			}
			else //否则是动作区
			{
				handle = "GetExpiredTimer()";
			}
			callname = "YDLocalArrayGet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{

			if (branch->getActionId() < 2) //0是事件区 1是参数区
			{
				return getLocal(parent->getParentNode(), name, type);
			}
			else
			{//否则是动作区
				handle = "GetTriggeringTrigger()";
			}
			callname = "YDLocalArrayGet";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{

			callname = "YDLocal2ArrayGet";
			break;
		}
		}
	}
	std::string output;

	output += callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += type;
	output += ",\"";
	output += name;
	output += "\",";
	output += index;
	output += ")";
	return output;
}
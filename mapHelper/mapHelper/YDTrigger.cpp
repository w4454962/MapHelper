#include "stdafx.h"
#include "YDTrigger.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"


YDTrigger::YDTrigger()
	:m_bEnable(true)
{
	
}

YDTrigger::~YDTrigger()
{

}

YDTrigger* YDTrigger::getInstance()
{
	static YDTrigger instance;

	return &instance;
}

bool YDTrigger::isEnable()
{
	return m_bEnable;
}

void YDTrigger::onGlobals(BinaryWriter& writer)
{
	writer.write_string("#include <YDTrigger/Import.h>\n");
	writer.write_string("#include <YDTrigger/YDTrigger.h>\n");
}

void YDTrigger::onEndGlobals(BinaryWriter& writer)
{
	writer.write_string("#include <YDTrigger/Globals.h>\n");
	writer.write_string("endglobals\n");
	writer.write_string("#include <YDTrigger/Function.h>\n");
}

bool YDTrigger::onRegisterEvent(std::string& events,Trigger* trigger,Action* action,std::string& name)
{
	if (name == "YDWEDisableRegister")
	{
		m_triggerHasDisable[trigger] = true;
		return false;
	}
		

	m_hasAnyPlayer = false;
	//搜索事件中 所有子动作的 预设参数 是否有 任意玩家 
	std::function<bool(Parameter**,uint32_t)> seachAnyPlayer = [&](Parameter** params,uint32_t count)
	{
		for (int i = 0; i < count; i++)
		{
			Parameter* param = params[i];
			Parameter**  childs = NULL;
			uint32_t child_count = 0;
			switch (param->typeId)
			{
			case Parameter::Type::preset:
				if (!strcmp(param->value, "PlayerALL"))
					return true;
				break;
			case Parameter::Type::variable:
				if (param->arrayParam)
				{
					child_count = 1;
					childs = &param->arrayParam;
				}
				break;
			case Parameter::Type::function:
				child_count = param->funcParam->param_count;
				childs = param->funcParam->parameters;
				break;
			}
			if (child_count > 0 && childs)
			{
				return seachAnyPlayer(childs, child_count);
			}
		}
		return false;
	};

	if (seachAnyPlayer(action->parameters, action->param_count))
	{
		events += "#define YDTRIGGER_COMMON_LOOP(n) ";
		m_hasAnyPlayer = true;
	}
	
	return true;
}

void YDTrigger::onRegisterEvent2(std::string& events, Trigger* trigger, Action* action, std::string& name)
{
	if (m_hasAnyPlayer)
	{
		events += "#define YDTRIGGER_COMMON_LOOP_LIMITS (0, 15)\n";
		events += "#include <YDTrigger/Common/loop.h>\n";
		m_hasAnyPlayer = false;
	}
}

bool YDTrigger::onActionToJass(std::string& output, Action* action,Action* parent, std::string& pre_actions, const std::string& trigger_name, bool nested)
{
	if (!action->enable)
		return false;
	TriggerEditor* editor = TriggerEditor::getInstance();
	int& stack = editor->space_stack;
	
	Parameter** parameters = action->parameters;
	switch (hash_(action->name))
	{
	case "YDWEForLoopLocVarMultiple"s_hash:
	{
		std::string variable = std::string("ydul_") + action->parameters[0]->value;
		output += "set " + variable + " = ";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions) + "\n";
		output += editor->spaces[stack];
		output += "loop\n";
		output += editor->spaces[++stack];
		output += "exitwhen " + variable + " > " + editor->resolve_parameter(parameters[2], parent, trigger_name, pre_actions) + "\n";
		for (uint32_t i = 0; i < action->child_count; i++)
		{
			Action* childAction = action->child_actions[i];
			output += editor->spaces[stack];
			output += editor->convert_action_to_jass(childAction, action, pre_actions, trigger_name, false) + "\n";
		}
		output += editor->spaces[stack];
		output += "set " + variable + " = " + variable + " + 1\n";
		output += editor->spaces[--stack];
		output += "endloop\n";
		return true;
	}
	case "YDWEEnumUnitsInRangeMultiple"s_hash:
	{
		if (m_isInYdweEnumUnit) break;
		m_isInYdweEnumUnit = true;

		output += "set ydl_group = CreateGroup()\n";
		output += editor->spaces[stack];
		output += "call GroupEnumUnitsInRange(ydl_group,";
		output += parameters[0]->value;
		output += ",";
		output += parameters[1]->value;
		output += ",";
		output += parameters[2]->value;
		output += ",null)\n";
		output += editor->spaces[stack];
		output += "loop\n";

		output += editor->spaces[++stack];
		output += "set ydl_unit = FirstOfGroup(ydl_group)\n";

		output += editor->spaces[stack];
		output += "exitwhen ydl_unit == null\n";

		output += editor->spaces[stack];
		output += "call GroupRemoveUnit(ydl_group, ydl_unit)\n";
		for (uint32_t i = 0; i < action->child_count; i++)
		{
			Action* childAction = action->child_actions[i];
			output += editor->spaces[stack];
			output += editor->convert_action_to_jass(childAction, action, pre_actions, trigger_name, false) + "\n";
		}
		output += editor->spaces[--stack];
		output += "endloop\n";
		output += editor->spaces[stack];
		output += "call DestroyGroup(ydl_group)\n";

		m_isInYdweEnumUnit = false;
		return true;
	}
	case "YDWESaveAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataSet(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
		output += ",\"";
		output += parameters[2]->value;
		output += "\",";
		output += parameters[3]->value + 11;
		output += ",";
		output += editor->resolve_parameter(parameters[4], parent, trigger_name, pre_actions);
		output += ")\n";
		return true;
	}
	case "YDWEFlushAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataClear(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
		output += ",\"";
		output += parameters[3]->value;
		output += "\",";
		output += parameters[2]->value + 11;
		output += ")\n";

		return true;
	}
	case "YDWEFlushAllByUserData"s_hash:
	{
		output += "call YDUserDataClearTable(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
		output += ")\n";
		return true;
	}

	case "YDWEExecuteTriggerMultiple"s_hash:
	{
		output += "set ydl_trigger = ";
		output += editor->resolve_parameter(parameters[0], parent, trigger_name, pre_actions);
		output += "\n" + editor->spaces[stack];
		output += "YDLocalExecuteTrigger(ydl_trigger)\n";

		for (int i = 0; i < action->child_count; i++)
		{
			Action* childAction = action->child_actions[i];

			output += editor->spaces[stack];
			output += editor->convert_action_to_jass(childAction, action, pre_actions, trigger_name, false) + "\n";
		}
		output += editor->spaces[stack];
		output += "call YDTriggerExecuteTrigger(ydl_trigger,";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
		output += ")\n";
		return true;
		
	}
	case "YDWETimerStartMultiple"s_hash:
	{

		std::string param_text;
		std::string action_text;

		param_text += "set ydl_timer = ";
		param_text += editor->resolve_parameter(parameters[0], parent, trigger_name, pre_actions) + "\n";

		std::map<std::string, std::string> hashVarTable;
		std::map<std::string, std::string> hashVarTable2;

		for (int i = 0; i < action->child_count; i++)
		{
			Action* childAction = action->child_actions[i];

			if (childAction->child_flag == 0)//如果是参数区
			{
				output += editor->spaces[stack];
				if (strcmp(childAction->name, "YDWESetAnyTypeLocalVariable") == 0)
				{
					std::string var_name = childAction->parameters[1]->value;
					std::string var_type = childAction->parameters[0]->value + 11;
					hashVarTable.emplace(var_name, var_type);
			
					//在参数区 值是用上一个父节点的
					std::string var_value = editor->resolve_parameter(childAction->parameters[2], parent, trigger_name, pre_actions);
					//设置局部变量则是新的节点里的
					param_text += editor->spaces[stack];
					param_text += setLocal(childAction, action, var_name, var_type, var_value) + "\n";
				}
				else
				{
					//用当前逆天计时器作为新的父节点来生成代码
					param_text += editor->convert_action_to_jass(childAction, action, pre_actions, trigger_name, false) + "\n";
				}
			}
		}


		int s = stack;
		stack = 1;


		for (int i = 0; i < action->child_count; i++)
		{
			Action* childAction = action->child_actions[i];

			if (childAction->child_flag != 0)//如果是动作区
			{

				seachHashLocal(childAction->parameters, childAction->param_count, &hashVarTable2);
				action_text += editor->spaces[stack];
				action_text += editor->convert_action_to_jass(childAction, action, pre_actions, trigger_name, false) + "\n";
			}
		}

		stack = s;

		for (auto&[n, t] : hashVarTable)
		{	
			hashVarTable2.erase(n);
		}


		output += param_text;

		for (auto&[n, t] : hashVarTable2)
		{
			output += editor->spaces[stack];
			output += setLocal(action, parent, n, t, getLocal(action, parent, n, t)) + "\n";
		}

		std::string func_name = editor->generate_function_name(trigger_name);
		pre_actions += "function " + func_name + " takes nothing returns nothing\n";
		onActionsToFuncBegin(pre_actions, NULL, action);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, NULL, action);
		pre_actions += "endfunction\n";

		output += editor->spaces[stack];
		output += "call TimerStart(ydl_timer,";
		output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
		output += ",";
		output += editor->resolve_parameter(parameters[2], parent, trigger_name, pre_actions);
		output += ",function ";
		output += func_name;
		output += ")";

		return true;
		
	}

	case "YDWESetAnyTypeLocalVariable"s_hash:
	{
		
		std::string var_name = parameters[1]->value;
		std::string var_type= parameters[0]->value + 11;
		
		std::string var_value = editor->resolve_parameter(parameters[2], parent, trigger_name, pre_actions);
		
		output +=setLocal(action,parent,var_name, var_type, var_value);
		return true;
	}

	}

	return false;
}

bool YDTrigger::onParamterToJass(std::string& output, Parameter* paramter, Action* parent, std::string& pre_actions, const std::string& trigger_name, bool nested)
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
			output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
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
			output += editor->resolve_parameter(parameters[1], parent, trigger_name, pre_actions);
			output += ",\"";
			output += parameters[3]->value;
			output += "\",";
			output += parameters[2]->value + 11;
			output += ")";
		}
		case "GetEnumUnit"s_hash:
		{
			if (m_isInYdweEnumUnit)
				output += "ydl_unit";
			else
				output += "GetEnumUnit()";
			return true;
		}
		case "GetFilterUnit"s_hash:
		{
			if (m_isInYdweEnumUnit)
				output += "ydl_unit";
			else
				output += "GetFilterUnit()";
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

			output += getLocal(action, parent, var_name, var_type);
			return true;
		}
		}
	}

	}
	return false;
}

bool YDTrigger::seachHashLocal(Parameter** parameters, uint32_t count, std::map<std::string, std::string>* mapPtr)
{

	for (int i = 0; i < count; i++)
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
			//case "YDWEGetAnyTypeLocalArray"s_hash:
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
void YDTrigger::onActionsToFuncBegin(std::string& funcCode,Trigger* trigger, Action* parent)
{
	if (!trigger && !parent)
		return;
	bool isSeachHashLocal = parent == NULL;

	


	//搜索需要注册的局部变量
	std::function<void(Action**, uint32_t,Action*,bool,bool)> seachLocal = [&](Action** actions, uint32_t count,Action* parent_action,bool isSeachChild,bool isTimer)
	{
		for (int i = 0; i < count; i++)
		{
			Action* action = actions[i];
			//如果不搜索子动作 and action是子动作则跳过
			//if (!isSeachChild && action->child_flag == 0)
			//	continue;
			
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
				m_isInMainProc = seachHashLocal(action->parameters, action->param_count);
			}

#define next(b) seachLocal(action->child_actions, action->child_count,action,b,isTimer);
			switch (hash)
			{
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
				next(false);
				break;
			}

			case "YDWETimerStartMultiple"s_hash:
			{
				addLocalVar("ydl_timer", "timer");
				next(false);
				break;
			}
			case "YDWEExecuteTriggerMultiple"s_hash:
			{
				addLocalVar("ydl_triggerstep", "integer");
				addLocalVar("ydl_trigger", "trigger");
				next(false);
				break;
			}
			case "YDWESetAnyTypeLocalVariable"s_hash:
			case "YDWESetAnyTypeLocalArray"s_hash:
			{
				//只搜索计时器中的条件区里的设置逆天局部变量
				if (isSeachHashLocal && (!isTimer || (isTimer && action->child_flag == 0)))
				{
					m_isInMainProc = true;
				}
				break;
			}

			}
#undef next(b)
		}

	};

	if (trigger)
	{
		seachLocal(trigger->actions, trigger->line_count,NULL,true,false);
	}
	else
	{
		seachLocal(parent->child_actions, parent->child_count,parent,true,false);
	}

	for (auto& i : m_localTable)
	{
		funcCode += "\tlocal " + i.type + " " + i.name;
		if (i.value.empty())
		{
			funcCode += "\n";
		}
		else
		{
			funcCode += " = " + i.value + "\n";
		}
	}

	if (m_isInMainProc)
	{
		funcCode += "\tYDLocalInitialize()\n";
	}
	
}

void YDTrigger::onActionsToFuncEnd(std::string& funcCode, Trigger* trigger, Action* parent)
{

	if (m_isInMainProc)
	{
		funcCode += "\tcall YDLocal1Release()\n";
	}
	for (auto& i : m_localTable)
	{
		switch (hash_(i.type.c_str()))
		{
		case "unit"s_hash:
		case "group"s_hash:
		case "timer"s_hash:
		case "trigger"s_hash:
		case "force"s_hash:
		{
			funcCode += "\tset " + i.name + " = null\n";
			break;
		}
		}
	}

	m_localTable.clear();
	m_localMap.clear();
	m_isInMainProc = false;

	m_HashLocalTable.clear();
}

bool YDTrigger::hasDisableRegister(Trigger* trigger)
{
	auto it = m_triggerHasDisable.find(trigger);
	return it != m_triggerHasDisable.end();
}

void YDTrigger::addLocalVar(std::string name, std::string type, std::string value)
{
	if (m_localMap.find(name) != m_localMap.end())
		return;

	m_localTable.push_back({ name,type,value });
	m_localMap[name] = true;
}

std::string YDTrigger::setLocal(Action* action, Action* parent, std::string name, std::string type,std::string value)
{
	//TriggerEditor* editor = TriggerEditor::getInstance();
	//std::string base_type = editor->get_base_type(type);
	//auto it = m_HashLocalTable.find(name);
	//if (it == m_HashLocalTable.end() && !type.empty())
	//{
	//	m_HashLocalTable[name] = base_type;
	//	return true;
	//}
	//return it->second.compare(base_type) == 0

	//根据当前设置逆天局部变量的位置 来决定生成的代码
	std::string callname;
	std::string handle;
	if (parent == NULL)//如果是在触发中
	{
		callname = "YDLocal1Set";
	}
	else
	{
		switch (hash_(parent->name))
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (action->child_flag == 0) 
				handle = "ydl_timer";
			else //否则是动作区
				handle = "GetExpiredTimer()";
			callname = "YDLocalSet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{
			if (action->child_flag == 0) 
				handle = "ydl_trigger";
			else //否则是动作区
				handle = "GetTriggeringTrigger()";

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
			if (m_isInMainProc)
				callname = "YDLocal1Set";
			else
				callname = "YDLocal2Set";
		}

		}
	}
	//std::string var_name = parameters[1]->value;
	//std::string var_type = parameters[0]->value + 11;
	//
	//if (!setHashLocal(var_name, var_type))
	//{
	//	auto it = m_HashLocalTable.find(var_name);
	//	output += "\n你使用了局部变量“" + var_name + "”(类型:" + var_type;
	//	output += ")，但你在其他地方使用的是 " + it->second + "类型)。\n";
	//}

	std::string output;

	output += "call " + callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += type + ",\"" + name + "\",";
	output += value;
	output += ")";

	HashVar var;
	var.name = name;
	var.type = type;
	var.action = action;
	var.parent = parent;
	
	m_HashLocalTable[name] = var;

	return output;
}

std::string YDTrigger::getLocal(Action* action, Action* parent, std::string name, std::string type)
{

	//根据当前设置逆天局部变量的位置 来决定生成的代码
	std::string callname;
	std::string handle;
	if (action == NULL || parent == NULL)//如果是在触发中
	{
		callname = "YDLocal1Get";
	}
	else
	{
		switch (hash_(parent->name))
		{
			//如果是在逆天计时器里
		case "YDWETimerStartMultiple"s_hash:
		{
			if (action->child_flag == 0) //0是参数区
				handle = "ydl_timer";
			else //否则是动作区
				handle = "GetExpiredTimer()";
			callname = "YDLocalGet";
			break;
		}
		//如果是在逆天触发器里
		case "YDWERegisterTriggerMultiple"s_hash:
		{
			if (action->child_flag == 0) //0是参数区
				handle = "ydl_trigger";
			else //否则是动作区
				handle = "GetTriggeringTrigger()";

			callname = "YDLocalGet";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			if (m_isInMainProc)
				callname = "YDLocal1Get";
			else
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
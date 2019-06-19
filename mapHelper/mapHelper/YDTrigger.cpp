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
	writer.write_string("#include <YDTrigger/ImportSaveLoadSystem.h>\n");
	writer.write_string("#include <YDTrigger/Hash.h>\n");
	writer.write_string("#include <YDTrigger/YDTrigger.h>\n");
}

void YDTrigger::onEndGlobals(BinaryWriter& writer)
{
	writer.write_string("#include <YDTrigger/Globals.h>\n");
	writer.write_string("endglobals\n");
	writer.write_string("#include <YDTrigger/Function.h>\n");
}

bool YDTrigger::onRegisterEvent(std::string& events,ActionNodePtr node)
{
	if (node->getParentNode()->isRootNode() && node->getNameId() == "YDWEDisableRegister"s_hash)
	{
		m_triggerHasDisable[node->getTrigger()] = true;
		return false;
	}
		

	m_hasAnyPlayer = false;
	//搜索事件中 所有子动作的 预设参数 是否有 任意玩家 
	std::function<bool(Parameter**,uint32_t)> seachAnyPlayer = [&](Parameter** params,uint32_t count)
	{
		for (size_t i = 0; i < count; i++)
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

	if (seachAnyPlayer(node->getAction()->parameters, node->getAction()->param_count))
	{
		events += "#define YDTRIGGER_COMMON_LOOP(n) ";
		m_hasAnyPlayer = true;
	}
	
	return true;
}

void YDTrigger::onRegisterEvent2(std::string& events,ActionNodePtr node)
{
	if (m_hasAnyPlayer)
	{
		events += "#define YDTRIGGER_COMMON_LOOP_LIMITS (0, 15)\n";
		events += "#include <YDTrigger/Common/loop.h>\n";
		m_hasAnyPlayer = false;
	}
}

void YDTrigger::onRegisterTrigger(std::string& output,const std::string& trigger_name, const std::string& var_name)
{
	output += "#ifdef DEBUG\n";
	output += "\tcall YDWESaveTriggerName(" + var_name + ",\"" + trigger_name + "\")\n";
	output += "#endif\n";
}


bool YDTrigger::onActionToJass(std::string& output,ActionNodePtr node, std::string& pre_actions, bool nested)
{
	TriggerEditor* editor = TriggerEditor::getInstance();
	int& stack = editor->space_stack;

	std::vector<ActionNodePtr> list;
	Action* action = node->getAction();
	
	Parameter** parameters = action->parameters;

    switch (node->getNameId())
	{
	case "YDWEForLoopLocVarMultiple"s_hash:
	{
		std::string variable = std::string("ydul_") + action->parameters[0]->value;
		output += editor->spaces[stack];
		output += "set " + variable + " = ";
		output += editor->convertParameter(parameters[1], node, pre_actions) + "\n";
		output += editor->spaces[stack];
		output += "loop\n";
		output += editor->spaces[++stack];
		output += "exitwhen " + variable + " > " + editor->convertParameter(parameters[2], node, pre_actions) + "\n";
	
		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor->spaces[stack];
			output += editor->convertAction(child, pre_actions, false) + "\n";
		}

		output += editor->spaces[stack];
		output += "set " + variable + " = " + variable + " + 1\n";
		output += editor->spaces[--stack];
		output += "endloop\n";
		return true;
	}
	case "YDWERegionMultiple"s_hash:
	{

		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor->spaces[stack];
			output += editor->convertAction(child, pre_actions, false) + "\n";
		}

		return true;
	}
	case "YDWEEnumUnitsInRangeMultiple"s_hash:
	{

		output += "set ydl_group = CreateGroup()\n";
		output += editor->spaces[stack];
		output += "call GroupEnumUnitsInRange(ydl_group,";
		output += editor->convertParameter(parameters[0], node, pre_actions);
		output += ",";
		output += editor->convertParameter(parameters[1], node, pre_actions);
		output += ",";
		output += editor->convertParameter(parameters[2], node, pre_actions);
		output += ",null)\n";
		output += editor->spaces[stack];
		output += "loop\n";

		output += editor->spaces[++stack];
		output += "set ydl_unit = FirstOfGroup(ydl_group)\n";

		output += editor->spaces[stack];
		output += "exitwhen ydl_unit == null\n";

		output += editor->spaces[stack];
		output += "call GroupRemoveUnit(ydl_group, ydl_unit)\n";

		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor->spaces[stack];
			//循环里的子动作 沿用外面相同的父节点
			output += editor->convertAction(child, pre_actions, false) + "\n";
		}
		output += editor->spaces[--stack];
		output += "endloop\n";
		output += editor->spaces[stack];
		output += "call DestroyGroup(ydl_group)\n";

		return true;
	}
	case "YDWESaveAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataSet(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor->convertParameter(parameters[1], node, pre_actions);
		output += ",\"";
		output += parameters[2]->value;
		output += "\",";
		output += parameters[3]->value + 11;
		output += ",";
		output += editor->convertParameter(parameters[4], node, pre_actions);
		output += ")\n";
		return true;
	}
	case "YDWEFlushAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataClear(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor->convertParameter(parameters[1], node, pre_actions);
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
		output += editor->convertParameter(parameters[1], node, pre_actions);
		output += ")\n";
		return true;
	}

	case "YDWEExecuteTriggerMultiple"s_hash:
	{
		output += "set ydl_trigger = ";
		output += editor->convertParameter(parameters[0], node, pre_actions);
		output += "\n" + editor->spaces[stack];
		output += "YDLocalExecuteTrigger(ydl_trigger)\n";
	
		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor->spaces[stack];
			output += editor->convertAction(child, pre_actions, false) + "\n";
		}
		output += editor->spaces[stack];
		output += "call YDTriggerExecuteTrigger(ydl_trigger,";
		output += editor->convertParameter(parameters[1], node, pre_actions);
		output += ")\n";
		return true;
		
	}
	
	case "YDWETimerStartMultiple"s_hash:
	{

		std::string param_text;
		std::string action_text;

		param_text += "set ydl_timer = ";
		param_text += editor->convertParameter(parameters[0], node, pre_actions) + "\n";


		std::map<std::string, std::string> hashVarTable;

		//当前这一层需要传参的变量表
		std::map<std::string, std::string> thisVarTable;

		
		//找到上一层函数的逆天局部变量表
		auto mapPtr = node->getLastVarTable();

		node->getChildNodeList(list);
		for (auto& child : list)
		{
			Action* childAction = child->getAction();
			if (child->getActionId() == 0)//如果是参数区
			{
				switch (child->getNameId())
				{
					//在逆天计时器参数中使用逆天局部变量
				case "YDWESetAnyTypeLocalArray"s_hash:
				case "YDWESetAnyTypeLocalVariable"s_hash:
				{
					std::string var_name = childAction->parameters[1]->value;
					std::string var_type = childAction->parameters[0]->value + 11;
					hashVarTable.emplace(var_name, var_type);
					break;
				}
				}
				param_text += editor->spaces[stack];
				param_text += editor->convertAction(child, pre_actions, false) + "\n";
			}
		}


		int s = stack;
		stack = 1;
		
		for (auto& child : list)
		{
			if (child->getActionId() != 0)//如果是动作区
			{
				Action* childAction = child->getAction();

				seachHashLocal(childAction->parameters, childAction->param_count, &thisVarTable);
				action_text += editor->spaces[stack];
				action_text += editor->convertAction(child, pre_actions, false) + "\n";
			}
		}

		stack = s;

		for (auto&[n, t] : hashVarTable)
		{	
			thisVarTable.erase(n);
			if (mapPtr)
			{
				mapPtr->erase(n);
			}
		}

		output += param_text;

		ActionNodePtr temp = ActionNodePtr(new ActionNode(action, node));

		//如果当前这层有需要申请的变量
		if (mapPtr->size() > 0)
		{
			for (auto&[n, t] : *mapPtr)
			{
				output += editor->spaces[stack];
				output += setLocal(temp, n, t, getLocal(node, n, t), true) + "\n";
			}
			mapPtr->clear();
		}
		
		//将这一层需要传参的变量 传递给上一层
		for (auto&[n, t] : thisVarTable)
		{
			output += editor->spaces[stack];
			output += setLocal(temp, n, t, getLocal(node, n, t),true) + "\n";

			mapPtr->emplace(n, t);
		}

		std::string func_name = editor->generate_function_name(node->getTriggerNamePtr());
		pre_actions += "function " + func_name + " takes nothing returns nothing\n";


		onActionsToFuncBegin(pre_actions,node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";

		output += editor->spaces[stack];
		output += "call TimerStart(ydl_timer,";
		output += editor->convertParameter(parameters[1], node, pre_actions);
		output += ",";
		output += editor->convertParameter(parameters[2], node, pre_actions);
		output += ",function ";
		output += func_name;
		output += ")";

		return true;
		
	}


	case "YDWERegisterTriggerMultiple"s_hash:
	{
		std::string param_text;
		std::string action_text;

		param_text += "set ydl_trigger = ";
		param_text += editor->convertParameter(parameters[0], node, pre_actions) + "\n";


		std::map<std::string, std::string> hashVarTable;

		//当前这一层需要传参的变量表
		std::map<std::string, std::string> thisVarTable;

		//找到上一层函数的逆天局部变量表
		auto mapPtr = node->getLastVarTable();

		node->getChildNodeList(list);

		for (auto& child : list)
		{
			Action* childAction = child->getAction();

			//如果是事件 则单独处理
			if (child->getActionType() == Action::Type::event)
			{
				if (child->getNameId() == "MapInitializationEvent"s_hash)
				{
					continue;
				}
				onRegisterEvent(param_text,child);
				param_text += editor->spaces[stack];
				
				param_text += "call " + editor->getBaseName(child) + "(ydl_trigger";

				for (size_t k = 0; k < childAction->param_count; k++)
				{
					param_text += ", ";
					param_text += editor->convertParameter(childAction->parameters[k], child, pre_actions);
				}
				param_text += ")\n";
				onRegisterEvent2(param_text, child);
			}
			else if (child->getActionId() == 1)//如果是参数区
			{
				switch (child->getNameId())
				{
					//在逆天计时器参数中使用逆天局部变量
				case "YDWESetAnyTypeLocalArray"s_hash:
				case "YDWESetAnyTypeLocalVariable"s_hash:
				{
					std::string var_name = childAction->parameters[1]->value;
					std::string var_type = childAction->parameters[0]->value + 11;
					hashVarTable.emplace(var_name, var_type);
					break;
				}
				}
				param_text += editor->spaces[stack];
				param_text += editor->convertAction(child, pre_actions, false) + "\n";
			}
		}

		int s = stack;
		stack = 1;

		for (auto& child : list)
		{
			Action* childAction = child->getAction();

			if (child->getActionId() == 2)//如果是动作区
			{
				seachHashLocal(childAction->parameters, childAction->param_count, &thisVarTable);
				action_text += editor->spaces[stack];
				action_text += editor->convertAction(child, pre_actions, false) + "\n";
			}
		}

		stack = s;

		for (auto&[n, t] : hashVarTable)
		{
			thisVarTable.erase(n);
			if (mapPtr)
			{
				mapPtr->erase(n);
			}
		}

		output += param_text;

		ActionNodePtr temp = ActionNodePtr(new ActionNode(action, node));

		//如果当前这层有需要申请的变量
		if (mapPtr->size() > 0)
		{
			for (auto&[n, t] : *mapPtr)
			{
				output += editor->spaces[stack];
				output += setLocal(temp, n, t, getLocal(node, n, t), true) + "\n";
			}
		}

		//将这一层需要传参的变量 传递给上一层
		for (auto&[n, t] : thisVarTable)
		{
			output += editor->spaces[stack];
			output += setLocal(temp, n, t, getLocal(node, n, t), true) + "\n";

			mapPtr->emplace(n, t);
		}



		std::string func_name = editor->generate_function_name(node->getTriggerNamePtr());
		pre_actions += "function " + func_name + " takes nothing returns nothing\n";
	
		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";

		output += editor->spaces[stack];
		output += "call TriggerAddCondition(ydl_trigger,Condition(function ";
		output += func_name;
		output += "))";

		return true;
	}

	case "YDWESetAnyTypeLocalVariable"s_hash:
	{
	
		std::string var_name = parameters[1]->value;
		std::string var_type= parameters[0]->value + 11;

		std::string var_value = editor->convertParameter(parameters[2], node, pre_actions);
	
		output +=setLocal(node,var_name, var_type, var_value);
		return true;
	}
	case "YDWESetAnyTypeLocalArray"s_hash:
	{

		std::string var_name = parameters[1]->value;
		std::string var_type = parameters[0]->value + 11;

		std::string index = editor->convertParameter(parameters[2], node, pre_actions);
		std::string var_value = editor->convertParameter(parameters[3], node, pre_actions);

		output += setLocalArray(node, var_name, var_type,index, var_value);
		return true;
	}



	case "YDWETimerStartFlush"s_hash:
	{
		ActionNodePtr ptr = node->getParentNode();
		bool isInTimer = false;
		while (ptr.get())
		{
			if (ptr->getNameId() == "YDWETimerStartMultiple"s_hash)
			{
				isInTimer = true;
				break;
			}
			ptr = ptr->getParentNode();
		}
		if (isInTimer)
		{
			output += "call YDLocal3Release()\n";
			output += editor->spaces[stack];
			output += "call DestroyTimer(GetExpiredTimer())\n";
			return true;
		}
		else
		{
			output += "不要在逆天计时器的动作外使用<清除逆天计时器>";
			return true;
		}
	}
	case "YDWERegisterTriggerFlush"s_hash:
	{
		ActionNodePtr parent = node->getParentNode();

		ActionNodePtr ptr = node->getParentNode();
		bool isInTrigger = false;
		while (ptr.get())
		{
			if (ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash)
			{
				isInTrigger = true;
				break;
			}
			ptr = ptr->getParentNode();
		}
		if (isInTrigger)
		{
			output += "call YDLocal4Release()\n";
			output += editor->spaces[stack];
			output += "call DestroyTrigger(GetTriggeringTrigger())\n";
			return true;
		}
		else
		{
			output += "不要在逆天触发器的动作外使用<清除逆天触发器>";
			return true;
		}
	}
	
	case "TriggerSleepAction"s_hash:
	case "PolledWait"s_hash:
	{

		
		output += editor->convertCall(node, pre_actions, !nested) + "\n";


		ActionNodePtr branch = node->getBranchNode();

		ActionNodePtr parent = branch->getParentNode();
		if (parent.get() && (
			parent->getNameId() == "YDWETimerStartMultiple"s_hash ||
			parent->getNameId() == "YDWERegisterTriggerMultiple"s_hash
			))
		{
			output += "不要在逆天计时器/逆天触发器内使用等待";
		}
		else
		{
			output += editor->spaces[stack];
			output += "call YDLocalReset()\n";
		}
			
		return true;
	}
	case "ReturnAction"s_hash:
	{
		output += editor->spaces[stack];
		onActionsToFuncEnd(output, node);
		output += "return\n";
		return true;
	}

	case "YDWEExitLoop"s_hash:
	{
		output += "exitwhen true\n";
		return true;
	}
	case "CustomScriptCode"s_hash:
	case "YDWECustomScriptCode"s_hash:
	{
		output += parameters[0]->value;
		return true;
	}
	case "YDWEActivateTrigger"s_hash:
	{
		output += "";
		Parameter* param = parameters[0];
		if (param->typeId == Parameter::Type::variable && param->arrayParam == NULL)
		{
			const char* ptr = param->value;
			if (ptr && strncmp(ptr, "gg_trg_", 7) == 0)
				ptr = ptr + 7;
			
			std::string func_name = std::string("InitTrig_") + ptr;
			convert_name(func_name);

			std::string ret = editor->convertParameter(parameters[1], node, pre_actions);
			if (ret.compare("true") == 0)
			{
				output += "call ExecuteFunc(\"" + func_name + "\")\n";
			}
			else
			{
				output += "call " + func_name + "()\n";
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
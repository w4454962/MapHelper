#include "stdafx.h"
#include "YDTrigger.h"
#include "TriggerEditor.h"
#include "WorldEditor.h"
#include "SaveLoadCheck.h"

YDTrigger::YDTrigger()
	:m_bEnable(true),
	 m_funcStack(0),
	m_enumUnitStack(0)
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
	writer.write_string("#define USE_BJ_ANTI_LEAK\n");
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
	auto& editor = get_trigger_editor();
	int& stack = editor.space_stack;

	std::vector<ActionNodePtr> list;
	Action* action = node->getAction();
	
	Parameter** parameters = action->parameters;

	switch (node->getNameId())
	{
	case "YDWEForLoopLocVarMultiple"s_hash:
	{
		
		std::string variable = std::string("ydul_") + action->parameters[0]->value;
		convert_loop_var_name(variable);

		output += "set " + variable + " = ";
		output += editor.convertParameter(parameters[1], node, pre_actions) + "\n";
		output += editor.spaces[stack];
		output += "loop\n";
		output += editor.spaces[++stack];
		output += "exitwhen " + variable + " > " + editor.convertParameter(parameters[2], node, pre_actions) + "\n";
	
		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor.spaces[stack];
			output += editor.convertAction(child, pre_actions, false) + "\n";
		}

		output += editor.spaces[stack];
		output += "set " + variable + " = " + variable + " + 1\n";
		output += editor.spaces[--stack];
		output += "endloop\n";
		return true;
	}
	case "YDWERegionMultiple"s_hash:
	{
		output += "// --------------------\n";
		output += editor.spaces[stack];
		output += "//" + std::string(parameters[0]->value) + "\n";

		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor.spaces[stack];
			output += editor.convertAction(child, pre_actions, false) + "\n";
		}
		output += editor.spaces[stack];
		output += "// --------------------\n";

		return true;
	}
	case "YDWEEnumUnitsInRangeMultiple"s_hash:
	{
		
		output += "set ydl_group = CreateGroup()\n";
		output += editor.spaces[stack];
		output += "call GroupEnumUnitsInRange(ydl_group,";
		output += editor.convertParameter(parameters[0], node, pre_actions);
		output += ",";
		output += editor.convertParameter(parameters[1], node, pre_actions);
		output += ",";
		output += editor.convertParameter(parameters[2], node, pre_actions);
		output += ",null)\n";
		output += editor.spaces[stack];
		output += "loop\n";

		output += editor.spaces[++stack];
		output += "set ydl_unit = FirstOfGroup(ydl_group)\n";

		output += editor.spaces[stack];
		output += "exitwhen ydl_unit == null\n";

		output += editor.spaces[stack];
		output += "call GroupRemoveUnit(ydl_group, ydl_unit)\n";
		m_enumUnitStack++;
		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor.spaces[stack];
			//循环里的子动作 沿用外面相同的父节点
			output += editor.convertAction(child, pre_actions, false) + "\n";
		}
		m_enumUnitStack--;
		output += editor.spaces[--stack];
		output += "endloop\n";
		output += editor.spaces[stack];
		output += "call DestroyGroup(ydl_group)\n";
		
		return true;
	}
	case "YDWESaveAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataSet(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor.convertParameter(parameters[1], node, pre_actions);
		output += ",\"";
		output += parameters[2]->value;
		output += "\",";
		output += parameters[3]->value + 11;
		output += ",";
		output += editor.convertParameter(parameters[4], node, pre_actions);
		output += ")\n";
		return true;
	}
	case "YDWEFlushAnyTypeDataByUserData"s_hash:
	{
		output += "call YDUserDataClear(";
		output += parameters[0]->value + 11; //typename_01_integer  + 11 = integer
		output += ",";
		output += editor.convertParameter(parameters[1], node, pre_actions);
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
		output += editor.convertParameter(parameters[1], node, pre_actions);
		output += ")\n";
		return true;
	}

	case "YDWEExecuteTriggerMultiple"s_hash:
	{
		output += "set ydl_trigger = ";
		output += editor.convertParameter(parameters[0], node, pre_actions);
		output += "\n" + editor.spaces[stack];
		output += "YDLocalExecuteTrigger(ydl_trigger)\n";
	
		node->getChildNodeList(list);
		for (auto& child : list)
		{
			output += editor.spaces[stack];
			output += editor.convertAction(child, pre_actions, false) + "\n";
		}
		output += editor.spaces[stack];
		output += "call YDTriggerExecuteTrigger(ydl_trigger,";
		output += editor.convertParameter(parameters[1], node, pre_actions);
		output += ")\n";
		return true;
		
	}
	
	case "YDWETimerStartMultiple"s_hash:
	{

		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		pre_actions += "function " + func_name + " takes nothing returns nothing\n";


		onActionsToFuncBegin(pre_actions,node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";

		output += editor.spaces[stack];
		output += "call TimerStart(ydl_timer,";
		output += editor.convertParameter(parameters[1], node, pre_actions);
		output += ",";
		output += editor.convertParameter(parameters[2], node, pre_actions);
		output += ",function ";
		output += func_name;
		output += ")";

		return true;
		
	}


	case "YDWERegisterTriggerMultiple"s_hash:
	{
		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		pre_actions += "function " + func_name + " takes nothing returns nothing\n";
	
		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";

		output += editor.spaces[stack];
		output += "call TriggerAddCondition(ydl_trigger,Condition(function ";
		output += func_name;
		output += "))";

		return true;
	}

	case "YDWESetAnyTypeLocalVariable"s_hash:
	{
	
		std::string var_name = parameters[1]->value;
		std::string var_type= parameters[0]->value + 11;

		std::string var_value = editor.convertParameter(parameters[2], node, pre_actions);
	
		output +=setLocal(node,var_name, var_type, var_value);

		return true;
	}
	case "YDWESetAnyTypeLocalArray"s_hash:
	{

		std::string var_name = parameters[1]->value;
		std::string var_type = parameters[0]->value + 11;

		std::string index = editor.convertParameter(parameters[2], node, pre_actions);
		std::string var_value = editor.convertParameter(parameters[3], node, pre_actions);

		output += setLocalArray(node, var_name, var_type,index, var_value);
		return true;
	}



	case "YDWETimerStartFlush"s_hash:
	{
		ActionNodePtr branch = node->getBranchNode();
		ActionNodePtr ptr = branch->getParentNode();
		bool isInTimer = false;
		while (ptr.get())
		{
			if (ptr->getNameId() == "YDWETimerStartMultiple"s_hash)
			{
				isInTimer = true;
				break;
			}
			//如果节点无法穿透就跳出
			if (!ptr->canHasVarTable())
				ptr = ptr->getBranchNode()->getParentNode();
			else break;
		}
		if (isInTimer)
		{
			output += "call YDLocal3Release()\n";
			output += editor.spaces[stack];
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
		ActionNodePtr branch = node->getBranchNode();
		ActionNodePtr ptr = branch->getParentNode();
		bool isInTrigger = false;
		while (ptr.get())
		{
			if (ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash)
			{
				isInTrigger = true;
				break;
			}
			//如果节点无法穿透就跳出
			if (!ptr->canHasVarTable())
				ptr = ptr->getBranchNode()->getParentNode();
			else break;
		}
		if (isInTrigger)
		{
			output += "call YDLocal4Release()\n";
			output += editor.spaces[stack];
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

		
		output += editor.convertCall(node, pre_actions, !nested) + "\n";


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
			ActionNodePtr root = node->getRootNode();
			if (parent.get() == nullptr || parent->isRootNode())
			{
				if (root->m_haveHashLocal)
				{
					output += editor.spaces[stack];
					output += "call YDLocalReset()\n";
				}
			}
		}
		return true;
	}
	case "ReturnAction"s_hash:
	{
		output += editor.spaces[stack];
		ActionNodePtr branch = node->getBranchNode();
		if (branch->isRootNode() || branch->getParentNode()->isRootNode())
		{
			onActionsToFuncEnd(output, node->getRootNode());
		}
		
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

			std::string ret = editor.convertParameter(parameters[1], node, pre_actions);
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
	case "DzTriggerRegisterMouseEventMultiple"s_hash:
	case "DzTriggerRegisterKeyEventMultiple"s_hash:
	{
		std::string action_name = node->getName();
		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		action_name = action_name.replace(action_name.find("Multiple"),-1,"ByCode");

		pre_actions += "function " + func_name + " takes nothing returns nothing\n";

		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";
		output += editor.spaces[stack];
		output += "if GetLocalPlayer() == " + editor.convertParameter(parameters[0], node, pre_actions) + " then\n";
		output += editor.spaces[stack] + editor.spaces[stack];
		output += "call "+ action_name +"(null, " + editor.convertParameter(parameters[2], node, pre_actions) +
			", " + editor.convertParameter(parameters[1], node, pre_actions) + ", false, function ";
		output += func_name;
		output += ")";
		output += editor.spaces[stack] + "\n";
		output += editor.spaces[stack] + "endif";
		if (output.front() == '\t')
			output = output.erase(0, 1);
		return true;
	}
	case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
	case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
	case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
	{
		std::string action_name = node->getName();
		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		action_name = action_name.replace(action_name.find("Multiple"), -1, "ByCode");

		pre_actions += "function " + func_name + " takes nothing returns nothing\n";

		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";
		output += editor.spaces[stack];
		output += "if GetLocalPlayer() == " + editor.convertParameter(parameters[0], node, pre_actions) + " then\n";
		output += editor.spaces[stack] + editor.spaces[stack];
		output += "call " + action_name + "(null, false, function ";
		output += func_name;
		output += ")";
		output += editor.spaces[stack] + "\n";
		output += editor.spaces[stack] + "endif";
		if (output.front() == '\t')
			output = output.erase(0, 1);
		return true;
	}
	case "DzFrameSetUpdateCallbackMultiple"s_hash:
	{
		std::string action_name = node->getName();
		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		action_name = action_name.replace(action_name.find("Multiple"), -1, "ByCode");

		pre_actions += "function " + func_name + " takes nothing returns nothing\n";

		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";
		output += editor.spaces[stack];
		output += "if GetLocalPlayer() == " + editor.convertParameter(parameters[0], node, pre_actions) + " then\n";
		output += editor.spaces[stack] + editor.spaces[stack];
		output += "call " + action_name + "(function ";
		output += func_name;
		output += ")";
		output += editor.spaces[stack] + "\n";
		output += editor.spaces[stack] + "endif";
		if (output.front() == '\t')
			output = output.erase(0, 1);
		return true;
	}
	case "DzFrameSetScriptMultiple"s_hash:
	{
		std::string action_name = node->getName();
		std::string func_name = editor.generate_function_name(node->getTriggerNamePtr());
		node->setFunctionNamePtr(func_name);
		std::string action_text = localVarTransfer(output, node, pre_actions);
		action_name = action_name.replace(action_name.find("Multiple"), -1, "ByCode");

		pre_actions += "function " + func_name + " takes nothing returns nothing\n";

		onActionsToFuncBegin(pre_actions, node);
		pre_actions += action_text;
		onActionsToFuncEnd(pre_actions, node);
		pre_actions += "endfunction\n";
		output += editor.spaces[stack];
		output += "if GetLocalPlayer() == " + editor.convertParameter(parameters[0], node, pre_actions) + " then\n";
		output += editor.spaces[stack] + editor.spaces[stack];
		output += "call " + action_name + "("+ editor.convertParameter(parameters[2], node, pre_actions)+
			", " + editor.convertParameter(parameters[1], node, pre_actions) + ", function ";
		output += func_name;
		output += ", false)";
		output += editor.spaces[stack] + "\n";
		output += editor.spaces[stack] + "endif";
		if (output.front() == '\t')
			output = output.erase(0,1);
		return true;
	}
	}

	return false;
}

bool YDTrigger::onParamterToJass(Parameter* paramter, ActionNodePtr node, std::string& output, std::string& pre_actions, bool nested)
{
	auto& editor = get_trigger_editor();

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
			output += editor.convertParameter(parameters[1], node, pre_actions);
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
			output += editor.convertParameter(parameters[1], node, pre_actions);
			output += ",\"";
			output += parameters[3]->value;
			output += "\",";
			output += parameters[2]->value + 11;
			output += ")";
			return true;
		}
		case "GetEnumUnit"s_hash:
		{
			if (m_enumUnitStack > 0 && node->getBranchNode()->getParentNode()->getNameId() == "YDWEEnumUnitsInRangeMultiple"s_hash)
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
			if (m_enumUnitStack > 0 && node->getBranchNode()->getParentNode()->getNameId() == "YDWEEnumUnitsInRangeMultiple"s_hash)
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
			std::string variable = std::string("ydul_") + action->parameters[0]->value;
			convert_loop_var_name(variable);
			output += variable;
			return true;
		}
		case "YDWEGetAnyTypeLocalVariable"s_hash:
		{
			std::string var_name = parameters[0]->value;
			std::string var_type = paramter->type_name;

			auto mapPtr = node->getLastVarTable();
			
			if (mapPtr->find(var_name) != mapPtr->end() && mapPtr->at(var_name) != var_type)
				var_name = "error" + var_name;
			mapPtr->emplace(var_name, var_type);
			output += getLocal(node, var_name, var_type);
			return true;
		}
		case "YDWEGetAnyTypeLocalArray"s_hash:
		{
			std::string var_name = parameters[0]->value;
			std::string var_type = paramter->type_name;
			std::string index = editor.convertParameter(parameters[1], node, pre_actions);
			
			output += getLocalArray(node, var_name, var_type,index);
			return true;
		}
		case "CustomScriptCode"s_hash:
		case "YDWECustomScriptCode"s_hash:
		{
			output += parameters[0]->value;
			return true;
		}
		case "GetTriggeringTrigger"s_hash:
		case "GetTriggerEventId"s_hash:
		case "GetEventGameState"s_hash:
		case "GetWinningPlayer"s_hash:
		case "GetTriggeringRegion"s_hash:
		case "GetEnteringUnit"s_hash:
		case "GetLeavingUnit"s_hash:
		case "GetTriggeringTrackable"s_hash:
		case "GetClickedButton"s_hash:
		case "GetClickedDialog"s_hash:
		case "GetTournamentFinishSoonTimeRemaining"s_hash:
		case "GetTournamentFinishNowRule"s_hash:
		case "GetTournamentFinishNowPlayer"s_hash:
		case "GetSaveBasicFilename"s_hash:
		case "GetTriggerPlayer"s_hash:
		case "GetLevelingUnit"s_hash:
		case "GetLearningUnit"s_hash:
		case "GetLearnedSkill"s_hash:
		case "GetLearnedSkillLevel"s_hash:
		case "GetRevivableUnit"s_hash:
		case "GetRevivingUnit"s_hash:
		case "GetAttacker"s_hash:
		case "GetRescuer"s_hash:
		case "GetDyingUnit"s_hash:
		case "GetKillingUnit"s_hash:
		case "GetDecayingUnit"s_hash:
		case "GetConstructingStructure"s_hash:
		case "GetCancelledStructure"s_hash:
		case "GetConstructedStructure"s_hash:
		case "GetResearchingUnit"s_hash:
		case "GetResearched"s_hash:
		case "GetTrainedUnitType"s_hash:
		case "GetTrainedUnit"s_hash:
		case "GetDetectedUnit"s_hash:
		case "GetSummoningUnit"s_hash:
		case "GetSummonedUnit"s_hash:
		case "GetTransportUnit"s_hash:
		case "GetLoadedUnit"s_hash:
		case "GetSellingUnit"s_hash:
		case "GetSoldUnit"s_hash:
		case "GetBuyingUnit"s_hash:
		case "GetSoldItem"s_hash:
		case "GetChangingUnit"s_hash:
		case "GetChangingUnitPrevOwner"s_hash:
		case "GetManipulatingUnit"s_hash:
		case "GetManipulatedItem"s_hash:
		case "GetOrderedUnit"s_hash:
		case "GetIssuedOrderId"s_hash:
		case "GetOrderPointX"s_hash:
		case "GetOrderPointY"s_hash:
		case "GetOrderPointLoc"s_hash:
		case "GetOrderTarget"s_hash:
		case "GetOrderTargetDestructable"s_hash:
		case "GetOrderTargetItem"s_hash:
		case "GetOrderTargetUnit"s_hash:
		case "GetSpellAbilityUnit"s_hash:
		case "GetSpellAbilityId"s_hash:
		case "GetSpellAbility"s_hash:
		case "GetSpellTargetLoc"s_hash:
		case "GetSpellTargetDestructable"s_hash:
		case "GetSpellTargetItem"s_hash:
		case "GetSpellTargetUnit"s_hash:
		case "GetEventPlayerState"s_hash:
		case "GetEventPlayerChatString"s_hash:
		case "GetEventPlayerChatStringMatched"s_hash:
		case "GetTriggerUnit"s_hash:
		case "GetEventUnitState"s_hash:
		case "GetEventDamage"s_hash:
		case "GetEventDamageSource"s_hash:
		case "GetEventDetectingPlayer"s_hash:
		case "GetEventTargetUnit"s_hash:
		case "GetTriggerWidget"s_hash:
		case "GetTriggerDestructable"s_hash:
		{
			//判断这些接口是否在触发器最外层被引用 自动传递参数

			

			ActionNodePtr ptr = node;
			while (!ptr->isRootNode())
			{
				ActionNodePtr parentPtr = ptr->getParentNode();
				int flag = 0;
				switch (parentPtr->getNameId())
				{
				case "YDWEExecuteTriggerMultiple"s_hash:
					flag = 1; 
					break;
				case "YDWETimerStartMultiple"s_hash:
					flag = ptr->getActionId() == 0 ? 1 : 2;
					break;
				case "DzTriggerRegisterMouseEventMultiple"s_hash:
				case "DzTriggerRegisterKeyEventMultiple"s_hash:
				case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
				case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
				case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
				case "DzFrameSetUpdateCallbackMultiple"s_hash:
				case "DzFrameSetScriptMultiple"s_hash:
					flag = ptr->getActionId() == 0 ? 1 : 2;
					break;
				// 逆天触发器并不会自动传参
				case "YDWERegisterTriggerMultiple"s_hash:
					return false;
				//	flag = ptr->getActionId() < 2 ? 1 : 2;
				//	break;
				}
				//如果当前节点为注册逆天触发则不进行传参
				if (flag > 0)
				{
					std::string var_name = action->name;
					std::string var_type = "AUTO_" + editor.getBaseType(paramter->type_name);

					auto mapPtr = parentPtr->getVarTable();

					mapPtr->emplace(var_name, var_type);

					//如果flag = 1 表示是在传参区 跟父节点是相同的环境
					//flag == 2 表示是在 传参后的动作区 则返回局部变量代码
					if (flag == 2)
					{
						output += getLocal(ptr, var_name, var_type);
						return true;
					}
				}
				
				ptr = parentPtr;
			}
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
			if (seachHashLocal(childs, child_count, mapPtr))
				return true;
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
			if (!isSeachChild && action->group_id != -1 )
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
				std::string varname = std::string("ydul_") + action->parameters[0]->value;
				convert_loop_var_name(varname);
				addLocalVar(varname, "integer");
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
				next(true);
				break;
			}
			case "YDWERegisterTriggerMultiple"s_hash:
			{
				addLocalVar("ydl_trigger", "trigger");
				next(true);
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

			default:
				if (action->child_count > 0)
				{
					next(true);
				}
			}
#undef next
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

std::string YDTrigger::localVarTransfer(std::string& output, ActionNodePtr node, std::string& pre_actions)
{
	auto& editor = get_trigger_editor();
	int& stack = editor.space_stack;
	Action* action = node->getAction();
	Parameter** parameters = action->parameters;
	std::vector<ActionNodePtr> list;
	std::string param_text;
	std::string action_text;
	int param_index;//参数区索引
	bool param_auto;//是否自动传参
	switch (node->getNameId())
	{
		case "YDWERegisterTriggerMultiple"s_hash:
			param_text += "set ydl_trigger = ";
			param_text += editor.convertParameter(parameters[0], node, pre_actions) + "\n";
			param_index = 1;
			param_auto = false;
			break;
		case "YDWETimerStartMultiple"s_hash:
			param_text += "set ydl_timer = ";
			param_text += editor.convertParameter(parameters[0], node, pre_actions) + "\n";
			param_index = 0;
			param_auto = true;
			break;
		case "DzTriggerRegisterMouseEventMultiple"s_hash:
		case "DzTriggerRegisterKeyEventMultiple"s_hash:
		case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
		case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
		case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		case "DzFrameSetUpdateCallbackMultiple"s_hash:
		case "DzFrameSetScriptMultiple"s_hash:
			param_index = 0;
			param_auto = true;
			break;
	}


	std::map<std::string, std::string> hashVarTable;

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
			onRegisterEvent(param_text, child);
			param_text += editor.spaces[stack];

			param_text += "call " + editor.getBaseName(child) + "(ydl_trigger";

			for (size_t k = 0; k < childAction->param_count; k++)
			{
				param_text += ", ";
				param_text += editor.convertParameter(childAction->parameters[k], child, pre_actions);
			}
			param_text += ")\n";
			onRegisterEvent2(param_text, child);
		}
		else if (child->getActionId() == param_index)//如果是参数区
		{
			switch (child->getNameId())
			{
				//使用逆天局部变量
			case "YDWESetAnyTypeLocalArray"s_hash:
			case "YDWESetAnyTypeLocalVariable"s_hash:
			{
				std::string var_name = childAction->parameters[1]->value;
				std::string var_type = childAction->parameters[0]->value + 11;
				hashVarTable.emplace(var_name, var_type);
				break;
			}
			}
			param_text += editor.spaces[stack];
			param_text += editor.convertAction(child, pre_actions, false) + "\n";
		}
	}

	int s = stack;
	stack = 1;

	for (auto& child : list)
	{
		Action* childAction = child->getAction();

		if (child->getActionId() > param_index)//如果是动作区
		{
			action_text += editor.spaces[stack];
			action_text += editor.convertAction(child, pre_actions, false) + "\n";
		}
	}

	stack = s;

	//如果当前这层有需要申请的变量
	auto table = node->getVarTable();


	for (auto& [n, t] : hashVarTable)
	{
		table->erase(n);

		if (mapPtr)
		{
			mapPtr->erase(n);
		}
	}

	output += param_text;

	ActionNodePtr temp = ActionNodePtr(new ActionNode(action, node));

	if (table->size() > 0)
	{
		for (auto& [n, t] : *table)
		{
			if (strncmp(t.c_str(), "AUTO_", 5) == 0)
			{
				if (!param_auto)
					continue;
				output += editor.spaces[stack];
				auto branch = node->getBranchNode();
				auto parent = branch->getParentNode();
				if (!parent.get() || parent->getNameId() == "YDWERegisterTriggerMultiple"s_hash)
				{
					output += setLocal(temp, n, t, n + "()", true) + "\n";
				}
				else
				{
					output += setLocal(temp, n, t, getLocal(node, n, t), true) + "\n";
				}
			}
			else
			{
				output += editor.spaces[stack];
				output += setLocal(temp, n, t, getLocal(node, n, t), true) + "\n";
			}

			//将这一层需要传参的变量 传递给上一层
			if (mapPtr.get() != table.get())
			{
				mapPtr->emplace(n, t);
			}
		}
		table->clear();
	}
	return action_text;
}

std::string YDTrigger::setLocal(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& value,bool add)
{
	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();
	//根据当前设置逆天局部变量的位置 来决定生成的代码

	std::string callname;
	std::string handle;

	//如果是自动传递GetTriggerUnit() 这些函数值的话
	bool isauto = type.length() > 5 && strncmp(type.c_str(), "AUTO_", 5) == 0;


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
			if (branch->getActionId() < 2 || add)
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
		case "DzTriggerRegisterMouseEventMultiple"s_hash:
		case "DzTriggerRegisterKeyEventMultiple"s_hash:
		case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
		case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
		case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		case "DzFrameSetUpdateCallbackMultiple"s_hash:
		case "DzFrameSetScriptMultiple"s_hash:
		{
			callname = "YDLocal6Set";
			handle = "\"" + *parent->getFunctionNamePtr() + "\"";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			callname = "YDLocal2Set";
			ActionNodePtr ptr = parent;
			while (!ptr->isRootNode())
			{
				ActionNodePtr parentPtr = ptr->getParentNode();
				if (parentPtr->canHasVarTable())
				{
					return setLocal(ptr, name, type,value,add);
				}
				ptr = parentPtr;
			}
			break;
		}
		}
	}

	std::string output;
	std::string var_type;
	if (isauto)
	{
		var_type = std::string(type.begin() + 5, type.end());
	}
	else
	{
		var_type = type;
	}

	std::string tmp = name;
	std::string val = value;
	if (tmp.substr(0, 5) ==  "error") {
		tmp = tmp.substr(5, tmp.size());
	}
	
	if (SaveLoadCheck_Set(tmp, var_type) == false)
		SaveLoadError(node, tmp, var_type);
		//val = SaveLoadError(node, tmp, var_type) + val;
	output += "call " + callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += var_type + ",\"" + tmp + "\"," + val + ")";

  
	return output;
}

std::string YDTrigger::getLocal(ActionNodePtr node, const std::string& name, const std::string& type)
{
	ActionNodePtr branch = node->getBranchNode();

	ActionNodePtr parent = branch->getParentNode();

	//根据当前设置逆天局部变量的位置 来决定生成的代码
	std::string callname;
	std::string handle;

	//如果是自动传递GetTriggerUnit() 这些函数值的话
	bool isauto = type.length() > 5 && strncmp(type.c_str(), "AUTO_", 5) == 0;


	if (parent.get() == NULL || parent->isRootNode() || branch->isRootNode())//如果是在触发中
	{
		//如果是自动传递GetTriggerUnit() 这些函数值的话
		if (isauto)
		{
			return name + "()";
		}
		callname = "YDLocal1Get";
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
			// 逆天计时器第二个参数如果转换传参会bug
			else if (branch->getActionId() == 0xFFFFFFFF) {
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
				return getLocal(parent, name, type);
			}
			else
			{//否则是动作区
				handle = "GetTriggeringTrigger()";
			}
			callname = "YDLocalGet";
			break;
		}
		case "DzTriggerRegisterMouseEventMultiple"s_hash:
		case "DzTriggerRegisterKeyEventMultiple"s_hash:
		case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
		case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
		case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		case "DzFrameSetUpdateCallbackMultiple"s_hash:
		case "DzFrameSetScriptMultiple"s_hash:
		{
			if (branch->getActionId() != 1) //只要不是动作区域就获取父级的局部变量
			{
				return getLocal(parent, name, type);
			}
			callname = "YDLocal6Get";
			handle = "\"" + *parent->getFunctionNamePtr() + "\"";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{


			ActionNodePtr ptr = parent;
			while (!ptr->isRootNode())
			{
				ActionNodePtr parentPtr = ptr->getParentNode();
				if (parentPtr->canHasVarTable())
				{
					return getLocal(ptr, name, type);
				}
				ptr = parentPtr;
			}

			//如果是自动传递GetTriggerUnit() 这些函数值的话
			if (isauto)
			{
				return name + "()";
			}

			callname = "YDLocal2Get";
			break;
		}
		}
	}

	std::string output;

	std::string var_type;
	if (isauto)
		var_type = std::string(type.begin() + 5, type.end());
	else
		var_type = type;

	std::string tmp = name;
	if (tmp.substr(0, 5) == "error")
		tmp = tmp.substr(5, tmp.size());

	if (SaveLoadCheck_Set(tmp, var_type) == false)
		SaveLoadError(node, tmp, var_type);
		//output += SaveLoadError(node, tmp, var_type);
	output += callname + "(";
	if (!handle.empty()) //带handle参数的
	{
		output += handle + ",";
	}
	output += var_type + ",\"" + tmp + "\")";


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
			if (branch->getActionId() == 0)
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
			if (branch->getActionId() < 2 )
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
		case "DzTriggerRegisterMouseEventMultiple"s_hash:
		case "DzTriggerRegisterKeyEventMultiple"s_hash:
		case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
		case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
		case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		case "DzFrameSetUpdateCallbackMultiple"s_hash:
		case "DzFrameSetScriptMultiple"s_hash:
		{
			callname = "YDLocal6ArraySet";
			handle = "\"" + *parent->getFunctionNamePtr() + "\"";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{
			callname = "YDLocal2ArraySet";

			ActionNodePtr ptr = parent;
			while (!ptr->isRootNode())
			{
				ActionNodePtr parentPtr = ptr->getParentNode();
				if (parentPtr->canHasVarTable())
				{
					return setLocalArray(ptr, name, type,index, value);
				}
				ptr = parentPtr;
			}

		}

		}
	}

	std::string output;

	auto val = value;
	if (SaveLoadCheck_Set(name, type) == false)
		SaveLoadError(node, name, type);
		//val = SaveLoadError(node, name, type) + val;
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
	output += val;
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
		callname = "YDLocal1ArrayGet";
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
				return getLocalArray(parent, name, type,index);
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
				return getLocalArray(parent, name, type, index);
			}
			else
			{//否则是动作区
				handle = "GetTriggeringTrigger()";
			}
			callname = "YDLocalArrayGet";
			break;
		}
		case "DzTriggerRegisterMouseEventMultiple"s_hash:
		case "DzTriggerRegisterKeyEventMultiple"s_hash:
		case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
		case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
		case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
		case "DzFrameSetUpdateCallbackMultiple"s_hash:
		case "DzFrameSetScriptMultiple"s_hash:
		{
			if (branch->getActionId() != 1) //只要不是动作区域就获取父级的局部变量
			{
				return getLocalArray(parent, name, type, index);
			}
			callname = "YDLocal6ArrayGet";
			handle = "\"" + *parent->getFunctionNamePtr() + "\"";
			break;
		}
		//否则 在其他未定义的动作组中
		default:
		{

			callname = "YDLocal2ArrayGet";

			ActionNodePtr ptr = parent;
			while (!ptr->isRootNode())
			{
				ActionNodePtr parentPtr = ptr->getParentNode();
				if (parentPtr->canHasVarTable())
				{
					return getLocalArray(ptr, name, type, index);
				}

				ptr = parentPtr;
			}
			break;
		}
		}
	}
	std::string output;

	if (SaveLoadCheck_Set(name, type) == false)
		SaveLoadError(node, name, type);
		//output += SaveLoadError(node, name, type);
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
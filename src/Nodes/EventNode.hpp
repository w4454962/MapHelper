#pragma once
#include "Node.h"
#include "ActionNode.hpp"
#include "../TriggerEditor.h"

namespace mh {

	class EventNode : public ActionNode {
	public:
		REGISTER_FROM_ACTION(EventNode)

		bool hasAnyPlayer(Parameter** params, uint32_t count) {
			for (size_t i = 0; i < count; i++)
			{
				Parameter* param = params[i];
				Parameter** childs = NULL;
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
					return hasAnyPlayer(childs, child_count);
				}
			}
			return false;
		}

		virtual std::string toString(TriggerFunction* func) override {
			std::string result;

			std::string trigger_variable_name;

			if (getParentNode()->getType() == TYPE::ROOT) {
				//是在触发里 

				Trigger* trigger = (Trigger*)getRootNode()->getData();

				if (m_nameId == "YDWEDisableRegister"s_hash) {
					g_disableTriggerMap[trigger] = true;
					return "";

				} else if (m_nameId == "MapInitializationEvent"s_hash) {
					g_initTriggerMap[trigger] = true;

					return "";
				}

				trigger_variable_name = getTriggerVariableName();

			} else {
				//是在逆天触发器里
				
				if (m_nameId == "MapInitializationEvent"s_hash) {
					return "";
				}
				trigger_variable_name = "ydl_trigger";
			}

			auto& editor = get_trigger_editor();

			result = func->getSpaces() + "call " + editor.getScriptName(m_action) + "(" + trigger_variable_name;

			for (auto& param: getParameterList()) {
				result += ", " + param->toString(func);
			}

			result += ")\n";
			
			//如果有任意玩家 加上预处理宏
			if (hasAnyPlayer(m_action->parameters, m_action->param_count)) {
				result = "#define YDTRIGGER_COMMON_LOOP(n) " + result;
				result += "#define YDTRIGGER_COMMON_LOOP_LIMITS (0, 15)\n";
				result += "#include <YDTrigger/Common/loop.h>\n";
			}

			return result;
		}
	};

}


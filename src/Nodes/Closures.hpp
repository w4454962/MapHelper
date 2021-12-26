#pragma once


namespace mh {


	class YDWEExecuteTriggerMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWEExecuteTriggerMultiple)


			virtual std::string getUpvalueScriptName(UPVALUE_TYPE type) override {
			switch (type) {
			case mh::Node::UPVALUE_TYPE::SET_LOCAL:
				return "YDLocal5Set";
			case mh::Node::UPVALUE_TYPE::GET_LOCAL:
				break;
			case mh::Node::UPVALUE_TYPE::SET_ARRAY:
				return "YDLocal5ArraySet";
			case mh::Node::UPVALUE_TYPE::GET_ARRAY:
				break;
			default:
				break;
			}
			return std::string();
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_trigger", "trigger", std::string(), false);

			std::string result;

			result += func->getSpaces() + "set ydl_trigger = " + params[0]->toString(func) + "\n";
			result += "YDLocalExecuteTrigger(ydl_trigger)\n";

			for (auto& node : getChildList()) {
				result += node->toString(func);
			}
			result += func->getSpaces() + "call YDTriggerExecuteTrigger(ydl_trigger, " + params[1]->toString(func) + ")\n";

			return result;
		}
	};


	class YDWETimerStartMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWETimerStartMultiple)

			virtual uint32_t getCrossDomainIndex() override { return 0; }

		virtual bool isCrossDomain() override { return true; }

		virtual std::string getHandleName() override {
			if (getCurrentGroupId() <= getCrossDomainIndex()) {
				return "ydl_timer";
			}
			return "GetExpiredTimer()";
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_timer", "timer", std::string(), false);

			std::string result;

			std::string func_name = getFuncName() + "T";

			result += func->getSpaces() + "set ydl_timer = " + params[0]->toString(func) + "\n";
			result += func->getSpaces() + "call TimerStart(ydl_timer, "
				+ params[1]->toString(func) + ", "
				+ params[2]->toString(func)
				+ ", function " + func_name + ")\n";

			Function* closure = getBlock(func, func_name, result);

			func->addFunction(closure);

			return result;
		}
	};

	class YDWERegisterTriggerMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWERegisterTriggerMultiple)

			virtual uint32_t getCrossDomainIndex() override { return 1; }

		virtual bool isCrossDomain() override { return false; }

		virtual std::string getHandleName() override {
			if (getCurrentGroupId() <= getCrossDomainIndex()) {
				return "ydl_trigger";
			}
			return "GetTriggeringTrigger()";
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_trigger", "trigger", std::string(), false);

			std::string result;

			std::string func_name = getFuncName() + "T";

			result += func->getSpaces() + "set ydl_trigger = " + params[0]->toString(func) + "\n";
			result += func->getSpaces() + "call TriggerAddCondition( ydl_trigger, Condition(function " + func_name + ")\n";

			Function* closure = getBlock(func, func_name, result);

			func->addFunction(closure);

			return result;
		}
	};


	class DzTriggerRegisterMouseEventMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(DzTriggerRegisterMouseEventMultiple)

			virtual uint32_t getCrossDomainIndex() override { return 0; }

		virtual bool isCrossDomain() override { return true; }

		virtual std::string getUpvalueScriptName(UPVALUE_TYPE type) override {
			switch (type) {
			case mh::Node::UPVALUE_TYPE::SET_LOCAL:
				return "YDLocal6Set";
			case mh::Node::UPVALUE_TYPE::GET_LOCAL:
				return "YDLocal6Get";
			case mh::Node::UPVALUE_TYPE::SET_ARRAY:
				return "YDLocal6ArraySet";
			case mh::Node::UPVALUE_TYPE::GET_ARRAY:
				return "YDLocal6ArrayGet";
			default:
				break;
			}
			return std::string();
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			std::string action_name = *getName();
			action_name = action_name.replace(action_name.find("Multiple"), -1, "ByCode");

			std::string result;

			std::string func_name = getFuncName() + "T";

			result += func->getSpaces() + "if GetLocalPlayer() == " + params[0]->toString(func) + " then\n";
			result += getScript(func, action_name, func_name, params);
			result += func->getSpaces() + "endif\n";

			Function* closure = getBlock(func, func_name, result);
			func->addFunction(closure);

			return result;
		}

		virtual std::string getScript(TriggerFunction* func, const std::string& name, const std::string func_name, const std::vector<NodePtr>& params) {

			std::string result;
			std::vector<std::string> args;

			switch (m_nameId) {
			case "DzTriggerRegisterMouseEventMultiple"s_hash:
			case "DzTriggerRegisterKeyEventMultiple"s_hash:
				args = { "null", params[2]->toString(func), params[1]->toString(func), "false", "function " + func_name };
				break;
			case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
			case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
			case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
				args = { "null", "false", "function " + func_name };
				break;
			case "DzFrameSetUpdateCallbackMultiple"s_hash:
				args = { "function " + func_name };
				break;
			case "DzFrameSetScriptMultiple"s_hash:
				args = { params[2]->toString(func), params[1]->toString(func), "false" };
				break;
			default:
				break;
			}
			result += func->getSpaces(1) + "call " + name + "( ";
			for (size_t i = 0; i < args.size(); i++) {
				auto& arg = args[i];
				result += arg;
				if (i < args.size() - 1) {
					result += ", ";
				}
			}
			result += "\n";
			return  result;
		}
	};
}
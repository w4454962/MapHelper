#pragma once


namespace mh {


	typedef std::shared_ptr<class SigleNodeClosure> SigleNodeClosurePtr;
	//闭包
	class SigleNodeClosure : public ClosureNode {
		REGISTER_FROM_CLOSUER(SigleNodeClosure)
	public:

		SigleNodeClosure(NodePtr node, const std::string& return_type, NodePtr parent)
			:SigleNodeClosure((Action*)node->getData(), 0, node)
		{
			m_node = node;
			m_return_type = return_type;
		}

		//没有逆天参数 所以-1
		virtual int getCrossDomainIndex() { return -1; }


		virtual std::vector<NodePtr> getChildList() {
			std::vector<NodePtr> list;

			NodePtr node = shared_from_this();

			//为当前闭包添加一条动作
			NodePtr fake = NodeFromAction(m_action, 0, node);

			list.push_back(fake);
			return list;
		}
		virtual std::string toString(TriggerFunction* func) override {

			auto& editor = get_trigger_editor();
			// script_name
			std::string name = editor.getScriptName(m_action);
			std::string func_name = getParentNode()->getFuncName();

			std::string result;

			//将动作编译到函数里 返回函数名
			result = "function " + func_name;

			std::string upvalues;
			std::vector<std::string> actions;

			FunctionPtr code = getBlock(func, func_name, m_return_type, upvalues, actions);

			func->addFunction(code);

			return result;
		}

		virtual std::string getUpvalue(const Upvalue& info) override {
			auto ptr = getParentNode();

			
			NodePtr last_closure = nullptr;
			getValue([&](NodePtr ptr) {
				if (ptr.get() != this && ptr->getType() == TYPE::CLOSURE) {
					auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
					if (node->params_finish) {
						last_closure = ptr;
						return true;
					}
				}
				return false;
			});

			if (last_closure) {
				return last_closure->getUpvalue(info);
			} 

			std::string result;

			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocal2Set({}, \"{}\", {})", info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocal2Get({}, \"{}\")", info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocal2ArraySet({}, \"{}\", {}, {})", info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocal2ArrayGet({}, \"{}\", {})", info.type, info.name, info.index);
				break;
			default:
				break;
			}

			return result;
		}

	private:
		NodePtr m_node;
		std::string m_return_type;
	};


	class ForForceMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(ForForceMultiple)

		//没有逆天参数 所以-1
		virtual int getCrossDomainIndex() { return -1; }

		virtual std::string toString(TriggerFunction* func) override {

			auto& editor = get_trigger_editor();
			// script_name
			std::string name = editor.getScriptName(m_action);
			std::string func_name = getFuncName() + "A";

			std::string result = func->getSpaces() + "call " + name + "(";

			

			params_finish = false;

			for (auto& param : getParameterList()) {
				result += " " + param->toString(func);
				result += ",";
			}
			result += "function " + func_name + ")\n";

			params_finish = true;

			std::string upvalues;
			std::vector<std::string> upactions;
			FunctionPtr code = getBlock(func, func_name, "nothing", upvalues, upactions);

			for (auto& action : upactions) {
				result += action;
			}

			func->addFunction(code);
			return result;
		}

		virtual std::string getUpvalue(const Upvalue& info) override {

			if (!params_finish) { //如果是参数里的动作 就让他们访问上一级
				return getParentNode()->getUpvalue(info);
			}

			NodePtr last_closure = nullptr;
			getValue([&](NodePtr ptr) {
				if (ptr.get() != this && ptr->getType() == TYPE::CLOSURE) {
					last_closure = ptr;
					return true;
				}
				return false;
			});
			if (last_closure) { //如果有上一层闭包  则使用上一层闭包里的环境  如果没有 则是在根触发里 就使用下面的规则
				return last_closure->getUpvalue(info);
			} else if (info.is_func){
				//如果是根触发  并且是要获取函数值 则直接使用根触发里的
				return getRootNode()->getUpvalue(info);
			}

			std::string result;
			

			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocal2Set({}, \"{}\", {})", info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocal2Get({}, \"{}\")", info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocal2ArraySet({}, \"{}\", {}, {})", info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocal2ArrayGet({}, \"{}\", {})", info.type, info.name, info.index);
				break;
			default:
				break;
			}
			return result;
		}

	};


	class YDWEExecuteTriggerMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWEExecuteTriggerMultiple)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_triggerstep", "integer");
			func->current()->addLocal("ydl_trigger", "trigger", std::string(), false);

			std::string result;

			params_finish = false; 
			result += func->getSpaces() + "set ydl_trigger = " + params[0]->toString(func) + "\n";
			result += func->getSpaces() + "YDLocalExecuteTrigger(ydl_trigger)\n";

			params_finish = true;
			for (auto& node : getChildList()) {
				result += node->toString(func);
			}
			result += func->getSpaces() + "call YDTriggerExecuteTrigger(ydl_trigger, " + params[1]->toString(func) + ")\n";

			return result;
		}


		virtual std::string getUpvalue(const Upvalue& info) override {
			if (!params_finish) { //如果是参数里的动作 就让他们访问上一级
				return getParentNode()->getUpvalue(info);
			}

			std::string result;

			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocal5Set({}, \"{}\", {})", info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = getParentNode()->getUpvalue(info);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocal5ArraySet({}, \"{}\", {}, {})", info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = getParentNode()->getUpvalue(info);
				break;
			default:
				break;
			}
			return result;
		}
	};


	class YDWETimerStartMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWETimerStartMultiple)

		virtual int getCrossDomainIndex() override { return 0; }

		//是否自动传递逆天局部变量  类似闭包里跨域引用
		virtual bool isVariableCrossDomain() { return true; }

		//是否自动传递 获取触发单位 获取触发玩家 这些函数值
		virtual bool isFunctionCrossDomain() { return true; }

		std::string getHandleName() {
			if (getCurrentGroupId() <= getCrossDomainIndex()) {
				return "ydl_timer";
			}
			return "GetExpiredTimer()";
		}

		virtual std::string getUpvalue(const Upvalue& info) override {
			if (!params_finish) { //如果是参数里的动作 就让他们访问上一级
				return getParentNode()->getUpvalue(info);
			}

			std::string result;

			bool is_get = info.uptype == Upvalue::TYPE::GET_LOCAL || info.uptype == Upvalue::TYPE::GET_ARRAY;
			if (getCrossDomainIndex() == getCurrentGroupId() && is_get) {	//如果当前是传参区 则使用上一层的局部变量
				result = getParentNode()->getUpvalue(info);
				return result;
			}

			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocalSet({}, {}, \"{}\", {})", getHandleName(), info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocalGet({}, {}, \"{}\")", getHandleName(), info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocalArraySet({}, {}, \"{}\", {}, {})", getHandleName(), info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocalArrayGet({}, {}, \"{}\", {})", getHandleName(), info.type, info.name, info.index);
				break;
			default:
				break;
			}
			return result;
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_timer", "timer", std::string(), false);

			std::string result;
			std::string save_state; 
			std::string start;

			std::string func_name = getFuncName() + "T";
			params_finish = false; 

			result = func->getSpaces() + "set ydl_timer = " + params[0]->toString(func) + "\n";
			start = func->getSpaces() + "call TimerStart(ydl_timer, "
				+ params[1]->toString(func) + ", "
				+ params[2]->toString(func)
				+ ", function " + func_name + ")\n";

			params_finish = true;

			std::vector<std::string> upactions(getCrossDomainIndex() + 1);

			FunctionPtr closure = getBlock(func, func_name, "nothing", save_state, upactions);
			result += upactions[0]; //参数区
			result += save_state;	//自动传参
			result += start;		//开始计时器

			func->addFunction(closure);

			return result;
		}


	};

	class YDWERegisterTriggerMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(YDWERegisterTriggerMultiple)

		virtual int getCrossDomainIndex() override { return 1; }

		//是否自动传递逆天局部变量  类似闭包里跨域引用
		virtual bool isVariableCrossDomain() { return true; }

		//是否自动传递 获取触发单位 获取触发玩家 这些函数值
		virtual bool isFunctionCrossDomain() { return false; }

		std::string getHandleName() {
			if (getCurrentGroupId() <= getCrossDomainIndex()) {
				return "ydl_trigger";
			}
			return "GetTriggeringTrigger()";
		}

		virtual std::string getUpvalue(const Upvalue& info) override {
			std::string result;

			if (!params_finish) { //如果是参数里的动作 就让他们访问上一级
				return getParentNode()->getUpvalue(info);
			}

			bool is_get = info.uptype == Upvalue::TYPE::GET_LOCAL || info.uptype == Upvalue::TYPE::GET_ARRAY;

			//如果当前是传参区 则使用上一层的局部变量
			bool is_param_block = (getCrossDomainIndex() == getCurrentGroupId() && is_get);

			//事件也使用上一层的变量
			bool is_event_block = getCurrentGroupId() < getCrossDomainIndex();

			//如果是函数值 并且当前不允许函数值跨域 也使用上一层
			bool is_func = info.is_func && !isFunctionCrossDomain();

			if (is_param_block || is_event_block || is_func) {
				result = getParentNode()->getUpvalue(info);
				return result;
			}
		
			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocalSet({}, {}, \"{}\", {})", getHandleName(), info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocalGet({}, {}, \"{}\")", getHandleName(), info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocalArraySet({}, {}, \"{}\", {}, {})", getHandleName(), info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocalArrayGet({}, {}, \"{}\", {})", getHandleName(), info.type, info.name, info.index);
				break;
			default:
				break;
			}
			return result;
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//添加局部变量
			func->current()->addLocal("ydl_trigger", "trigger", std::string(), false);

			std::string result;
			std::string save_state;

			std::string func_name = getFuncName() + "Conditions";
	
			params_finish = false;

			result += func->getSpaces() + "set ydl_trigger = " + params[0]->toString(func) + "\n";

			params_finish = true;

			std::vector<std::string> upactions(getCrossDomainIndex() + 1);

			FunctionPtr closure = getBlock(func, func_name, "nothing", save_state, upactions);
			result += upactions[1]; //参数区
			result += save_state;	//自动传参代码
			result += upactions[0];	//事件区
			result += func->getSpaces() + "call TriggerAddCondition( ydl_trigger, Condition(function " + func_name + "))\n";


			func->addFunction(closure);

			return result;
		}

	};


	class DzTriggerRegisterMouseEventMultiple : public ClosureNode {
	public:
		REGISTER_FROM_CLOSUER(DzTriggerRegisterMouseEventMultiple)

		virtual int getCrossDomainIndex() override { return 0; }

		//是否自动传递逆天局部变量  类似闭包里跨域引用
		virtual bool isVariableCrossDomain() { return true; }

		//是否自动传递 获取触发单位 获取触发玩家 这些函数值
		virtual bool isFunctionCrossDomain() { return false; }

		virtual std::string getUpvalue(const Upvalue& info) override {
			std::string result;


			if (!params_finish) { //如果是参数里的动作 就让他们访问上一级
				return getParentNode()->getUpvalue(info);
			}

			bool is_get = info.uptype == Upvalue::TYPE::GET_LOCAL || info.uptype == Upvalue::TYPE::GET_ARRAY;
			if ((getCrossDomainIndex() == getCurrentGroupId() && is_get)|| !params_finish) {	//如果当前是传参区 则使用上一层的局部变量
				result = getParentNode()->getUpvalue(info);
				return result;
			}
	
			std::string func_name = getFuncName();

			switch (info.uptype) {
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocal6Set(\"{}\", {}, \"{}\", {})", func_name, info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocal6Get(\"{}\", {}, \"{}\")", func_name, info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocal6ArraySet(\"{}\", {}, \"{}\", {}, {})", func_name, info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocal6ArrayGet(\"{}\", {}, \"{}\", {})", func_name, info.type, info.name, info.index);
				break;
			default:
				break;
			}
			return result;
		}

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			std::string action_name = getName();
			action_name = action_name.replace(action_name.find("Multiple"), -1, "ByCode");

			std::string result;

			params_finish = false;

			result += func->getSpaces() + "if GetLocalPlayer() == " + params[0]->toString(func) + " then\n";
			func->addSpace();
			result += getScript(func, action_name, params);
			func->subSpace();
			result += func->getSpaces() + "endif\n";


			std::string upvalues;

			params_finish = true;

			std::vector<std::string> upactions(getCrossDomainIndex() + 1);

			FunctionPtr closure = getBlock(func, m_function, "nothing", upvalues, upactions);
			func->addFunction(closure);

			return upvalues + result;
		}

		virtual std::string getScript(TriggerFunction* func, const std::string& name, const std::vector<NodePtr>& params) {

			std::string result;
			std::vector<std::string> args;

			std::string key = "T";
			switch (m_nameId) {
			case "DzTriggerRegisterMouseEventMultiple"s_hash:
				key = "MT";  break;
			case "DzTriggerRegisterKeyEventMultiple"s_hash:
				key = "KT";  break;
			case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
				key = "WT";  break;
			case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
				key = "MMT";  break;
			case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
				key = "WRT";  break;
			case "DzFrameSetUpdateCallbackMultiple"s_hash:
				key = "CT";  break;
				break;
			case "DzFrameSetScriptMultiple"s_hash:
				key = "FT";  break;
			default:
				break;
			}
			m_function = getFuncName() + key;

			switch (m_nameId) {
			case "DzTriggerRegisterMouseEventMultiple"s_hash:
			case "DzTriggerRegisterKeyEventMultiple"s_hash:
				args = { "null", params[2]->toString(func), params[1]->toString(func), "false", "function " + m_function };
				break;
			case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
			case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
			case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
				args = { "null", "false", "function " + m_function };
				break;
			case "DzFrameSetUpdateCallbackMultiple"s_hash:
				args = { "function " + m_function };
				break;
			case "DzFrameSetScriptMultiple"s_hash:
				args = { params[2]->toString(func), params[1]->toString(func), "function " + m_function, "false"};
				break;
			default:
				break;
			}
			result += func->getSpaces() + "call " + name + "( ";
			for (size_t i = 0; i < args.size(); i++) {
				auto& arg = args[i];
				result += arg;
				if (i < args.size() - 1) {
					result += ", ";
				}
			}
			result += ")\n";
			return  result;
		}

	private:
		std::string m_function;

	};
}
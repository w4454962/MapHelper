#pragma once
#include "stdafx.h"

#include <memory>
#include <functional>
#include "WorldEditor.h"
#include "TriggerEditor.h"

#include "Node.h"
#include "Nodes/TriggerNode.hpp"
#include "Nodes/ActionNode.hpp"
#include "Nodes/Parameter.hpp"

namespace mh {

	extern std::unordered_map<std::string, MakeNode> MakeNodeMap;


	class CommentString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CommentString, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			NodePtr node = shared_from_this();
			NodePtr param = NodeFramParameter(m_action->parameters[0], 0, node);

			return toLine("//" + param->toString(pre_actions));
		}
	};

	class CustomScriptCode : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CustomScriptCode, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			return toLine(m_action->parameters[0]->value);
		}
	};
	
	class GetTriggerName : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetTriggerName, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			return "\"" + *getRootNode()->getName() + "\"";
		}
	};
	
	class OperatorString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorString, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			auto list = getParameterList();
			return "(" + list[0]->toString(pre_actions) + " + " + list[1]->toString(pre_actions) + ")";
		}
	};
	
	
	class ForLoopA : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopA, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			std::string result;
	
			auto params = getParameterList();
	
			std::string variable = m_nameId == "ForLoopA"s_hash ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		
			result += Spaces() + "set " + variable + " = " + params[0]->toString(pre_actions) + "\n";
			result += Spaces() + "loop\n"; 
			SpaceAdd();
	
			result += Spaces() + "exitwhen " + variable + ">" + params[1]->toString(pre_actions) + "\n";
			result += Spaces() + params[2]->toString(pre_actions) + "\n";
			result += Spaces() + "set " + variable + " = " + variable + " + 1\n";
			SpaceRemove();
			result += Spaces() + "endloop\n"; 
	
			return result;
		}
	};
	
	
	class ForLoopVar : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopVar, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			std::string result;
	
			auto params = getParameterList();
	
			std::string variable = params[0]->toString(pre_actions);
	
			result += Spaces() + "set " + variable + " = " + params[1]->toString(pre_actions) + "\n";
			result += Spaces() + "loop\n";
			SpaceAdd();
	
			result += Spaces() + "exitwhen " + variable + ">" + params[2]->toString(pre_actions) + "\n";
			result += Spaces() + params[3]->toString(pre_actions) + "\n";
			result += Spaces() + "set " + variable + " = " + variable + " + 1\n";
			result += Spaces() + "endloop\n";
			SpaceRemove();
	
			return result;
		}
	};
	
	class IfThenElse : public ActionNode {
	public:
		REGISTER_FROM_ACTION(IfThenElse, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			std::string result;
			auto params = getParameterList();
			result += Spaces() + "if (" + params[0]->toString(pre_actions) + ") then\n";
			result += Spaces(1) + params[1]->toString(pre_actions) + "\n";
			result += Spaces() + "else\n";
			result += Spaces(1) + params[2]->toString(pre_actions) + "\n";
			result += Spaces() + "endif\n";
			return result;
		}
	};

	class GetBooleanAnd : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetBooleanAnd, Action*)

			virtual std::string toString(std::string& pre_actions) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(pre_actions) + " and " + params[1]->toString(pre_actions) + ")";
		}
	};

	class GetBooleanOr : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetBooleanOr, Action*)

			virtual std::string toString(std::string& pre_actions) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(pre_actions) + " or " + params[1]->toString(pre_actions) + ")";
		}
	};

	class OperatorInt : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorInt, Action*)

			virtual std::string toString(std::string& pre_actions) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(pre_actions) + " " + params[1]->toString(pre_actions) + " " + params[2]->toString(pre_actions) + ")";
		}
	};


	class OperatorCompare : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorCompare, Action*)

			virtual std::string toString(std::string& pre_actions) override {
			auto params = getParameterList();

			return params[0]->toString(pre_actions) + " " + params[1]->toString(pre_actions) + " " + params[2]->toString(pre_actions);
		}
	};
	



	std::unordered_map<std::string, MakeNode> MakeNodeMap = {
		{"CommentString",		CommentString::From},
		{"CustomScriptCode",	CustomScriptCode::From},
		{"GetTriggerName",		GetTriggerName::From},
		{"OperatorString",		OperatorString::From},

		{"ForLoopA",			ForLoopA::From},
		{"ForLoopB",			ForLoopA::From}, //跟A一样

		{"ForLoopVar",			ForLoopVar::From},
		{"IfThenElse",			IfThenElse::From},
		{"GetBooleanAnd",		GetBooleanAnd::From},
		{"GetBooleanOr",		GetBooleanOr::From},

		{"OperatorInt",			OperatorInt::From},
		{"OperatorReal",		OperatorInt::From}, //跟int一样

		{"OperatorCompare",		OperatorCompare::From},

	};




	NodePtr NodeFromTrigger(Trigger* trigger) {
		return TriggerNode::From(trigger, -1, nullptr);
	}

	NodePtr NodeFromAction(Action* action, uint32_t childId, NodePtr parent) {
		std::string name = action->name;
		auto it = MakeNodeMap.find(name); //有预置类型的
		if (it != MakeNodeMap.end()) {
			return it->second(action, childId, parent);
		}

		if (name.substr(0, 15) == "OperatorCompare") { //各种比较的操作符
			return OperatorCompare::From(action, childId, parent);
		}
		return ActionNode::From(action, childId, parent);
	}

	NodePtr NodeFramParameter(Parameter* parameter, uint32_t index, NodePtr parent) {
		return ParameterNode::From(parameter, index, parent);
	}

	std::string CodeConnent(Codes& codes) {
		std::string code;
		for (auto& s : codes) {
			code += s;
		}
		return code;
	}

	
	int space_stack = 0;

	void SpaceAdd() {
		space_stack = space_stack + 1;
	}

	void SpaceRemove() {
		space_stack = space_stack - 1;
	}

	std::string& Spaces(int offset) {
		auto& editor = get_trigger_editor();

		int index = max(0, space_stack - offset);
		return editor.spaces[index];
	}
}
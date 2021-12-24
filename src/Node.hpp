#pragma once
#include "stdafx.h"

#include <memory>
#include <functional>
#include "WorldEditor.h"
#include "TriggerEditor.h"
#include "Node.h"

int __fastcall fakeGetChildCount(Action* action);
Action::Type get_action_type(Action* action);


namespace mh {

	extern std::unordered_map<std::string, MakeNode> MakeNodeMap;

	class TriggerNode : public Node{
	public:
		static NodePtr From(void* trigger, uint32_t childId, NodePtr parent) {
			return NodePtr(new TriggerNode((Trigger*)trigger));
		}

		 virtual void* getData() override { return m_trigger; }

		 virtual StringPtr getName() override { return m_name; }

		 virtual uint32_t getNameId() override { return m_nameId; }

		 virtual TYPE getType() override { return TYPE::ROOT; }

		 virtual NodePtr getParentNode() override { return nullptr; }

		 virtual NodePtr getRootNode() override { return shared_from_this(); }

		 virtual std::vector<NodePtr> getChildList() override {
			 std::vector<NodePtr> list;

			 NodePtr node = shared_from_this();

			 if (m_trigger->line_count > 0 && m_trigger->actions) {
				 for (uint32_t i = 0; i < m_trigger->line_count; i++)
				 {
					 Action* action = m_trigger->actions[i];
					 if (!action->enable || action->group_id != -1) //被禁用时 或者 不属于根节点的动作时跳过
						 continue;
					 NodePtr child = NodeFromAction(action, -1, node);
					 list.push_back(child);
				 }
			 }
			 return list;
		 }

		virtual bool getValue(Codes& result, const NodeFilter& filter) override {
			auto list = getChildList();

			std::string& pre_actions = result[3];

			for (auto& node : list) {
				Action* action = (Action*)node->getData();
				auto type = get_action_type(action);
				if (type != Action::Type::none) {
					result[type] += node->toString(pre_actions);
				}
			}
			return true;
		}

		virtual std::string toString(std::string& pre_actions) override {
			Codes result(4);  //分别是 事件,条件，动作， 预生成代码。
			
			auto res = getValue(result, NodeFilter());

			return CodeConnent(result);
		}

	private:
		TriggerNode(Trigger* trigger) {
			m_trigger = trigger;
			m_name = StringPtr(new std::string(trigger->name));
			m_nameId = hash_(trigger->name);
		}
	protected:
		Trigger* m_trigger;
		StringPtr m_name;
		uint32_t m_nameId;
	};

//生成一个静态方法
#define REGISTER_FROM(name, type) static NodePtr From(void* param, uint32_t index, NodePtr parent) { return NodePtr(new name((type)param, index, parent)); } 

//生成一个静态方法 + 基于ActionNode的构造方法
#define REGISTER_FROM_ACTION(name, type) REGISTER_FROM(name, type); name(type param, uint32_t index, NodePtr parent): ActionNode(param, index, parent) { } ;


	class ActionNode : public Node {
	public:
		REGISTER_FROM(ActionNode, Action*)

		virtual void* getData() override { return m_action; }

		virtual StringPtr getName() override { return m_name; }

		virtual uint32_t getNameId() override { return m_nameId; }
		
		virtual TYPE getType() override { return m_type; }

		virtual NodePtr getParentNode() override { return m_parent; }

		virtual NodePtr getRootNode() override { return m_root; }

		virtual std::vector<NodePtr> getChildList() override {
			std::vector<NodePtr> list;

			if (m_action->child_count > 0 && m_action->child_actions) {

				NodePtr node = shared_from_this();

				for (uint32_t i = 0; i < m_action->child_count; i++)
				{
					Action* action = m_action->child_actions[i];

					if (!action->enable || !((int)action->group_id < fakeGetChildCount(m_action)))
						continue;
					NodePtr child = NodeFromAction(action, action->group_id, node);

					list.push_back(child);
				}
			}

			return list;
		}

		std::vector<NodePtr> getParameterList() {
			std::vector<NodePtr> list;


			NodePtr node = shared_from_this();

			uint32_t size = m_action->param_count;
			if (size > 0 && m_action->parameters) {
				for (uint32_t i = 0; i < size; i++) {
					Parameter* parameter = m_action->parameters[i];
					NodePtr param = NodeFramParameter(parameter, i, node);
					list.push_back(param);
				}
			}
			return list;
		}

		virtual bool getValue(Codes& result, const NodeFilter& filter) override {

			NodePtr node = shared_from_this();
			if (filter(node)) {
				return true;

			} else if (m_parent->getType() != TYPE::ROOT) { //向上传递

				return m_parent->getValue(result, filter);
			}

			return true;
		}

		virtual std::string toString(std::string& pre_actions) override {
			Codes code(2);

			if (getType() == TYPE::CALL) {
				code[0] = Spaces();
				code[1] = "call ";
			}

			NodePtr node = shared_from_this();

			bool res = getValue(code, [&](NodePtr prev_node) {
				code[1] += *node->getName();
				code[1] += "(";
				auto list = getParameterList();
				uint32_t size = list.size();
				for(uint32_t i =0; i <size; i++) {
					auto& param = list[i];
					code[1] += param->toString(pre_actions);
					if (i < size - 1) {
						code[1] += ", ";
					}
				}
				code[1] += ")";
				return true;
			});
			if (getType() == TYPE::CALL) {
				code[1] = "\n";
			}
			return CodeConnent(code);
		}

		std::string toCode(const std::string& result) {
			if (getType() == TYPE::CALL) {
				return Spaces() + result + "\n";
			}
			return result;
		}

		ActionNode(Action* action, uint32_t childId, NodePtr parent) {
			m_action = action;

			m_parent = parent;

			m_childId = childId;
			if (parent->getType() == TYPE::PARAM || parent->getType() == TYPE::GET) {
				m_type == TYPE::GET;
			} else {
				m_type = TYPE::CALL;
			}
			
			m_name = StringPtr(new std::string(action->name));
			m_nameId = hash_(action->name);
			if (parent->getType() == TYPE::ROOT) {
				m_root = parent;
			} else {
				m_root = parent->getRootNode();
			}
		}

	protected:
		Action* m_action;
		TYPE m_type;

		NodePtr m_root;
		NodePtr m_parent;

		StringPtr m_name;
		uint32_t m_nameId;

		uint32_t m_childId; //自身作为子动作的id
		uint32_t m_childCount;//拥有的子动作id上限
	};



	class ParameterNode : public Node {
	public:
		REGISTER_FROM(ParameterNode, Parameter*)

		virtual void* getData() override { return m_parameter; }

		virtual StringPtr getName() override { return m_name; }

		virtual uint32_t getNameId() override { return m_nameId; }

		virtual TYPE getType() override { return TYPE::PARAM; }

		virtual NodePtr getParentNode() override { return m_parent; }

		virtual NodePtr getRootNode() override { return m_root; }

		virtual std::vector<NodePtr> getChildList() override {
			std::vector<NodePtr> list;

			if (m_parameter->funcParam) {

				NodePtr node = shared_from_this();

				NodePtr child = NodeFromAction(m_parameter->funcParam, 0, node);
				list.push_back(child);
			}
			return list;
		}

		virtual bool getValue(Codes& result, const NodeFilter& filter) override {

			NodePtr node = shared_from_this();
			if (filter(node)) {
				return true;
			}
			else if (m_parent->getType() != TYPE::ROOT) { //向上传递
				return m_parent->getValue(result, filter);
			}

			return true;
		}

		virtual std::string toString(std::string& pre_actions) override {
	;
			NodePtr node = shared_from_this();

			auto& editor = get_trigger_editor();
			auto& world = get_world_editor();

			auto value = std::string(m_parameter->value);

			switch (m_parameter->typeId)
			{
			case Parameter::Type::invalid:
				return "";

			case Parameter::Type::preset: //预设值
			{
				const auto preset_type = world.getConfigData("TriggerParams", value, 1);
				const auto preset_value = world.getConfigData("TriggerParams", value, 2);
				bool has_quote_symbol = preset_value.find('`') == 0;

				if (editor.getBaseType(preset_type) == "string") {
					auto result = world.getConfigData("TriggerParams", value, 2);
					if (has_quote_symbol) { //如果是未定义类型的 并且是单引号开头的 替换成双引号
						result = string_replaced(result, "`", "\"");
					}
					return result;
				}
				return world.getConfigData("TriggerParams", value, 2);
			}
			case Parameter::Type::variable:
			{
				auto result = value;

				if (!result._Starts_with("gg_")) {
					result = "udg_" + result;
				}
				//if (value == "Armagedontimerwindow") //不知道是什么
				//{
				//	puts("s");
				//}
				if (m_parameter->arrayParam) {
					NodePtr array_node = NodeFramParameter(m_parameter->arrayParam, 0, node);
					result += "[" + array_node->toString(pre_actions) + "]";
				}
				return result;
			}

			case Parameter::Type::function:
			{
				auto list = getChildList();
				if (list.size() > 0) {
					return list[0]->toString(pre_actions);
				}
				return value + "()";
			}

			case Parameter::Type::string:
			{
				auto type{ std::string(m_parameter->type_name) };

				switch (hash_(type.c_str()))
				{
				case "scriptcode"s_hash:
					return value;
				case "abilcode"s_hash:
				case "heroskillcode"s_hash:
				case "buffcode"s_hash:
				case "destructablecode"s_hash:
				case "itemcode"s_hash:
					//case "ordercode"s_hash:
				case "techcode"s_hash:
				case "doodadcode"s_hash:// 处理下装饰物没有判断的问题
				case "unitcode"s_hash:
					return "'" + value + "'";
				default:
				{
					uint32_t is_import_path = 0;
					auto* type_data = editor.getTypeData(type);
					if (type_data) {
						is_import_path = type_data->is_import_path;
					}
					if (type == "HotKey" && value == "HotKeyNull") {
						return "0";
					}
					if (is_import_path || editor.getBaseType(type) == "string") {
						value = string_replaced(value, "\\", "\\\\");
						return "\"" + string_replaced(value, "\"", "\\\"") + "\"";
					}
					return value;
				}
				}
			}
			default:
				break;
			}
		}

	private:
		ParameterNode(Parameter* parameter, uint32_t index, NodePtr parent) {
			m_parameter = parameter;

			m_parent = parent;

			m_index = index;

			m_action = parameter->funcParam;
			m_name = StringPtr(new std::string());
			m_nameId = 0;
			
			if (parent->getType() == TYPE::ROOT) {
				m_root = parent;
			}
			else {
				m_root = parent->getRootNode();
			}
		}

	protected:
		Parameter* m_parameter;
		Action* m_action;

		NodePtr m_root;
		NodePtr m_parent;

		StringPtr m_name;
		uint32_t m_nameId;
		uint32_t m_index;
	};


	class CommentString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CommentString, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			NodePtr node = shared_from_this();
			NodePtr param = NodeFramParameter(m_action->parameters[0], 0, node);

			return toCode("//" + param->toString(pre_actions));
		}
	};

	class CustomScriptCode : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CustomScriptCode, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			return toCode(m_action->parameters[0]->value);
		}
	};
	
	class GetTriggerName : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetTriggerName, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			return toCode("\"" + *getRootNode()->getName() + "\"");
		}
	};
	
	class OperatorString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorString, Action*)

		virtual std::string toString(std::string& pre_actions) override {
			auto list = getParameterList();
			return toCode("(" + list[0]->toString(pre_actions) + " + " + list[1]->toString(pre_actions) + ")");
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
	
	class ForLoopB : public ForLoopA {
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


	std::unordered_map<std::string, MakeNode> MakeNodeMap = {
		{"CommentString",		CommentString::From},
		{"CustomScriptCode",	CustomScriptCode::From},
		{"GetTriggerName",		GetTriggerName::From},
		{"OperatorString",		OperatorString::From},
		{"ForLoopA",			ForLoopA::From},
		{"ForLoopB",			ForLoopB::From},
		{"ForLoopVar",			ForLoopVar::From},
		{"IfThenElse",			IfThenElse::From},
	};




	NodePtr NodeFromTrigger(Trigger* trigger) {
		return TriggerNode::From(trigger, -1, nullptr);
	}

	NodePtr NodeFromAction(Action* action, uint32_t childId, NodePtr parent) {
		std::string name = action->name;
		auto it = MakeNodeMap.find(name);
		if (it != MakeNodeMap.end()) {
			return it->second(action, childId, parent);
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
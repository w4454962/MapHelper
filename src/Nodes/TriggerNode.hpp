#pragma once

#include "Node.h"
#include "Function.hpp"
#include <set>

namespace mh {
	//触发器节点
	class TriggerNode : public Node {
	public:
		static NodePtr From(void* trigger, uint32_t childId, NodePtr parent) {
			return NodePtr(new TriggerNode((Trigger*)trigger));
		}

		virtual void* getData() override { return m_trigger; }

		virtual const std::string& getName() override { return m_name; }

		virtual uint32_t getNameId() override { return m_nameId; }

		virtual TYPE getType() override { return TYPE::ROOT; }

		virtual void setType(TYPE type) override { }

		virtual NodePtr getParentNode() override { return nullptr; }

		virtual NodePtr getRootNode() override { return shared_from_this(); }

		virtual std::vector<NodePtr> getChildList() override {
			std::vector<NodePtr> list;

			NodePtr node = shared_from_this();

			if (m_trigger->line_count > 0 && m_trigger->actions) {
				for (uint32_t i = 0; i < m_trigger->line_count; i++) {
					Action* action = m_trigger->actions[i];
					if (!action->enable || action->group_id != -1) //被禁用时 或者 不属于根节点的动作时跳过
						continue;
					NodePtr child = NodeFromAction(action, i, node);
					list.push_back(child);
				}
			}
			return list;
		}

		virtual bool getValue(const NodeFilter& filter) override {
			return true;
		}

		virtual std::string toString(TriggerFunction* func) override {
		
	
			TriggerFunction tfunc(m_funcName, m_name);
			
			func = &tfunc;


			std::vector<std::vector<NodePtr>> nodes(4);
			
			for (auto& node : getChildList()) {
				Action* action = (Action*)node->getData();
				auto type = get_action_type(action);
				nodes[type].push_back(node);
			}

			//event
			{
				auto events = func->event;
				func->push(events);

				

				*events << "\tset " + m_trigger_variable_name + " = CreateTrigger()\n";
				if (m_trigger->is_disable_init) {
					*events << "\tcall DisableTrigger(" + m_trigger_variable_name + ")\n";
				}

				*events << "#ifdef DEBUG\n";
				*events << "\tcall YDWESaveTriggerName(" + m_trigger_variable_name + ",\"" + m_name + "\")\n";
				*events << "#endif\n";

				for (auto& node : nodes[Action::Type::event]) {
					*events << node->toString(func);
				}

				if (!nodes[Action::Type::condition].empty()) {
					*events << "\tcall TriggerAddCondition(" + m_trigger_variable_name + ", Condition(function " + func->condition->getName() + "))\n";
				}
				*events << "\tcall TriggerAddAction(" + m_trigger_variable_name + ", function " + func->action->getName() + ")\n";
				func->pop();
			}
			 
			//condition
			{
				auto conditions = func->condition;
				func->push(conditions);
				for (auto& node : nodes[Action::Type::condition]) {
					if (!conditions->isEmpty()) {
						*conditions << " and ";
					} else {
						*conditions << "\treturn ";
					}
					node->setType(TYPE::GET);
					*conditions << "(" + node->toString(func) + ")";
				}
				if (!conditions->isEmpty()) {
					*conditions << "\n";
				}
				func->pop();

				if (!conditions->isEmpty()) {
					func->addFunction(conditions);
				}
			}
			
			//action 
			{
				auto actions = func->action;
				func->push(actions);
				for (auto& node : nodes[Action::Type::action]) {
					*actions << node->toString(func);
				} 
				func->pop();
				//如果当前触发里有逆天变量的话 则在开头跟结束插入2段代码
				if (hasUpvalue) {
					actions->insert_begin += func->getSpaces() + "YDLocalInitialize()\n";
					actions->insert_end += func->getSpaces() + "call YDLocal1Release()\n";
				}
			} 

			auto& world = get_world_editor();
			
			//检查逆天变量类型 发出警告
			for (auto&& [name, map] : all_upvalue_map) {
				if (map.size() > 1) {
					std::string warning = std::format("Warning: 触发器[{}]: 逆天局部变量[loc_{}] 有多个类型，请尽快修复:\n", base::u2a(m_name), base::u2a(name));
					print(warning.c_str());
					for (auto&& [type, code] : map) {
						std::string type_name = world.getConfigData("TriggerTypes", type, 3);
						print("[%s] %s\n", base::u2a(type_name).c_str(), base::u2a(code).c_str());
					}
					print("\n");
				}
			}


			auto& editor = get_trigger_editor();

			editor.m_initFuncTable[func->event->getName()] = true;

			return func->toString();
		}


		virtual std::string getFuncName() override {
			return m_funcName;
		}

		virtual const std::string& getTriggerVariableName() override {
			return m_trigger_variable_name;
		}

		virtual std::string getUpvalue(const Upvalue& info) override {
			std::string result;

			//传递过来的是一个函数名 
			if (info.uptype == Upvalue::TYPE::GET_LOCAL && info.is_func) {
				return info.name + "()";
			}

			switch (info.uptype)
			{
			case Upvalue::TYPE::SET_LOCAL:
				result = std::format("YDLocal1Set({}, \"{}\", {})", info.type, info.name, info.value);
				break;
			case Upvalue::TYPE::GET_LOCAL:
				result = std::format("YDLocal1Get({}, \"{}\")", info.type, info.name);
				break;
			case Upvalue::TYPE::SET_ARRAY:
				result = std::format("YDLocal1ArraySet({}, \"{}\", {}, {})", info.type, info.name, info.index, info.value);
				break;
			case Upvalue::TYPE::GET_ARRAY:
				result = std::format("YDLocal1ArrayGet({}, \"{}\", {})", info.type, info.name, info.index);
				break;
			default:
				break;
			}
			return result;
		}
	public:
		bool hasUpvalue = false;

		// all_upvalue_map[name] = {type, action}
		std::map<std::string, std::map<std::string, std::string>> all_upvalue_map;

	private:
		TriggerNode(Trigger* trigger) {
			m_trigger = trigger;

			std::string name = trigger->name;
			convert_name(name);

			m_trigger_variable_name = "gg_trg_" + name;
			m_funcName = std::string("Trig_" + name);

			m_name = std::string(trigger->name);
			m_nameId = hash_(m_name.c_str());
		}
	protected:
		Trigger* m_trigger;
		std::string m_name;
		uint32_t m_nameId;

		std::string m_funcName;
		std::string m_trigger_variable_name;

	};

}
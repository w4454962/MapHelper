#pragma once

#include "Node.h"
#include "Function.hpp"

namespace mh {
	//触发器节点
	class TriggerNode : public Node {
	public:
		static NodePtr From(void* trigger, uint32_t childId, NodePtr parent) {
			return NodePtr(new TriggerNode((Trigger*)trigger));
		}

		virtual void* getData() override { return m_trigger; }

		virtual StringPtr getName() override { return m_name; }

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
		
	
			TriggerFunction tfunc(*m_funcName, *m_name);
			
			func = &tfunc;


			std::vector<std::vector<NodePtr>> nodes(4);
			
			for (auto& node : getChildList()) {
				Action* action = (Action*)node->getData();
				auto type = get_action_type(action);
				nodes[type].push_back(node);
			}

			//event
			{
				auto& events = func->event;
				func->push(&events);

				
				events << "\tset " + m_trigger_variable_name + " = CreateTrigger()\n";
				events << "#ifdef DEBUG\n";
				events << "\tcall YDWESaveTriggerName(" + m_trigger_variable_name + ",\"" + *m_name + "\")\n";
				events << "#endif\n";

				if (m_trigger->is_disable_init) {
					events << "\tcall DisableTrigger(" + m_trigger_variable_name + ")\n";
				}
				for (auto& node : nodes[Action::Type::event]) {
					events << node->toString(func);
				}

				if (!nodes[Action::Type::condition].empty()) {
					events << "\tcall TriggerAddCondition(" + m_trigger_variable_name + ", Condition(function " + *m_funcName + "_Conditions))\n";
				}
				events << "\tcall TriggerAddAction(" + m_trigger_variable_name + ", function " + *m_funcName + "Actions)\n";
				func->pop();
			}
			 
			//condition
			{
				auto& conditions = func->condition;
				func->push(&conditions);
				for (auto& node : nodes[Action::Type::condition]) {
					if (!conditions.isEmpty()) {
						conditions << " and ";
					} else {
						conditions << "\treturn ";
					}
					conditions << "(" + node->toString(func) + ")";
				}
				if (!conditions.isEmpty()) {
					conditions << "\n";
				}
				func->pop();
			}
			
			//action 
			{
				auto& actions = func->action;
				for (auto& node : nodes[Action::Type::action]) {
					actions << node->toString(func);
				}
			}

			return func->toString();
		}


		virtual std::string getFuncName() override {
			return *m_funcName;
		}

		virtual const std::string& getTriggerVariableName() override {
			return m_trigger_variable_name;
		}

	
		virtual std::string getUpvalueScriptName(UPVALUE_TYPE type) override {
			switch (type) {
			case mh::Node::UPVALUE_TYPE::SET_LOCAL:
				return "YDLocal1Set";
			case mh::Node::UPVALUE_TYPE::GET_LOCAL:
				return "YDLocal1Get";
			case mh::Node::UPVALUE_TYPE::SET_ARRAY:
				return "YDLocal1ArraySet";
			case mh::Node::UPVALUE_TYPE::GET_ARRAY:
				return "YDLocal1ArrayGet";
			default:
				break;
			}
			return std::string();
		}

		virtual std::string getHandleName() override {
			return std::string();
		}


	public:
		//逆天局部变量表
		std::map<std::string, std::string> upvalue_map;

	private:
		TriggerNode(Trigger* trigger) {
			m_trigger = trigger;

			std::string name = trigger->name;
			convert_name(name);

			m_trigger_variable_name = "gg_trg_" + name;
			m_funcName = StringPtr(new std::string("Trig_" + name));

			m_name = StringPtr(new std::string(trigger->name));
			m_nameId = hash_(m_name->c_str());
		}
	protected:
		Trigger* m_trigger;
		StringPtr m_name;
		uint32_t m_nameId;

		StringPtr m_funcName;
		std::string m_trigger_variable_name;

	};

}
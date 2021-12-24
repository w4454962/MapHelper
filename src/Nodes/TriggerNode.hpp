#pragma once

#include "Node.h"

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
			Codes code(4);  //分别是 事件,条件，动作， 预生成代码。
			std::string& trigger_name = *m_name;

			std::string trigger_variable_name = "gg_trg_" + trigger_name;
			std::string trigger_action_name = "Trig_" + trigger_name + "Actions";

			//event 
			code[0] += "function InitTrig_" + trigger_name + " takes nothing returns nothing\n";
			code[0] += "\tset " + trigger_variable_name + " = CreateTrigger()\n";
			if (m_trigger->is_disable_init)
			{
				code[0] += "\tcall DisableTrigger(" + trigger_variable_name + ")\n";
			}
			SpaceAdd();


			auto res = getValue(code, NodeFilter());


			return CodeConnent(code);
		}

	private:
		TriggerNode(Trigger* trigger) {
			m_trigger = trigger;

			std::string name = trigger->name;
			convert_name(name);
			m_name = StringPtr(new std::string(name));
			m_nameId = hash_(name.c_str());
		}
	protected:
		Trigger* m_trigger;
		StringPtr m_name;
		uint32_t m_nameId;
	};

}
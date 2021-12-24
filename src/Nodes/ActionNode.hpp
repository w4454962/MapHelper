#pragma once


namespace mh {


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

			}
			else if (m_parent->getType() != TYPE::ROOT) { //向上传递

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
				for (uint32_t i = 0; i < size; i++) {
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

		std::string toLine(const std::string& result) {
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
				m_type = TYPE::GET;
			}
			else {
				m_type = TYPE::CALL;
			}

			m_name = StringPtr(new std::string(action->name));
			m_nameId = hash_(action->name);
			if (parent->getType() == TYPE::ROOT) {
				m_root = parent;
			}
			else {
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
}
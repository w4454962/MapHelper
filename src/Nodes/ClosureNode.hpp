#pragma once


namespace mh {

	//������Ҫ����������εĽڵ㶼Ӧ�ü̳�ClosureNode

	//����һ����̬���� + ����ClosureNode�Ĺ��췽��
#define REGISTER_FROM_CLOSUER(name) REGISTER_FROM(name,	Action*); name(Action* param, uint32_t index, NodePtr parent): ClosureNode(param, index, parent) { } ;

#define REGISTER_FROM_CLOSUER_CHILD(name, parent_class) REGISTER_FROM(name,	Action*); name(Action* param, uint32_t index, NodePtr parent): parent_class(param, index, parent) { } ;


	typedef std::shared_ptr<class ClosureNode> ClosureNodePtr;

	class ClosureNode : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ClosureNode)
	
		//�������������
		virtual uint32_t getCrossDomainIndex() { return 0; }

		//�Ƿ��Զ���������ֲ�����  ���Ʊհ����������
		virtual bool isVariableCrossDomain() {  return true; }

		//�Ƿ��Զ����� ��ȡ������λ ��ȡ������� ��Щ����ֵ
		virtual bool isFunctionCrossDomain() { return true; }

		virtual TYPE getType() override {  return TYPE::CLOSURE; }

		virtual std::string toString(TriggerFunction* func) override { 
			return std::string();
		}
	
		uint32_t getCurrentGroupId() { return m_current_group_id; }

		virtual Function* getBlock(TriggerFunction* func, const std::string& closure_name, std::string& upvalues) {
			std::vector<NodePtr> upvalues_scope; //���в������Ľڵ�
			std::vector<NodePtr> closure_scope;  //���ж������Ľڵ�

			std::map<std::string, std::string> search_upvalue_map;

			m_current_group_id = -1;

			//����
			for (auto& node : getChildList()) {
				Action* action = (Action*)node->getData();
				if (action->group_id <= getCrossDomainIndex()) {
					upvalues_scope.push_back(node);
				} else {
					closure_scope.push_back(node);
				}
			}

			//�����¼��Լ��������Ĵ��� �������������������ֲ�������
			for (auto& node : upvalues_scope) {
				Action* action = (Action*)node->getData();
				if (action->group_id == getCrossDomainIndex()) { //����ǲ�����
					uint32_t id = node->getNameId();
					if (id == "YDWESetAnyTypeLocalArray"s_hash or id == "YDWESetAnyTypeLocalVariable"s_hash) {
						std::string upvalue_name = action->parameters[1]->value;
						std::string upvalue_type = action->parameters[0]->value + 11;
						search_upvalue_map.emplace(upvalue_name, upvalue_type);
					}
				}

				m_current_group_id = action->group_id;
				upvalues += node->toString(func);
			}

			std::map<std::string, Upvalue>* prev_upvalue_map_ptr = nullptr;

			//�ҵ���һ��հ� ������ֲ�������
			getValue([&](const NodePtr ptr) {
				if (ptr.get() != this) {
					if (ptr->getType() == TYPE::CLOSURE) {
						auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
						prev_upvalue_map_ptr = &node->upvalue_map;
						return true;
					} 
				}
				return false;
			});
			

			Function* closure = new Function(closure_name, "nothing");
			func->push(closure);
			//���ɱհ�����
			for (auto& node : closure_scope) {
				m_current_group_id = ((Action*)node->getData())->group_id;
				*closure << node->toString(func);
			}
			func->pop();

			
			//���������ڲ�����������ֲ����� ������һ�㴦��
			for (auto& [n, t] : search_upvalue_map) {
				upvalue_map.erase(n);
				if (prev_upvalue_map_ptr) {
					prev_upvalue_map_ptr->erase(n);
				}
			}
			
			for (auto&& [n, v] : upvalue_map) {
				//��ǰ����������� ������������Զ����δ��롣
				if ((!v.is_func && isVariableCrossDomain()) || (v.is_func && isFunctionCrossDomain())) {
					//���ɱ���״̬����
					Upvalue upvalue = { Upvalue::TYPE::SET_LOCAL };
					upvalue.name = v.name;
					upvalue.type = v.type;
					upvalue.is_func = v.is_func;

					m_current_group_id = getCrossDomainIndex();
					upvalue.value = getUpvalue({ Upvalue::TYPE::GET_LOCAL, v.name, v.type, "", "", v.is_func });
					upvalues += func->getSpaces() + "call " + getUpvalue(upvalue) + "\n";
					m_current_group_id = -1;

					if (prev_upvalue_map_ptr && prev_upvalue_map_ptr != &upvalue_map) {
						//��֪ͨ��һ��հ� �����Ǳ���״̬
						prev_upvalue_map_ptr->emplace(n, upvalue);
					}
				}
			}

			upvalue_map.clear();

			m_current_group_id = -1;

			return closure;
		}

		virtual std::string getUpvalue(const Upvalue& info) override  {
			return std::string();
		}


	public:
		//����ֲ�������
		std::map<std::string, Upvalue> upvalue_map;

		uint32_t m_current_group_id = -1;

		bool params_finish;
	
	};
}
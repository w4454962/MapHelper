#include "ActionNode.h"


int __fastcall fakeGetChildCount(Action* action);

ActionNode::ActionNode()
	:m_action(0),
	m_parent(0),
	m_nameId(0),
	m_trigger(0),
	m_parameter(0),
	m_haveHashLocal(false)
{}


ActionNode::ActionNode(Trigger* root)
	:ActionNode()
{
	m_trigger = root;
	m_type = Type::trigger;

	std::string name = std::string(root->name);
	convert_name(name);
	m_trigger_name = std::make_shared<std::string>(name);

	m_group_count = 1;
}


ActionNode::ActionNode(Action* action, ActionNodePtr parent)
{
	m_action = action;
	m_parent = parent;
	m_nameId = action != NULL ? hash_(action->name) : 0;
	m_trigger = parent.get() ? parent->m_trigger : 0;

	m_type = Type::action;

	m_trigger_name = parent->m_trigger_name;

	m_group_count = fakeGetChildCount(action);
}

ActionNode::ActionNode(Action* action, Parameter* owner, ActionNodePtr parent)
	:ActionNode(action,parent)
{
	m_parameter = owner;
	m_type = Type::parameter;
}

Action* ActionNode::getAction()
{
	return m_action;
}

Trigger* ActionNode::getTrigger()
{
	return m_trigger;
}

std::shared_ptr<std::string> ActionNode::getTriggerNamePtr()
{
	return m_trigger_name;
}

std::string ActionNode::getName() const
{
	if (m_type == Type::trigger && m_trigger)
	{
		return m_trigger->name;
	}
	if (m_action)
	{
		return m_action->name;
	}
	return std::string();
}

uint32_t ActionNode::getNameId() const
{
	return m_nameId;
}

ActionNodePtr ActionNode::getParentNode()
{
	return m_parent;
}

uint32_t ActionNode::getActionId()
{
	if (m_action)
	{
		return m_action->group_id;
	}
	return -1;
}

Action::Type ActionNode::getActionType()
{
	if (m_action)
	{
		return static_cast<Action::Type>(m_action->table->getType(m_action));
	}
	return Action::Type::none;
}

bool ActionNode::isRootNode()
{
	return m_parent == NULL;
}

bool ActionNode::canHasVarTable()
{
	static std::map<uint32_t, bool> t = {
		{"YDWETimerStartMultiple"s_hash,true},
		{"YDWEExecuteTriggerMultiple"s_hash,true},
		{"YDWERegisterTriggerMultiple"s_hash,true},
		{"DzTriggerRegisterMouseEventMultiple"s_hash,true},
		{"DzTriggerRegisterKeyEventMultiple"s_hash,true},
		{"DzTriggerRegisterMouseWheelEventMultiple"s_hash,true},
		{"DzTriggerRegisterMouseMoveEventMultiple"s_hash,true},
		{"DzTriggerRegisterWindowResizeEventMultiple"s_hash,true},
		{"DzFrameSetUpdateCallbackMultiple"s_hash,true},
		{"DzFrameSetScriptMultiple"s_hash,true}
	};
	auto id = getNameId();
	return t.find(id) != t.end();
}


ActionNodePtr ActionNode::getRootNode()
{
	ActionNodePtr root = shared_from_this();
	while (root.get())
	{
		if (root->m_parent == nullptr)
		{
			return root;
		}
		root = root->m_parent;
	}
	return nullptr;
}


ActionNodePtr ActionNode::getBranchNode()
{
	ActionNodePtr branch = shared_from_this();
	ActionNodePtr parent = m_parent;

	//搜索父节点
	while (parent.get())
	{
		bool isBreak = false;

		if (parent->m_action)
		{
			switch (parent->m_nameId)
			{
			case "ForForceMultiple"s_hash:
			case "ForGroupMultiple"s_hash:
			case "EnumDestructablesInRectAllMultiple"s_hash:
			case "EnumDestructablesInCircleBJMultiple"s_hash:
			case "EnumItemsInRectBJMultiple"s_hash:
			case "YDWEExecuteTriggerMultiple"s_hash:
			case "YDWETimerStartMultiple"s_hash:
			case "YDWERegisterTriggerMultiple"s_hash:
			case "AndMultiple"s_hash:
			case "OrMultiple"s_hash:
			case "DzTriggerRegisterMouseEventMultiple"s_hash:
			case "DzTriggerRegisterKeyEventMultiple"s_hash:
			case "DzTriggerRegisterMouseWheelEventMultiple"s_hash:
			case "DzTriggerRegisterMouseMoveEventMultiple"s_hash:
			case "DzTriggerRegisterWindowResizeEventMultiple"s_hash:
			case "DzFrameSetUpdateCallbackMultiple"s_hash:
			case "DzFrameSetScriptMultiple"s_hash:

			case "YDWEEnumUnitsInRangeMultiple"s_hash://这个节点不会生成函数 但是要用来判断是否处于该节点下
				isBreak = true; break;
			default:
				break;
			}

			for (size_t k = 0; k < parent->m_action->param_count; k++) 
			{
				Parameter* param = parent->m_action->parameters[k];
				const std::string child_type = param->type_name;

				if (child_type == "boolexpr" || child_type == "code") 
				{
					isBreak = true; 
					break;
				}
			}
		}
		if (isBreak) break;
		branch = parent;
		parent = parent->m_parent;
	}
	return branch;
}

size_t ActionNode::size()
{
	if (m_type == Type::trigger && m_trigger)
	{
		return m_trigger->line_count;
	}
	if (m_action)
	{
		return m_action->child_count;
	}
	return 0;
}

ActionNodePtr ActionNode::operator[](size_t n)
{
	Action* action = nullptr;
	if (n >= size())
	{
		//return ActionNodePtr();
		return nullptr;
	}
	if (m_type == Type::trigger)
	{
		action = m_trigger->actions[n];
	}
	else if (m_type == Type::action)
	{
		action = m_action->child_actions[n];
	}

	return std::make_shared<ActionNode>(action, shared_from_this());
}


void ActionNode::getChildNodeList(std::vector<ActionNodePtr>& list)
{
	list.clear();
	//如果是根节点 and 触发器对象是存在的
	if (m_type == Type::trigger && m_trigger)
	{
		ActionNodePtr parent = shared_from_this();
		for (uint32_t i = 0; i < m_trigger->line_count; i++)
		{
			Action* action = m_trigger->actions[i];
			ActionNodePtr node = std::make_shared<ActionNode>(action, parent);
			if (!action->enable ||!((int)action->group_id < node->getParentGroupCount()))
				continue;
			list.push_back(node);
		}
	}
	//否则是寻找该节点下的子节点
	else if (m_action)
	{
		ActionNodePtr parent = shared_from_this();

		for (uint32_t i = 0; i < m_action->child_count; i++)
		{
			Action* action = m_action->child_actions[i];
			ActionNodePtr node = std::make_shared<ActionNode>(action, parent);
			if (!action->enable || !((int)action->group_id < node->getParentGroupCount()))
				continue;
			list.push_back(node);
		}
	}
}

size_t ActionNode::count() const
{
	if (m_action)
	{
		return m_action->param_count;
	}
	return 0;
}

Parameter* ActionNode::operator()(size_t n) const
{
	if (n >= count() || !m_action)
		return nullptr;

	return m_action->parameters[n];
}

VarTablePtr ActionNode::getVarTable()
{
	if (m_hashVarTablePtr.get() == nullptr)
	{
		m_hashVarTablePtr = VarTablePtr(new std::map<std::string, std::string>);
	}
	return m_hashVarTablePtr;
}

VarTablePtr ActionNode::getLastVarTable()
{
	ActionNode* node = this;

	VarTablePtr retval;
	while (node)
	{
		if (node->m_hashVarTablePtr.get())
		{
			retval = node->m_hashVarTablePtr;
			break;
		}
		else if(node->m_action != m_action && (node->canHasVarTable() || node->isRootNode()))
		{
			retval = VarTablePtr(new std::map<std::string, std::string>);
			node->m_hashVarTablePtr = retval;
			break;
		}
		node = node->m_parent.get();
	}

	if (retval.get() == nullptr)
	{
		retval = VarTablePtr(new std::map<std::string, std::string>);
		m_hashVarTablePtr = retval;
	}

	return retval;
}

VarTablePtr ActionNode::getLocalTable()
{
	if (m_localTablePtr.get() == nullptr)
	{
		m_localTablePtr = VarTablePtr(new std::map<std::string, std::string>);
	}
	return m_localTablePtr;
}



int ActionNode::getParentGroupCount()
{
	ActionNodePtr parent = getParentNode();

	if (parent.get() == NULL)
	{
		return 1;
	}
	else if (parent->getAction())
	{
		return parent->m_group_count;
	}
	return 0;
}

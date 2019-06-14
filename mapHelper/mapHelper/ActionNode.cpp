#include "ActionNode.h"


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
	m_trigger_name = std::shared_ptr<std::string>(new std::string(name));

	
}


ActionNode::ActionNode(Action* action, ActionNodePtr parent)
{
	m_action = action;
	m_parent = parent;
	m_nameId = action != NULL ? hash_(action->name) : 0;
	m_trigger = parent.get() ? parent->m_trigger : 0;

	m_type = Type::action;

	m_trigger_name = parent->m_trigger_name;
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

std::string ActionNode::getName()
{
	if (m_type == Type::trigger && m_trigger)
	{
		return m_trigger->name;
	}
	else if (m_action)
	{
		return m_action->name;
	}
}

uint32_t ActionNode::getNameId()
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
		return m_action->child_flag;
	}
	return -1;
}

Action::Type ActionNode::getActionType()
{
	if (m_action)
	{
		return (Action::Type)m_action->table->getType(m_action);
	}
	return Action::Type::none;
}

bool ActionNode::isRootNode()
{
	return m_parent == NULL;
}


ActionNodePtr ActionNode::getRootNode()
{
	ActionNodePtr root = shared_from_this();
	while (root.get())
	{
		if (root->m_parent == NULL)
		{
			return root;
		}
		root = root->m_parent;
	}
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
				//会生成函数的几个节点
			case "ForForceMultiple"s_hash:
			case "ForGroupMultiple"s_hash:
			case "EnumDestructablesInRectAllMultiple"s_hash:
			case "EnumDestructablesInCircleBJMultiple"s_hash:
			case "EnumItemsInRectBJMultiple"s_hash:
			case "YDWETimerStartMultiple"s_hash:
			case "YDWERegisterTriggerMultiple"s_hash:
				isBreak = true; break;
			default:
				break;
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
	else if (m_action)
	{
		return m_action->child_count;
	}
	return 0;
}

ActionNodePtr ActionNode::operator[](size_t n)
{
	Action* action = NULL;
	if (n >= size())
	{
		return ActionNodePtr();
	}
	if (m_type == Type::trigger)
	{
		action = m_trigger->actions[n];
	}
	else if (m_type == Type::action)
	{
		action = m_action->child_actions[n];
	}

	return ActionNodePtr(new ActionNode(action, shared_from_this()));
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

			if (!action->enable)
				continue;
			list.push_back(ActionNodePtr(new ActionNode(action, parent)));
		}
	}
	//否则是寻找该节点下的子节点
	else if (m_action)
	{
		ActionNodePtr parent = shared_from_this();

		for (uint32_t i = 0; i < m_action->child_count; i++)
		{
			Action* action = m_action->child_actions[i];
			if (!action->enable)
				continue;
			list.push_back(ActionNodePtr(new ActionNode(action, parent)));
		}
	}
}

size_t ActionNode::count()
{
	if (m_action)
	{
		return m_action->param_count;
	}
	return 0;
}

Parameter* ActionNode::operator()(size_t n)
{
	if (n >= count() || !m_action)
		return NULL;

	return m_action->parameters[n];
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
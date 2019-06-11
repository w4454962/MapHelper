#include "ActionNode.h"


ActionNode::ActionNode()
	:m_action(0),
	m_parent(0),
	m_nameId(0),
	m_trigger(0),
	m_parameter(0)
{}


ActionNode::ActionNode(Trigger* root)
	:ActionNode()
{
	m_trigger = root;
	m_type = Type::trigger;
}


ActionNode::ActionNode(Action* action, ActionNodePtr parent)
{
	m_action = action;
	m_parent = parent;
	m_nameId = action != NULL ? hash_(action->name) : 0;
	m_trigger = parent.get() ? parent->m_trigger : 0;
	mapPtr = NULL;

	m_type = Type::action;
}


Action* ActionNode::getAction()
{
	return m_action;
}

Trigger* ActionNode::getOwner()
{
	return m_trigger;
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
	if (m_type == Type::action && m_action)
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
	ActionNodePtr root = ActionNodePtr(this);
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
	ActionNodePtr parent = m_parent;

	//搜索父节点
	while (parent)
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
		parent = parent->m_parent;
	}
	return parent;
}

size_t ActionNode::getChildCount()
{
	if (m_action)
	{
		return m_action->child_count;
	}
	return 0;
}
void ActionNode::getChildNodeList(std::vector<ActionNodePtr>& list)
{
	
	list.clear();

	//如果是根节点 and 触发器对象是存在的
	if (m_type == Type::trigger && m_trigger)
	{
		ActionNodePtr parent = ActionNodePtr(this);

		for (uint32_t i = 0; i < m_trigger->line_count; i++)
		{
			Action* action = m_trigger->actions[i];

			if (!action->enable)
				continue;
			list.push_back(ActionNodePtr(new ActionNode(action, parent)));
		}
	}
	//否则是寻找该节点下的子节点
	else if (m_type == Type::action && m_action)
	{
		ActionNodePtr parent = ActionNodePtr(this);

		for (uint32_t i = 0; i < m_action->child_count; i++)
		{
			Action* action = m_action->child_actions[i];
			if (!action->enable)
				continue;
			list.push_back(ActionNodePtr(new ActionNode(action, parent)));
		}
	}
}

size_t ActionNode::getParamCount()
{
	if (m_action)
	{
		return m_action->param_count;
	}
	return 0;
}

void ActionNode::getParamNodeList(std::vector<ParamNodePtr>& list)
{
	
	uint32_t count = m_action->param_count;

	for (uint32_t i = 0; i < count; i++)
	{
		Parameter* param = m_action->parameters[i];

	}
}

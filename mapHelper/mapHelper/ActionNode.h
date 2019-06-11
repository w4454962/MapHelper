#pragma once
#include "stdafx.h"
#include "EditorData.h"

//动作节点

//每个触发器为一个 root 根节点 根节点的 action 跟 parent 为NULL 
//每个动作 的 节点为 动作,父节点  父节点为上一个支点 如if then else  根为root节点
//每个参数的节点为 action 当前参数的函数调用动作,父节点为所在的动作

typedef std::shared_ptr<class ActionNode> ActionNodePtr;

typedef std::shared_ptr<class ParamNode> ParamNodePtr;

class ActionNode
{
public:

	//节点类型
	enum class Type
	{
		trigger,
		action,
		parameter,
	};

	ActionNode();
	ActionNode(Trigger* root);
	ActionNode(Action* action, ActionNodePtr parent);

	//获取节点的动作
	Action* getAction();

	//获取节点所属触发器
	Trigger* getOwner();

	//获取动作名id 字符串哈希值
	uint32_t getNameId();

	//获取动作所属子动作id
	uint32_t getActionId();

	//判断该节点是否是根节点
	bool isRootNode();

	//获取父节点
	ActionNodePtr getParentNode();

	//获取根节点
	ActionNodePtr getRootNode();

	//获取支点
	ActionNodePtr getBranchNode();

	size_t getChildCount();
	//获取子动作节点
	void getChildNodeList(std::vector<ActionNodePtr>& list);
	
	size_t getParamCount();
	void getParamNodeList(std::vector<ParamNodePtr>& list);


	Action::Type getActionType();

private:

protected:
	Action* m_action;//当前动作
	ActionNodePtr m_parent;//父节点

	uint32_t m_nameId;//动作名的哈希值id

	Trigger* m_trigger;//所属触发器

	Parameter* m_parameter;//所属参数 只有节点类型为参数时才有效

	//用来记录多层次逆天计时器的局部变量 以便再上一层函数中申明
	std::map<std::string, std::string>* mapPtr;

	Type m_type;

};



class ParamNode
{
public:
	ParamNode(Parameter* param, ActionNodePtr parent);

	ActionNodePtr getOwner();

private: 

protected: 
	Parameter* m_parameter;
	ActionNodePtr m_parent;
};

ParamNode::ParamNode(Parameter* param,ActionNodePtr parent)
{
	m_parameter = param;
	m_parent = parent;
}

ActionNodePtr ParamNode::getOwner()
{
	return m_parent;
}


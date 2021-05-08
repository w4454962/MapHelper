#pragma once
#include "stdafx.h"
#include "EditorData.h"


//动作节点

//每个触发器为一个 root 根节点 根节点的 action 跟 parent 为NULL 
//每个动作 的 节点为 动作,父节点  父节点为上一个支点 如if then else  根为root节点
//每个参数的节点为 action 当前参数的函数调用动作,父节点为所在的动作

typedef std::shared_ptr<class ActionNode> ActionNodePtr;

typedef std::shared_ptr<std::map<std::string, std::string>> VarTablePtr;

class ActionNode :public std::enable_shared_from_this<ActionNode>
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
	ActionNode(Action* action, Parameter* owner, ActionNodePtr parent);

	//获取节点的动作
	Action* getAction();

	//获取节点所属触发器
	Trigger* getTrigger();

	std::shared_ptr<std::string> getTriggerNamePtr();

	//获取动作名
	std::string getName() const;

	//获取动作名id 字符串哈希值
	uint32_t getNameId() const;

	//获取动作所属子动作id
	uint32_t getActionId();

	//判断该节点是否是根节点
	bool isRootNode();

	//获取父节点
	ActionNodePtr getParentNode();

	//获取根节点
	ActionNodePtr getRootNode();

	//获取支点 action 为支点下的第一个动作  返回值->getParentNode()可以获得实际分支的节点
	ActionNodePtr getBranchNode();

	//获取子动作数量
	size_t size();

	//node[1] 取得子动作
	ActionNodePtr operator[](size_t n);

	//获取全部子动作节点
	void getChildNodeList(std::vector<ActionNodePtr>& list);
	


	//获取参数数量
	size_t count() const;
	
	// node(1) 取该动作的参数
	Parameter* operator()(size_t n) const;


	Action::Type getActionType();

	VarTablePtr getVarTable();

	VarTablePtr getLastVarTable();

	VarTablePtr getLocalTable();

	int getParentGroupCount();

	bool m_haveHashLocal;

	
private:

protected:
	ActionNodePtr m_parent;//父节点

	uint32_t m_nameId;//动作名的哈希值id

	Action* m_action;//当前动作

	Parameter* m_parameter;//所属参数 只有节点类型为参数时才有效

	Trigger* m_trigger;//所属触发器

	std::shared_ptr<std::string> m_trigger_name;

	Type m_type;

	//用来记录多层次逆天计时器的局部变量 以便再上一层函数中申明
	std::shared_ptr<std::map<std::string, std::string>> m_hashVarTablePtr;

	//记录局部变量的map指针
	std::shared_ptr<std::map<std::string, std::string>> m_localTablePtr;

	int m_group_count;

};



#pragma once
#include "stdafx.h"
#include <memory>
#include <functional>

namespace mh {
	class Node;

	typedef std::shared_ptr<std::string> StringPtr;

	typedef std::shared_ptr<Node> NodePtr;

	typedef std::vector<std::string> Codes;

	typedef std::function<bool(NodePtr node)> NodeFilter;

	class Node :public std::enable_shared_from_this<Node> {
	public:
		enum class TYPE :int {
			ROOT, //根节点
			CALL, //不需要返回值
			GET,  //需要返回值
			PARAM //是一个参数器
		};

		//获取当前触发器
		virtual void* getData() = 0;

		//获取当前节点名字
		virtual StringPtr getName() = 0;

		//当前名字id
		virtual uint32_t getNameId() = 0;

		//节点类型
		virtual TYPE getType() = 0;

		//获取父节点
		virtual NodePtr getParentNode() = 0;

		//获取根节点
		virtual NodePtr getRootNode() = 0;

		//获取子动作列表
		virtual std::vector<NodePtr> getChildList() = 0;

		//获取值
		virtual bool getValue(Codes& result, const NodeFilter& filter) = 0;

		//获取文本
		virtual std::string toString(std::string& pre_actions) = 0;
	};

	NodePtr NodeFromTrigger(Trigger* trigger);

	NodePtr NodeFromAction(Action* action, uint32_t childId, NodePtr parent);

	NodePtr NodeFramParameter(Parameter* parameter, uint32_t index, NodePtr parent);

	std::string CodeConnent(Codes& codes);

	typedef NodePtr(*MakeNode)(void* action, uint32_t childId, NodePtr parent);


	void SpaceAdd();
	void SpaceRemove();
	std::string& Spaces(int offset = 0);

}
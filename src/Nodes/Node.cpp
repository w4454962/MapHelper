
#include "../stdafx.h"
#include <regex>
#include <memory>
#include <functional>
#include "../WorldEditor.h"
#include "../TriggerEditor.h"

#include "Function.hpp"
#include "Node.h"
#include "TriggerNode.hpp"
#include "ActionNode.hpp"
#include "ParameterNode.hpp"
#include "EventNode.hpp"
#include "ClosureNode.hpp"
#include "Closures.hpp"

namespace mh {
	typedef std::shared_ptr<class ActionNode> ActionNodePtr;

	std::unordered_map<Trigger*, bool> g_initTriggerMap;
	std::unordered_map<Trigger*, bool> g_disableTriggerMap;


	class BoolexprNode : public ParameterNode {
	public:
		REGISTER_FROM_PARAM(BoolexprNode)

		virtual std::string toString(TriggerFunction* func) override {
			NodePtr node = shared_from_this();

			NodePtr param = getParamActionNode();

			//创建一个闭包
			SigleNodeClosurePtr closure = SigleNodeClosurePtr(new SigleNodeClosure(param, "boolean", node));

			return  "Condition(" + closure->toString(func) + ")";;
		}
	};

	class CodeNode : public ParameterNode {
	public:
		REGISTER_FROM_PARAM(CodeNode)

		virtual std::string toString(TriggerFunction* func) override {
			NodePtr node = shared_from_this();

			NodePtr param = getParamActionNode();
			param->setType(TYPE::CALL);
			//创建一个闭包
			SigleNodeClosurePtr closure = SigleNodeClosurePtr(new SigleNodeClosure(param, "nothing", node));


			return closure->toString(func);
		}
	};

	class CommentString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CommentString)

		virtual std::string toString(TriggerFunction* func) override {
			NodePtr node = shared_from_this();
			NodePtr param = NodeFramParameter(m_action->parameters[0], 0, node);

			return toLine("//" + param->toString(func), func);
		}
	};

	//class CustomScriptCode : public ActionNode {
	//public:
	//	REGISTER_FROM_ACTION(CustomScriptCode)
	//
	//	virtual std::string toString(TriggerFunction* func) override {
	//		return toLine(m_action->parameters[0]->value, func);
	//	}
	//};
	
	class GetTriggerName : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetTriggerName)

		virtual std::string toString(TriggerFunction* func) override {
			return "\"" + getRootNode()->getName() + "\"";
		}
	};
	
	class OperatorString : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorString)

		virtual std::string toString(TriggerFunction* func) override {
			auto list = getParameterList();
			return "(" + list[0]->toString(func) + " + " + list[1]->toString(func) + ")";
		}
	};
	
	
	class ForLoopA : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopA)

		virtual std::string toString(TriggerFunction* func) override {
			std::string result;
	
			auto params = getParameterList();
	
			auto node = std::dynamic_pointer_cast<ParameterNode>(params[2])->getParamActionNode();
			node->setType(TYPE::CALL);

			std::string variable = m_nameId == "ForLoopA"s_hash ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		
			result += func->getSpaces() + "set " + variable + " = " + params[0]->toString(func) + "\n";
			result += func->getSpaces() + "loop\n";
			func->addSpace();
	
			result += func->getSpaces() + "exitwhen " + variable + ">" + params[1]->toString(func) + "\n";

			result += node->toString(func) + "\n";
			result += func->getSpaces() + "set " + variable + " = " + variable + " + 1\n";
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
	
			return result;
		}
	};
	
	
	class ForLoopVar : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopVar)

		virtual std::string toString(TriggerFunction* func) override {
			std::string result;
	
			auto params = getParameterList();
	
			std::string variable = params[0]->toString(func);
	
			auto node = std::dynamic_pointer_cast<ParameterNode>(params[3])->getParamActionNode();
			node->setType(TYPE::CALL);

			result += func->getSpaces() + "set " + variable + " = " + params[1]->toString(func) + "\n";
			result += func->getSpaces() + "loop\n";
			func->addSpace();
	
			result += func->getSpaces() + "exitwhen " + variable + ">" + params[2]->toString(func) + "\n";
			result += node->toString(func) + "\n";
			result += func->getSpaces() + "set " + variable + " = " + variable + " + 1\n";
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
			
	
			return result;
		}
	};
	
	class IfThenElse : public ActionNode {
	public:
		REGISTER_FROM_ACTION(IfThenElse)

		virtual std::string toString(TriggerFunction* func) override {
			std::string result;
			auto params = getParameterList();

			auto node0 = (std::dynamic_pointer_cast<ParameterNode>(params[0]))->getParamActionNode();
			auto node1 = (std::dynamic_pointer_cast<ParameterNode>(params[1]))->getParamActionNode();
			auto node2 = (std::dynamic_pointer_cast<ParameterNode>(params[2]))->getParamActionNode();

			node0->setType(TYPE::GET);
			node1->setType(TYPE::CALL);
			node2->setType(TYPE::CALL);

	
		
			result += func->getSpaces() + "if (" + node0->toString(func) + ") then\n";
			func->addSpace();
			result += node1->toString(func);
			func->subSpace();
			result += func->getSpaces() + "else\n";
			func->addSpace();
			result += node2->toString(func);
			func->subSpace();
			result += func->getSpaces() + "endif\n";
		;
		
			return result;
		}
	};

	class GetBooleanAnd : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetBooleanAnd)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(func) + " and " + params[1]->toString(func) + ")";
		}
	};

	class GetBooleanOr : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetBooleanOr)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(func) + " or " + params[1]->toString(func) + ")";
		}
	};

	class OperatorInt : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorInt)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(func) + " " + params[1]->toString(func) + " " + params[2]->toString(func) + ")";
		}
	};


	class OperatorCompare : public ActionNode {
	public:
		REGISTER_FROM_ACTION(OperatorCompare)

			virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			return "(" + params[0]->toString(func) + " " + params[1]->toString(func) + " " + params[2]->toString(func) + ")";
		}
	};

	class WaitForCondition : public ActionNode {
	public:
		REGISTER_FROM_ACTION(WaitForCondition)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string result;
			result += func->getSpaces( ) + "loop\n";
			result += func->getSpaces(1) + "exitwhen (" + params[0]->toString(func) + ")\n";
			result += func->getSpaces(1) + "call TriggerSleepAction(RMaxBJ(bj_WAIT_FOR_COND_MIN_INTERVAL, " + params[1]->toString(func) + "))\n";
			result += func->getSpaces( ) + "endloop\n";

			return result;
		}
	};

	class ForLoopAMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopAMultiple)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string loop_index = getName() == "ForLoopAMultiple" ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
			std::string loop_index_end = getName() == "ForLoopAMultiple" ? "bj_forLoopAIndexEnd" : "bj_forLoopBIndexEnd";

			std::string result;
			result += func->getSpaces() + "set " + loop_index + " = " + params[0]->toString(func) + "\n";
			result += func->getSpaces() + "set " + loop_index_end + " = " + params[1]->toString(func) + "\n";
			result += func->getSpaces() + "loop\n";
			func->addSpace();
			result += func->getSpaces() + "exitwhen " + loop_index + " > " + loop_index_end + "\n";
			for (auto& child : getChildList()) {
				result += child->toString(func);
			}
			result += func->getSpaces() + "set " + loop_index + " = " + loop_index + " + 1\n";
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
			return result;
		}
	};

	class ForLoopVarMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ForLoopVarMultiple)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string variable = params[0]->toString(func);

			std::string result;
			result += func->getSpaces() + "set " + variable + " = " + params[1]->toString(func) + "\n";
			result += func->getSpaces() + "loop\n";
			func->addSpace();
			result += func->getSpaces() + "exitwhen " + variable + " > " + params[2]->toString(func) + "\n";
			for (auto& child : getChildList()) {
				result += child->toString(func);
			}
			result += func->getSpaces() + "set " + variable + " = " + variable + " + 1\n";
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
			return result;
		}
	};

	class IfThenElseMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(IfThenElseMultiple)

		virtual std::string toString(TriggerFunction* func) override {

			std::string iftext, thentext, elsetext, result;
			std::vector<std::vector<NodePtr>> nodes(3);

			for (auto& node : getChildList()) {
				Action* action = (Action*)node->getData();
				uint32_t index = 0;
				if (get_action_type(action) == Action::Type::condition) {
					index = 0;
				} else {
					if (action->group_id == 1) {
						index = 1;
					}else {
						index = 2;
					}
				}
				nodes[index].push_back(node);
			}
			
			for (auto& node : nodes[0]) {
				if (!iftext.empty()) {
					iftext += " and ";
				}
				node->setType(TYPE::GET);
				iftext += node->toString(func);
			}
			func->addSpace();
			for (auto& node : nodes[1]) {
				thentext += node->toString(func);
			}
			for (auto& node : nodes[2]) {
				elsetext += node->toString(func);
			}
			func->subSpace();
			if (iftext.empty()) {
				iftext = "true";
			}
			result += func->getSpaces() + "if (" + iftext + ") then\n";
			result += thentext;
			result += func->getSpaces() + "else\n";
			result += elsetext;
			result += func->getSpaces() + "endif\n";

			return result;
		}
	};


	


	class AndMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(AndMultiple)

			virtual std::string toString(TriggerFunction* func) override {

			std::string oper = m_name == "AndMultiple" ? " and " : " or ";

			std::string iftext;

			for (auto& node : getChildList()) {
				if (!iftext.empty()) {
					iftext += oper;
				}
				iftext +=  node->toString(func);
			}
			if (iftext.empty()) {
				iftext = "true";
			}
			return "(" + iftext + ")";
		}
	};

	class SetVariable : public ActionNode {
	public:
		REGISTER_FROM_ACTION(SetVariable)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			return func->getSpaces() + "set " + params[0]->toString(func) + " = " + params[1]->toString(func) + "\n";
		}
	};


	class AddTriggerEvent : public ActionNode {
	public:
		REGISTER_FROM_ACTION(AddTriggerEvent)

			virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string trg = params[0]->toString(func);


			ParameterNodePtr param1 = std::dynamic_pointer_cast<ParameterNode>(params[1]);
			ActionNodePtr node = std::dynamic_pointer_cast<ActionNode>(param1->getParamActionNode());
			Action* action = (Action*)(node->getData());

			auto& editor = get_trigger_editor();
			std::string name = editor.getScriptName(action);

			std::string result = func->getSpaces() + "call " + name + "(" + trg;
			for (auto& arg : node->getParameterList()) {
				result += ", " + arg->toString(func);
			}
			result += ")\n";

			return result;
		}
	};


	class YDWEForLoopLocVarMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEForLoopLocVarMultiple)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string variable = std::string("ydul_") + m_action->parameters[0]->value;
			convert_loop_var_name(variable);

			//添加一个局变量
			func->current()->addLocal(variable, "integer", std::string(), false);
		
			std::string result;
			result += func->getSpaces() + "set " + variable + " = " + params[1]->toString(func) + "\n";
			result += func->getSpaces() + "loop\n";
			func->addSpace();
			result += func->getSpaces() + "exitwhen " + variable + " > " + params[2]->toString(func) + "\n";
			for (auto& node : getChildList()) {
				result += node->toString(func);
			}
			result += func->getSpaces() + "set " + variable + " = " + variable + " + 1\n";
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
			return result;
		}
	};

	class YDWERegionMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWERegionMultiple)

		virtual std::string toString(TriggerFunction* func) override {
			
			std::string result;

			for (auto& node : getChildList()) {
				result += node->toString(func);
			}
			return result;
		}
	};

	class YDWEEnumUnitsInRangeMultiple : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEEnumUnitsInRangeMultiple)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			params_finish = false;

			//添加局部变量
			func->current()->addLocal("ydl_group", "group", std::string(), false);
			func->current()->addLocal("ydl_unit", "unit", std::string(), false);

			std::string result;
			
			result += func->getSpaces() + "set ydl_group = CreateGroup()\n";
			result += func->getSpaces() + "call GroupEnumUnitsInRange(ydl_group, " \
				+ params[0]->toString(func) + ", " \
				+ params[1]->toString(func) + ", " \
				+ params[2]->toString(func) + ", null)\n";

			params_finish = true;

			result += func->getSpaces() + "loop\n";
			func->addSpace();
			result += func->getSpaces() + "set ydl_unit = FirstOfGroup(ydl_group)\n";
			result += func->getSpaces() + "exitwhen ydl_unit == null\n";
			result += func->getSpaces() + "call GroupRemoveUnit(ydl_group, ydl_unit)\n";

			for (auto& node : getChildList()) {
				result += node->toString(func);
			}
			func->subSpace();
			result += func->getSpaces() + "endloop\n";
			result += func->getSpaces() + "call DestroyGroup(ydl_group)\n";


			return result;
		}

	public:
		bool params_finish = false;
	};


	class YDWESaveAnyTypeDataByUserData : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWESaveAnyTypeDataByUserData)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			//typename_01_integer  + 11 = integer
			std::string result = func->getSpaces();

			result += "call YDUserDataSet(";
			result += std::string(m_action->parameters[0]->value + 11) + ", ";
			result += params[1]->toString(func) + ",";
			result += "\"" +std::string(m_action->parameters[2]->value) + "\", ";
			result += std::string(m_action->parameters[3]->value + 11) + ", ";
			result += params[4]->toString(func) + ")\n";
			return  result;
		}
	};

	class YDWEFlushAnyTypeDataByUserData : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEFlushAnyTypeDataByUserData)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//typename_01_integer  + 11 = integer
			std::string result = func->getSpaces();

			result += "call YDUserDataClear(";
			result += std::string(m_action->parameters[0]->value + 11) + ", ";
			result += params[1]->toString(func) + ",";
			result += "\"" + std::string(m_action->parameters[3]->value) + "\", ";
			result += std::string(m_action->parameters[2]->value + 11) + ")\n";
	
			return result;
		}
	};

	class YDWEFlushAllByUserData : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEFlushAllByUserData)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			//typename_01_integer  + 11 = integer
			std::string result = func->getSpaces();

			result += "call YDUserDataClearTable(" 
				+ std::string(m_action->parameters[0]->value + 11) 
				+ ", " 
				+ params[1]->toString(func) 
				+ ")\n";
			return result;
		}
	};

	


	class YDWETimerStartFlush : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWETimerStartFlush)

		virtual std::string toString(TriggerFunction* func) override {
			bool in_block = false;

			getValue([&](NodePtr ptr) {
				if (ptr.get() != this && ptr->getType() == TYPE::CLOSURE) {
					if (ptr->getNameId() == "YDWETimerStartMultiple"s_hash) {
						in_block = true;
					}
				}
				return false;
			});

			std::string result;
			if (in_block) {
				result += func->getSpaces() + "call YDLocal3Release()\n";
				result += func->getSpaces() + "call DestroyTimer(GetExpiredTimer())\n";
			} else {
				//result += "不要在逆天计时器的动作外使用<清除逆天计时器>";
				print("Warning: 触发器[%s] 在逆天计时器的动作外使用<清除逆天计时器>, 请尽快修复\n\n", base::u2a(getRootNode()->getName()).c_str());
			}
			
			return result;
		}
	};

	class YDWERegisterTriggerFlush : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWERegisterTriggerFlush)

		virtual std::string toString(TriggerFunction* func) override {
			bool in_block = false;

			getValue([&](NodePtr ptr) {
				if (ptr.get() != this && ptr->getType() == TYPE::CLOSURE) {
					if (ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash) {
						in_block = true;
					}
				}
				return false;
			});

			std::string result;
			if (in_block) {
				result += func->getSpaces() + "call YDLocal4Release()\n";
				result += func->getSpaces() + "call DestroyTrigger(GetTriggeringTrigger())\n";
			} else {
				//result += "不要在逆天触发器的动作外使用<清除逆天触发器>";
				print("Warning: 触发器[%s] 在逆天触发器的动作外使用<清除逆天触发器>, 请尽快修复\n\n", base::u2a(getRootNode()->getName()).c_str());
			}
			
			return result;
		}
	};

	class TriggerSleepAction : public ActionNode {
	public:
		REGISTER_FROM_ACTION(TriggerSleepAction)

			virtual std::string toString(TriggerFunction* func) override {
			bool in_block = false;

			NodePtr parent = nullptr;

			getValue([&](NodePtr ptr) {
				if ((ptr->getType() == TYPE::CLOSURE || ptr->getType() == TYPE::ROOT)) {
					if (ptr->getNameId() == "YDWETimerStartMultiple"s_hash || ptr->getNameId() == "YDWERegisterTriggerMultiple"s_hash) {
						in_block = true;
					}
					parent = ptr;
				}
				return false;
			});

			std::string result;
			if (in_block) {
				//result += "不要在逆天计时器/逆天触发器内使用等待";
				print("Warning: 触发器[%s] 在逆天计时器/逆天触发器内使用等待, 请尽快修复\n\n", base::u2a(getRootNode()->getName()).c_str());
			} else {
				result = ActionNode::toString(func);
			}

			if (parent == nullptr || parent->getType() == TYPE::ROOT) {
				auto node = std::dynamic_pointer_cast<TriggerNode>(getRootNode());
				node->hasUpvalue = true;
				result += "call YDLocalReset()\n";
			}
			return result;
		}
	};

	class ReturnAction : public ActionNode {
	public:
		REGISTER_FROM_ACTION(ReturnAction)

		virtual std::string toString(TriggerFunction* func) override {

			std::string result = func->getSpaces(-1) + "__RETURN__" 
				+ func->getSpaces() + "return\n";

			return result;
		}
	};


	class YDWEExitLoop : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEExitLoop)

		virtual std::string toString(TriggerFunction* func) override {
			return func->getSpaces() + "exitwhen true\n";
		}
	};

	class CustomScriptCode : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CustomScriptCode)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();
			std::string script = params[0]->toString(func);

			if (getType() == TYPE::CALL) { //将自定义值里的 局部变量申明进行拆分
				
				//匹配局部变量数组 
				std::regex reg("^\\s*local\\s+(\\w+)\\s+array\\s+(\\w+)");
		
				auto it_end = std::sregex_iterator();
				auto it = std::sregex_iterator(script.begin(), script.end(), reg);
				for (; it != it_end; ++it) {
					func->current()->addLocal(it->str(2), it->str(1), std::string(), true);
				}

				//匹配局部变量
				reg = std::regex("^\\s*local\\s+(\\w+)\\s+(\\w+)");
				it = std::sregex_iterator(script.begin(), script.end(), reg);
				for (; it != it_end; ++it) {
					if (it->str(2) != "array") {
						func->current()->addLocal(it->str(2), it->str(1), std::string(), false);
					}
				}

				//将数组申明删掉
				script = regex_replace(script, std::regex("^\\s*local\\s+\\w+\\s+array\\s+\\w+\\s*"), "");

				//将局部变量申明赋值 申明部分删掉
				script = regex_replace(script, std::regex("^\\s*local\\s+\\w+\\s+(\\w+)\\s*="), "set $1 =");

				//将局部变量申明删掉
				script = regex_replace(script, std::regex("^\\s*local\\s+\\w+\\s+\\w+\\s*"), "");	
			}
			return toLine(script, func);
		}
	};


	class YDWEActivateTrigger : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEActivateTrigger)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			Parameter* param = m_action->parameters[0];

			std::string result;

			if (param->typeId == Parameter::Type::variable && param->arrayParam == NULL) {
				const char* ptr = param->value;
				if (ptr && strncmp(ptr, "gg_trg_", 7) == 0)
					ptr = ptr + 7;
				std::string func_name = std::string("InitTrig_") + ptr;
				convert_name(func_name);

				std::string ret = params[1]->toString(func);
				if (ret.compare("true") == 0) {
					result += "call ExecuteFunc(\"" + func_name + "\")\n";
				} else {
					result += "call " + func_name + "()\n";
				}
			}
			return result;
		}
	};




	class YDWELoadAnyTypeDataByUserData : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWELoadAnyTypeDataByUserData)

		virtual std::string toString(TriggerFunction* func) override {

			auto params = getParameterList();

			ParameterNodePtr parent = std::dynamic_pointer_cast<ParameterNode>(getParentNode());
			Parameter* parent_parameter = (Parameter*)parent->getData();

			std::string result;
			result += "YDUserDataGet(";
			result += m_action->parameters[0]->value + 11;//typename_01_integer  + 11 = integer
			result += ", ";
			result += params[1]->toString(func);
			result += ",\"";
			result += m_action->parameters[2]->value;
			result += "\", ";
			result += parent_parameter->type_name;
			result += ")";
			return result;
		}
	};


	class YDWEHaveSavedAnyTypeDataByUserData : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEHaveSavedAnyTypeDataByUserData)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			std::string result;
			result += "YDUserDataHas(";
			result += m_action->parameters[0]->value + 11;//typename_01_integer  + 11 = integer
			result += ", ";
			result += params[1]->toString(func);
			result += ",\"";
			result += m_action->parameters[3]->value;
			result += "\", ";
			result += m_action->parameters[2]->value + 11;
			result += ")";
			return result;
		}
	};

	class GetEnumUnit : public ActionNode {
	public:
		REGISTER_FROM_ACTION(GetEnumUnit)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			bool in_block = false;

			getValue([&](NodePtr ptr) {
				if (ptr->getNameId() == "YDWEEnumUnitsInRangeMultiple"s_hash) {
					auto node = std::dynamic_pointer_cast<YDWEEnumUnitsInRangeMultiple>(ptr);
					if (node->params_finish) {
						in_block = true;
					}
					return true;
				}
				return false;
			});

			std::string result;
			if (in_block) {
				result += "ydl_unit";
			} else {
				result += m_name + "()";
			}

			return result;
		}
	};


	class YDWEForLoopLocVarIndex : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEForLoopLocVarIndex)

	virtual std::string toString(TriggerFunction* func) override {
			std::string result = std::string("ydul_") + m_action->parameters[0]->value;
			convert_loop_var_name(result);

			return result;
		}
	};


	class YDWEGetAnyTypeLocalVariable : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEGetAnyTypeLocalVariable)

		virtual std::string toString(TriggerFunction* func) override {
			ParameterNodePtr parent = std::dynamic_pointer_cast<ParameterNode>(getParentNode());
			Parameter* parent_parameter = (Parameter*)parent->getData();

			Upvalue upvalue = { Upvalue::TYPE::GET_LOCAL };
			upvalue.name = m_action->parameters[0]->value;;
			upvalue.type = parent_parameter->type_name;;

			auto root = std::dynamic_pointer_cast<TriggerNode>(getRootNode());
			root->hasUpvalue = true;

			//向上传参
			getValue([&](NodePtr ptr) {
				if (ptr->getType() == TYPE::CLOSURE) {
					auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
					//如果是动作区 并且 不存在参数区的 默认传参
					if (node->getCurrentGroupId() > node->getCrossDomainIndex() || node->getCrossDomainIndex() == -1) {
						node->upvalue_map.emplace(upvalue.name, upvalue);
						return true;
					} 

				} 
				return false;
			});

			
			std::string result = getUpvalue(upvalue);

			//记录逆天变量 用来类型检查
			auto& map = root->all_upvalue_map[upvalue.name];
			auto& editor = get_trigger_editor();
			auto base = editor.getBaseType(upvalue.type);
			if (map.find(base) == map.end()) {
				map.emplace(base, result);
			}

			return result;
		}
	};

	
	class YDWESetAnyTypeLocalVariable : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWESetAnyTypeLocalVariable)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			Upvalue upvalue = { Upvalue::TYPE::SET_LOCAL };
			upvalue.name = m_action->parameters[1]->value;;
			upvalue.type = m_action->parameters[0]->value + 11;;
			upvalue.value = params[2]->toString(func);;

			auto root = std::dynamic_pointer_cast<TriggerNode>(getRootNode());
			root->hasUpvalue = true;


			std::string result = "call " + getUpvalue(upvalue);

			//主动申明
			getValue([&](NodePtr ptr) {
				if (ptr->getType() == TYPE::CLOSURE) {
					auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
					if (node->getCurrentGroupId() == node->getCrossDomainIndex()) {
						node->define_upvalue_map.emplace(upvalue.name, upvalue);
					}
					return true;
				}
				return false;
			});


			//记录逆天变量 用来类型检查
			auto& map = root->all_upvalue_map[upvalue.name];
			auto& editor = get_trigger_editor();
			auto base = editor.getBaseType(upvalue.type);
			if (map.find(base) == map.end()) {
				map.emplace(base, result);
			}

			return func->getSpaces() + result + "\n";
		}
	};


	class YDWEGetAnyTypeLocalArray : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWEGetAnyTypeLocalArray)

		virtual std::string toString(TriggerFunction* func) override {

			ParameterNodePtr parent = std::dynamic_pointer_cast<ParameterNode>(getParentNode());
			Parameter* parent_parameter = (Parameter*)parent->getData();

			auto params = getParameterList();

			Upvalue upvalue = { Upvalue::TYPE::GET_ARRAY };
			upvalue.name = m_action->parameters[0]->value;;
			upvalue.type = parent_parameter->type_name;;
			upvalue.index = params[1]->toString(func);;

			auto root = std::dynamic_pointer_cast<TriggerNode>(getRootNode());
			root->hasUpvalue = true;


			std::string result = getUpvalue(upvalue);

			//主动申明
			getValue([&](NodePtr ptr) {
				if (ptr->getType() == TYPE::CLOSURE) {
					auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
					if (node->getCurrentGroupId() == node->getCrossDomainIndex()) {
						node->define_upvalue_map.emplace(upvalue.name, upvalue);
					}
					return true;
				}
				return false;
			});

			//记录逆天变量 用来类型检查
			auto& map = root->all_upvalue_map[upvalue.name];
			auto& editor = get_trigger_editor();
			auto base = editor.getBaseType(upvalue.type);
			if (map.find(base) == map.end()) {
				map.emplace(base, result);
			}

			return result;
		}
	};

	class YDWESetAnyTypeLocalArray : public ActionNode {
	public:
		REGISTER_FROM_ACTION(YDWESetAnyTypeLocalArray)

		virtual std::string toString(TriggerFunction* func) override {
			auto params = getParameterList();

			Upvalue upvalue = { Upvalue::TYPE::SET_ARRAY };
			upvalue.name = m_action->parameters[1]->value;
			upvalue.type = m_action->parameters[0]->value + 11;
			upvalue.index = params[2]->toString(func);
			upvalue.value = params[3]->toString(func);;

			auto root = std::dynamic_pointer_cast<TriggerNode>(getRootNode());
			root->hasUpvalue = true;

			std::string result = "call " + getUpvalue(upvalue);


			//记录逆天变量 用来类型检查
			auto& map = root->all_upvalue_map[upvalue.name];
			auto& editor = get_trigger_editor();
			auto base = editor.getBaseType(upvalue.type);
			if (map.find(base) == map.end()) {
				map.emplace(base, result);
			}


			return func->getSpaces() + result + "\n";
		}
	};

	//跨域 让触发器api 在计时器里也能访问到对象
	class CrossDomain : public ActionNode {
	public:
		REGISTER_FROM_ACTION(CrossDomain)

		virtual std::string toString(TriggerFunction* func) override {
			
			auto& editor = get_trigger_editor();

			ParameterNodePtr parent = std::dynamic_pointer_cast<ParameterNode>(getParentNode());
			Parameter* parent_parameter = (Parameter*)parent->getData();

			std::string result;

			Upvalue upvalue = { Upvalue::TYPE::GET_LOCAL };
			upvalue.name = m_name;
			upvalue.type = editor.getBaseType(parent_parameter->type_name);
			upvalue.is_func = true;

			getValue([&](NodePtr ptr) {
				auto type = ptr->getType();
				if (type == TYPE::CLOSURE) {
					auto node = std::dynamic_pointer_cast<ClosureNode>(ptr);
					if (node->isFunctionCrossDomain()) {
						//向上投递
						node->upvalue_map.emplace(upvalue.name, upvalue);
						if (node->getCurrentGroupId() > node->getCrossDomainIndex()) {
							result += node->getUpvalue(upvalue);
							return true;
						}
					} else {
						return true;
					}
				}
				return false;
			});


			//如果没能从上一层作用域里拿到函数值 则自己调用
			if (result.empty()) {
				result = upvalue.name + "()";
			}
			return result;
		}
	};


	//参数类型节点
	std::unordered_map<std::string, MakeNode> MakeParameterNodeMap = {
		{"boolexpr",				BoolexprNode::From},
		{"code",					CodeNode::From},
	};

	//动作节点
	std::unordered_map<std::string, MakeNode> MakeActionNodeMap = {
		{"CommentString",			CommentString::From},
		//{"CustomScriptCode",		CustomScriptCode::From},
		{"GetTriggerName",			GetTriggerName::From},
		{"OperatorString",			OperatorString::From},

		{"ForLoopA",				ForLoopA::From},
		{"ForLoopB",				ForLoopA::From}, //跟A一样

		{"ForLoopVar",				ForLoopVar::From},
		{"IfThenElse",				IfThenElse::From},
		{"GetBooleanAnd",			GetBooleanAnd::From},
		{"GetBooleanOr",			GetBooleanOr::From},

		{"OperatorInt",				OperatorInt::From},
		{"OperatorReal",			OperatorInt::From}, //跟int一样
		{"OperatorDegree",			OperatorInt::From},
		
		{"OperatorCompare",			OperatorCompare::From},
		{"OperatorCompare",			OperatorCompare::From},
		{"WaitForCondition",		WaitForCondition::From},

		{"ForLoopAMultiple",		ForLoopAMultiple::From},
		{"ForLoopBMultiple",		ForLoopAMultiple::From}, //跟a一样

		{"ForLoopVarMultiple",		ForLoopVarMultiple::From},
		{"IfThenElseMultiple",		IfThenElseMultiple::From},

		{"ForForceMultiple",					ForForceMultiple::From}, //这几个实现相同
		{"ForGroupMultiple",					ForForceMultiple::From},
		{"EnumDestructablesInRectAllMultiple",	ForForceMultiple::From},
		{"EnumDestructablesInCircleBJMultiple",	ForForceMultiple::From},
		{"EnumItemsInRectBJMultiple",			ForForceMultiple::From},

		{"AndMultiple",							AndMultiple::From},
		{"OrMultiple",							AndMultiple::From},
		{"SetVariable",							SetVariable::From},
		{"AddTriggerEvent",						AddTriggerEvent::From},


		{"YDWEForLoopLocVarMultiple",			YDWEForLoopLocVarMultiple::From },
		{"YDWERegionMultiple",					YDWERegionMultiple::From },
		{"YDWEEnumUnitsInRangeMultiple",		YDWEEnumUnitsInRangeMultiple::From },
		{"YDWESaveAnyTypeDataByUserData",		YDWESaveAnyTypeDataByUserData::From },
		{"YDWEFlushAnyTypeDataByUserData",		YDWEFlushAnyTypeDataByUserData::From },
		{"YDWEFlushAllByUserData",				YDWEFlushAllByUserData::From },
		{"YDWEExecuteTriggerMultiple",			YDWEExecuteTriggerMultiple::From },
		{"YDWETimerStartMultiple",				YDWETimerStartMultiple::From },
		{"YDWERegisterTriggerMultiple",			YDWERegisterTriggerMultiple::From },
		{"YDWETimerStartFlush",					YDWETimerStartFlush::From },

		{"YDWERegisterTriggerFlush",			YDWERegisterTriggerFlush::From },
		{"TriggerSleepAction",					TriggerSleepAction::From },
		{"PolledWait",							TriggerSleepAction::From },

		{"ReturnAction",						ReturnAction::From },
		{"YDWEExitLoop",						YDWEExitLoop::From },

		{"CustomScriptCode",					CustomScriptCode::From },
		{"YDWECustomScriptCode",				CustomScriptCode::From },
		{"YDWEActivateTrigger",					YDWEActivateTrigger::From },


		{"DzTriggerRegisterMouseEventMultiple",			DzTriggerRegisterMouseEventMultiple::From },
		{"DzTriggerRegisterKeyEventMultiple",			DzTriggerRegisterMouseEventMultiple::From },
		{"DzTriggerRegisterMouseWheelEventMultiple",	DzTriggerRegisterMouseEventMultiple::From },
		{"DzTriggerRegisterMouseMoveEventMultiple",		DzTriggerRegisterMouseEventMultiple::From },
		{"DzTriggerRegisterWindowResizeEventMultiple",	DzTriggerRegisterMouseEventMultiple::From },
		{"DzFrameSetUpdateCallbackMultiple",			DzTriggerRegisterMouseEventMultiple::From },
		{"DzFrameSetScriptMultiple",					DzTriggerRegisterMouseEventMultiple::From },


		{"YDWELoadAnyTypeDataByUserData",			YDWELoadAnyTypeDataByUserData::From },
		{"YDWEHaveSavedAnyTypeDataByUserData",		YDWEHaveSavedAnyTypeDataByUserData::From },
		{"GetEnumUnit",								GetEnumUnit::From },
		{"GetFilterUnit",							GetEnumUnit::From },
		{"YDWEForLoopLocVarIndex",					YDWEForLoopLocVarIndex::From },

		{"YDWEGetAnyTypeLocalVariable",				YDWEGetAnyTypeLocalVariable::From},
		{"YDWESetAnyTypeLocalVariable",				YDWESetAnyTypeLocalVariable::From},
		{"YDWEGetAnyTypeLocalArray",				YDWEGetAnyTypeLocalArray::From},
		{"YDWESetAnyTypeLocalArray",				YDWESetAnyTypeLocalArray::From},

		{"GetTriggeringTrigger",					CrossDomain::From},
		{"GetTriggerEventId",						CrossDomain::From},
		{"GetEventGameState",						CrossDomain::From},
		{"GetWinningPlayer",						CrossDomain::From},
		{"GetTriggeringRegion",						CrossDomain::From},
		{"GetEnteringUnit",							CrossDomain::From},
		{"GetLeavingUnit",							CrossDomain::From},
		{"GetTriggeringTrackable",					CrossDomain::From},
		{"GetClickedButton",						CrossDomain::From},
		{"GetClickedDialog",						CrossDomain::From},
		{"GetTournamentFinishSoonTimeRemaining",	CrossDomain::From},
		{"GetTournamentFinishNowRule",				CrossDomain::From},
		{"GetTournamentFinishNowPlayer",			CrossDomain::From},
		{"GetSaveBasicFilename",					CrossDomain::From},
		{"GetTriggerPlayer",						CrossDomain::From},
		{"GetLevelingUnit",							CrossDomain::From},
		{"GetLearningUnit",							CrossDomain::From},
		{"GetLearnedSkill",							CrossDomain::From},
		{"GetLearnedSkillLevel",					CrossDomain::From},
		{"GetRevivableUnit",						CrossDomain::From},
		{"GetRevivingUnit",							CrossDomain::From},
		{"GetAttacker",								CrossDomain::From},
		{"GetRescuer",								CrossDomain::From},
		{"GetDyingUnit",							CrossDomain::From},
		{"GetKillingUnit",							CrossDomain::From},
		{"GetDecayingUnit",							CrossDomain::From},
		{"GetConstructingStructure",				CrossDomain::From},
		{"GetCancelledStructure",					CrossDomain::From},
		{"GetConstructedStructure",					CrossDomain::From},
		{"GetResearchingUnit",						CrossDomain::From},
		{"GetResearched",							CrossDomain::From},
		{"GetTrainedUnitType",						CrossDomain::From},
		{"GetTrainedUnit",							CrossDomain::From},
		{"GetDetectedUnit",							CrossDomain::From},
		{"GetSummoningUnit",						CrossDomain::From},
		{"GetSummonedUnit",							CrossDomain::From},
		{"GetTransportUnit",						CrossDomain::From},
		{"GetLoadedUnit",							CrossDomain::From},
		{"GetSellingUnit",							CrossDomain::From},
		{"GetSoldUnit",								CrossDomain::From},
		{"GetBuyingUnit",							CrossDomain::From},
		{"GetSoldItem",								CrossDomain::From},
		{"GetChangingUnit",							CrossDomain::From},
		{"GetChangingUnitPrevOwner",				CrossDomain::From},
		{"GetManipulatingUnit",						CrossDomain::From},
		{"GetManipulatedItem",						CrossDomain::From},
		{"GetOrderedUnit",							CrossDomain::From},
		{"GetIssuedOrderId",						CrossDomain::From},
		{"GetOrderPointX",							CrossDomain::From},
		{"GetOrderPointY",							CrossDomain::From},
		{"GetOrderPointLoc",						CrossDomain::From},
		{"GetOrderTarget",							CrossDomain::From},
		{"GetOrderTargetDestructable",				CrossDomain::From},
		{"GetOrderTargetItem",						CrossDomain::From},
		{"GetOrderTargetUnit",						CrossDomain::From},
		{"GetSpellAbilityUnit",						CrossDomain::From},
		{"GetSpellAbilityId",						CrossDomain::From},
		{"GetSpellAbility",							CrossDomain::From},
		{"GetSpellTargetLoc",						CrossDomain::From},
		{"GetSpellTargetDestructable",				CrossDomain::From},
		{"GetSpellTargetItem",						CrossDomain::From},
		{"GetSpellTargetUnit",						CrossDomain::From},
		{"GetEventPlayerState",						CrossDomain::From},
		{"GetEventPlayerChatString",				CrossDomain::From},
		{"GetEventPlayerChatStringMatched",			CrossDomain::From},
		{"GetTriggerUnit",							CrossDomain::From},
		{"GetEventUnitState",						CrossDomain::From},
		{"GetEventDamage",							CrossDomain::From},
		{"GetEventDamageSource",					CrossDomain::From},
		{"GetEventDetectingPlayer",					CrossDomain::From},
		{"GetEventTargetUnit",						CrossDomain::From},
		{"GetTriggerWidget",						CrossDomain::From},
		{"GetTriggerDestructable",					CrossDomain::From},
	};

	



	NodePtr NodeFromTrigger(Trigger* trigger) {
		return TriggerNode::From(trigger, -1, nullptr);
	}

	NodePtr NodeFromAction(Action* action, uint32_t index, NodePtr parent) {

		MakeNode From = nullptr;

		std::string name = action->name;

		if (get_action_type(action) == Action::Type::event) {
			From = EventNode::From;
		} else if (MakeActionNodeMap.find(name) != MakeActionNodeMap.end()) {
			From = MakeActionNodeMap.at(name);
		} else if (name.substr(0, 15) == "OperatorCompare") {
			From = OperatorCompare::From;
		} else {
			From = ActionNode::From;
		}

		return From(action, index, parent);
	}

	NodePtr NodeFramParameter(Parameter* parameter, uint32_t index, NodePtr parent) {
		const auto child_type = std::string(parameter->type_name);

		auto it = MakeParameterNodeMap.find(child_type); //有预置类型的
		if (it != MakeParameterNodeMap.end()) {
			return it->second(parameter, index, parent);
		}
		return ParameterNode::From(parameter, index, parent);
	}


	bool IsHandleType(const std::string& type) {
		switch (hash_(type.c_str())) {
		case "nothing"s_hash:
		case "integer"s_hash:
		case "real"s_hash:
		case "boolean"s_hash:
		case "string"s_hash:
		case "code"s_hash:
			return false;
		default:
			break;
		}
		return true;
	}
}





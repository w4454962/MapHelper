#pragma once 

#include "../stdafx.h"
#include <stack>
#include "../TriggerEditor.h"
#include <regex>

namespace mh {

	bool IsHandleType(const std::string& type);

	struct Local {
		std::string name;
		std::string type;
		std::string value;
		bool is_array = false;

		std::string toString(bool release) {
			std::string result;
			if (!release) { //define 
				result = "\tlocal " + type + " ";
				if (is_array) {
					result += "array ";
				}
				result += name;
				if (!value.empty()) {
					result += "= " + value;
				}
				result += "\n";
			} else { //release
				result = "\tset " + name + " = null\n";
			}
			return result;
		}
	};

	typedef std::shared_ptr<class Function> FunctionPtr;

	class Function {
	public:
		Function(const std::string& _name, const std::string& _returnt_type) 
			: m_space(1)
		{
			m_name = _name;
			m_return_type = _returnt_type;
		}

		friend Function& operator<<(Function& self, const std::string& line) {
			self.m_lines.push_back(line);
			return self;
		}

		void line(const std::string& line) {
			m_lines.push_back(line);
		}

		void addLocal(const std::string& name, const std::string type, const std::string& value = std::string(), bool is_array = false) {
			if (m_locals_map.find(name) == m_locals_map.end()) {
				m_locals.push_back(Local({ name, type, value, is_array }));
				m_locals_map[name] = true;
			}
		}

		void addSpace() {
			m_space++;
		}

		void subSpace() {
			if (m_space > 0)
				m_space--;
		}

		const std::string& getSpaces(int offset = 0) {
			auto& editor = get_trigger_editor();

			int index = m_space + offset;
			if (index >= 0) {
				return editor.spaces[index];
			}
			return "";
		}

		bool isEmpty() {
			return m_lines.empty() && m_locals.empty();
		}
			 
		std::string toString() {
		
			std::string result = "function " + m_name + " takes nothing returns " + m_return_type + "\n";
			//申明局部变量
			for (auto& local : m_locals) {
				result += local.toString(false);
			}

			result += insert_begin;

			std::string release = insert_end;
			for (auto& local : m_locals) {
				if (!local.is_array && IsHandleType(local.type)) {
					release += local.toString(true);
				}
			}
			
			if (m_return_type == "nothing") {
				m_lines.push_back(release);
			}

			for (auto& line : m_lines) {
				result += line;
			}
			result += "endfunction\n\n";

			if (!release.empty()) { //正则 将__RETURN__ 替换成实际 退出代码
		
				result = std::regex_replace(result, std::regex("__RETURN__"), release);
			} else {
				result = std::regex_replace(result, std::regex("__RETURN__"), "");
			}

	
			return result;
		}

		const std::string& getName() {
			return m_name;
		}
		
	public:
		std::string insert_begin;
		std::string insert_end;

	private:
		std::string m_name;
		std::string m_return_type;
		std::vector<std::string> m_lines;
	
		std::vector<Local> m_locals;
		std::map<std::string, bool> m_locals_map;
	

		int m_space;
	};


#define seperator "//===========================================================================\n"s

	class TriggerFunction {
	public:
		TriggerFunction(const std::string& name, const std::string& trigger_name) 
			:event(new Function("Init" + name, "nothing")),
			condition(new Function(name + "Conditions", "boolean")),
			action(new Function(name + "Actions", "nothing"))
		{
			m_comment = seperator;
			m_comment += "// Trigger: " + trigger_name + "\n";
			m_comment += base::a2u("//自定义jass生成器 作者：007 \n//有bug到魔兽地图编辑器吧 @w4454962 \n//bug反馈群：724829943   lua 技术交流3群：710331384\n");
			m_comment += seperator;
		}

		//重载操作符 为当前函数写入一行代码
		friend TriggerFunction& operator<<(TriggerFunction& self, const std::string& line) {
			if (!self.m_stack.empty()) {
				*self.m_stack.top() << line;
			}
			return self;
		}

		void addFunction(FunctionPtr func) {
			m_func_list.push_back(func);
		}

		Function& pushFunction(const std::string& name, const std::string return_type) {
			FunctionPtr func = FunctionPtr(new Function(name, return_type));
			m_func_list.push_back(func);
			return *func;
		}

		void push(FunctionPtr func) {
			if (m_stack.empty() || m_stack.top() != func) {
				m_stack.push(func);
			}
		}

		void pop() {
			if (!m_stack.empty()) {
				m_stack.pop();
			}
		}
		FunctionPtr current() {
			if (!m_stack.empty()) {
				return m_stack.top();
			}
			return action;
		}

		void addSpace() {
			current()->addSpace();
		}

		void subSpace() {
			current()->subSpace();
		}

		const std::string& getSpaces(int offset = 0) {
			return current()->getSpaces(offset);
		}


		
		std::string toString() {
			std::string result;

			result = m_comment;
			
			
			for(auto& func: m_func_list){
				result += func->toString();
			}
			result += action->toString();
			result += seperator;
			result += event->toString();

			return result;
		}

	public:
		FunctionPtr event;
		FunctionPtr condition;
		FunctionPtr action;

	private:
		std::string m_comment;
		std::vector<FunctionPtr> m_func_list;
		std::stack<FunctionPtr> m_stack;

	};
}

#undef seperator
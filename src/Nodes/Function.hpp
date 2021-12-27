#pragma once 

#include "../stdafx.h"
#include <stack>
#include "../TriggerEditor.h"

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

	class Function {
	public:
		Function(const std::string& _name, const std::string& _returnt_type) 
			:m_end_pos(-1),
			m_space(1)
		{
			m_name = _name;
			m_return_type = _returnt_type;
		}

		friend Function& operator<<(Function& self, const std::string& line) {
			self.m_lines.push_back(line);
			return self;
		}

		void line(const std::string& line, bool is_retn) {
			m_lines.push_back(line);
			if (is_retn) {
				m_ends.push_back(m_lines.size() - 1);
			}
		}

		void nextIsRetn() {
			m_ends.push_back(m_lines.size());
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

			int index = max(0, m_space - offset);
			return editor.spaces[index];
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
			if (!release.empty()) {
				for (auto& pos : m_ends) {
					auto& line = m_lines[pos];
					line = release + line;
				}
			}
			if (m_return_type == "nothing") {
				m_lines.push_back(release);
			}

			for (auto& line : m_lines) {
				result += line;
			}
			
			result += "endfunction\n\n";
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
		uint32_t m_end_pos;
		std::vector<uint32_t> m_ends;
		std::vector<Local> m_locals;
		std::map<std::string, bool> m_locals_map;
	

		int m_space;
	};


#define seperator "//===========================================================================\n"s

	class TriggerFunction {
	public:
		TriggerFunction(const std::string& name, const std::string& trigger_name) 
			:event("Init" + name, "nothing"),
			condition(name + "Conditions", "boolean"),
			action(name + "Actions", "nothing")
		{
			m_comment = seperator;
			m_comment += "// Trigger: " + trigger_name + "\n";
			m_comment += base::a2u("//自定义jass生成器 作者： 阿七  \n//有bug到魔兽地图编辑器吧 @w4454962 \n//技术交流群：1019770872\n");
			m_comment += seperator;
		}

		//重载操作符 为当前函数写入一行代码
		friend TriggerFunction& operator<<(TriggerFunction& self, const std::string& line) {
			if (!self.m_stack.empty()) {
				*self.m_stack.top() << line;
			}
			return self;
		}

		void addFunction(Function* func) {
			m_func_list.push_back(func);
		}

		Function& pushFunction(const std::string& name, const std::string return_type) {
			Function* func = new Function(name, return_type);
			m_func_list.push_back(func);
			return *func;
		}

		void push(Function* func) {
			if (m_stack.empty() || m_stack.top() != func) {
				m_stack.push(func);
			}
		}

		void pop() {
			if (!m_stack.empty()) {
				m_stack.pop();
			}
		}
		Function* current() {
			if (!m_stack.empty()) {
				return m_stack.top();
			}
			return &action;
		}

		void addSpace() {
			current()->addSpace();
		}

		void subSpace() {
			current()->subSpace();
		}

		const std::string& getSpaces(int offset = 0) {
			return current()->getSpaces();
		}

		std::string toString() {
			std::string result;

			result = m_comment;
			
			if (!condition.isEmpty()) {
				result += condition.toString();
			}

			for(auto& func: m_func_list){
				result += func->toString();
			}
			result += action.toString();
			result += seperator;
			result += event.toString();

			return result;
		}

	public:
		Function event;
		Function condition;
		Function action;

	private:
		std::string m_comment;
		std::vector<Function*> m_func_list;
		std::stack<Function*> m_stack;

	};
}

#undef seperator
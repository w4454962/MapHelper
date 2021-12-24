#pragma once

namespace mh {

	class ParameterNode : public Node {
	public:
		REGISTER_FROM(ParameterNode, Parameter*)

			virtual void* getData() override { return m_parameter; }

		virtual StringPtr getName() override { return m_name; }

		virtual uint32_t getNameId() override { return m_nameId; }

		virtual TYPE getType() override { return TYPE::PARAM; }

		virtual NodePtr getParentNode() override { return m_parent; }

		virtual NodePtr getRootNode() override { return m_root; }

		virtual std::vector<NodePtr> getChildList() override {
			std::vector<NodePtr> list;

			if (m_parameter->funcParam) {

				NodePtr node = shared_from_this();

				NodePtr child = NodeFromAction(m_parameter->funcParam, 0, node);
				list.push_back(child);
			}
			return list;
		}

		virtual bool getValue(Codes& result, const NodeFilter& filter) override {

			NodePtr node = shared_from_this();
			if (filter(node)) {
				return true;
			}
			else if (m_parent->getType() != TYPE::ROOT) { //向上传递
				return m_parent->getValue(result, filter);
			}

			return true;
		}

		virtual std::string toString(std::string& pre_actions) override {
			;
			NodePtr node = shared_from_this();

			auto& editor = get_trigger_editor();
			auto& world = get_world_editor();

			auto value = std::string(m_parameter->value);

			switch (m_parameter->typeId)
			{
			case Parameter::Type::invalid:
				return "";

			case Parameter::Type::preset: //预设值
			{
				const auto preset_type = world.getConfigData("TriggerParams", value, 1);
				const auto preset_value = world.getConfigData("TriggerParams", value, 2);
				bool has_quote_symbol = preset_value.find('`') == 0;

				if (editor.getBaseType(preset_type) == "string") {
					auto result = world.getConfigData("TriggerParams", value, 2);
					if (has_quote_symbol) { //如果是未定义类型的 并且是单引号开头的 替换成双引号
						result = string_replaced(result, "`", "\"");
					}
					return result;
				}
				return world.getConfigData("TriggerParams", value, 2);
			}
			case Parameter::Type::variable:
			{
				auto result = value;

				if (!result._Starts_with("gg_")) {
					result = "udg_" + result;
				}
				//if (value == "Armagedontimerwindow") //不知道是什么
				//{
				//	puts("s");
				//}
				if (m_parameter->arrayParam) {
					NodePtr array_node = NodeFramParameter(m_parameter->arrayParam, 0, node);
					result += "[" + array_node->toString(pre_actions) + "]";
				}
				return result;
			}

			case Parameter::Type::function:
			{
				auto list = getChildList();
				if (list.size() > 0) {
					return list[0]->toString(pre_actions);
				}
				return value + "()";
			}

			case Parameter::Type::string:
			{
				auto type{ std::string(m_parameter->type_name) };

				switch (hash_(type.c_str()))
				{
				case "scriptcode"s_hash:
					return value;
				case "abilcode"s_hash:
				case "heroskillcode"s_hash:
				case "buffcode"s_hash:
				case "destructablecode"s_hash:
				case "itemcode"s_hash:
					//case "ordercode"s_hash:
				case "techcode"s_hash:
				case "doodadcode"s_hash:// 处理下装饰物没有判断的问题
				case "unitcode"s_hash:
					return "'" + value + "'";
				default:
				{
					uint32_t is_import_path = 0;
					auto* type_data = editor.getTypeData(type);
					if (type_data) {
						is_import_path = type_data->is_import_path;
					}
					if (type == "HotKey" && value == "HotKeyNull") {
						return "0";
					}
					if (is_import_path || editor.getBaseType(type) == "string") {
						value = string_replaced(value, "\\", "\\\\");
						return "\"" + string_replaced(value, "\"", "\\\"") + "\"";
					}
					return value;
				}
				}
			}
			default:
				break;
			}
		}

	private:
		ParameterNode(Parameter* parameter, uint32_t index, NodePtr parent) {
			m_parameter = parameter;

			m_parent = parent;

			m_index = index;

			m_action = parameter->funcParam;
			m_name = StringPtr(new std::string());
			m_nameId = 0;

			if (parent->getType() == TYPE::ROOT) {
				m_root = parent;
			}
			else {
				m_root = parent->getRootNode();
			}
		}

	protected:
		Parameter* m_parameter;
		Action* m_action;

		NodePtr m_root;
		NodePtr m_parent;

		StringPtr m_name;
		uint32_t m_nameId;
		uint32_t m_index;
	};
}
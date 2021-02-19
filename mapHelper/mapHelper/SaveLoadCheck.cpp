#include "SaveLoadCheck.h"
#include "unicode.h"
class CSaveLoadCheck {
public:
	typedef std::shared_ptr<std::map<std::string, std::string>> TSaveLoadMap;

	void Reset() {
		m_map->clear();
	}

	bool emplace(const std::string key, const std::string value) {
		if (m_map->find(key) == m_map->end()) {
			m_map->emplace(key, this->Convert(value));
			return true;
		}
		else {
			return (m_map->at(key).compare(this->Convert(value)) == 0);
		}

		return false;
	}

	const std::string get(const std::string key) {
		if (m_map->find(key) == m_map->end()) {
			return "";
		}
		else {
			return this->Convert(m_map->at(key).c_str());
		}

		return "";
	}

private:
	const std::string Convert(const std::string src) const {
		if (src == "StringExt") {
			return "string";
		}
		if (src == "imagefile") {
			return "string";
		}
		if (src == "modelfile") {
			return "string";
		}
		if (src == "radian") {
			return "real";
		}
		if (src == "degree") {
			return "real";
		}
		if (src == "degree") {
			return "real";
		}
		if (src == "unitcode") {
			return "integer";
		}
		if (src == "abilcode") {
			return "integer";
		}
		if (src == "itemcode") {
			return "integer";
		}
		if (src.substr(0, 5) == "AUTO_") {
			return src.substr(5, src.size());
		}

		return src;
	}

	TSaveLoadMap m_map = TSaveLoadMap(new std::map<std::string, std::string>);
};

CSaveLoadCheck g_SaveLoadCheck;

void SaveLoadCheck_Reset() {
	g_SaveLoadCheck.Reset();
}

bool SaveLoadCheck_Set(std::string lpszKey, std::string lpszName) {
	return g_SaveLoadCheck.emplace(lpszKey, lpszName);
}

std::string SaveLoadCheck_Get(std::string lpszKey) {
	return g_SaveLoadCheck.get(lpszKey);
}

std::string SaveLoadError(ActionNodePtr node, std::string name, std::string type)
{
	size_t len = 0x400;
	auto buffer = std::vector<char>(len);
	std::string fmt = base::a2u("YDTrigger Error: 触发器-%s: 你使用了局部变量loc_%s类型为%s,但是在其他地方声明类型为%s");
	sprintf_s(buffer.data(), len, fmt.c_str(), node->getTriggerNamePtr()->c_str(), name.c_str(), type.c_str(), SaveLoadCheck_Get(name).c_str());
	return std::string(buffer.data());
}
#include "SaveLoadCheck.h"


bool CSaveLoadCheck::emplace(const std::string key, const std::string value) {
	if (m_map->find(key) == m_map->end()) {
		m_map->emplace(key, this->Convert(value));
		return true;
	}
	else {
		return (m_map->at(key).compare(this->Convert(value)) == 0);
	}

	return false;
}

const std::string CSaveLoadCheck::get(const std::string key) {
	if (m_map->find(key) == m_map->end()) {
		return "";
	}
	else {
		return this->Convert(m_map->at(key).c_str());
	}

	return "";
}

const std::string CSaveLoadCheck::Convert(const std::string src) const {
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


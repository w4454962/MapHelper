#pragma once
#include "stdafx.h"
#include "EditorData.h"

class CSaveLoadCheck {
public:
	typedef std::shared_ptr<std::map<std::string, std::string>> TSaveLoadMap;

	bool emplace(const std::string key, const std::string value);

	const std::string get(const std::string key);

	TSaveLoadMap m_map;

private:
	const std::string Convert(const std::string src) const;
	
};

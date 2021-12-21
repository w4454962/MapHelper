#pragma once
#include "stdafx.h"
#include "ActionNode.h"
#include "unicode.h"

void   SaveLoadCheck_Reset();
bool   SaveLoadCheck_Set(std::string lpszKey, std::string lpszName);
std::string SaveLoadCheck_Get(std::string lpszKey);
void SaveLoadError(ActionNodePtr node, std::string name, std::string type);
void SaveLoadInitLog();
void SaveLoadCloseLog();
#pragma once
#include "stdafx.h"

void   SaveLoadCheck_Reset();
bool   SaveLoadCheck_Set(std::string lpszKey, std::string lpszName);
std::string SaveLoadCheck_Get(std::string lpszKey);
#pragma once

// Global define
#pragma execution_character_set("utf-8")

// System Header
#define WIN32_LEAN_AND_MEAN
//#include <third/MINT/MINT.h>
#include <strsafe.h>
#include <intrin.h>
#include <wrl.h>

// C/C++ Header
#include <type_traits>
#include <string>
#include <memory>
#include <cwctype>
#include <algorithm>

// 3rdParty Header


// Global Var/Fun define
using namespace Microsoft;

#define NullMsgBox() MessageBox(0, 0, 0, 0)

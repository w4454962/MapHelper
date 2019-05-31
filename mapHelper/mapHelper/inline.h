#pragma once

#include <cstdint>

namespace hook {
	typedef void* hook_t;
	bool install(uintptr_t* pointer_ptr, uintptr_t detour, hook_t* ph = nullptr);
	bool uninstall(hook_t* ph);
}

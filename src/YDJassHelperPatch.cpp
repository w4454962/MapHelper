
#include "YDJassHelperPatch.h"
#include "MapHelper.h"
#include <base\hook\inline.h>
#include <base\util\BinaryWriter.h>
#include <regex>

//004A1EFB  begin compile logs\currentmapscript.j
//004A21CD  start pjass  logs\currentmapscript.j

YDJassHelperPatch* g_vj_patch = nullptr;


static uintptr_t g_real_compile_start;
static uintptr_t g_real_compile_end;


YDJassHelperPatch::YDJassHelperPatch()
	:m_attach(false)
{

}
YDJassHelperPatch::YDJassHelperPatch(HANDLE process)
	:m_process(process)
{ }


YDJassHelperPatch::~YDJassHelperPatch()

{ }


void YDJassHelperPatch::insert() {
	std::string full_name = g_module_path.string();

	void* ptr = VirtualAllocEx(m_process, NULL, full_name.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!ptr) {
		print("申请内存失败 %i\n", GetLastError());
		return;
	}

	SIZE_T dwWrite = 0;
	if (!WriteProcessMemory(m_process, ptr, full_name.c_str(), full_name.size(), &dwWrite)) {
		printf("写入内存失败！\n");
		return;
	}

	HANDLE hRemoteThread = CreateRemoteThread(m_process, NULL, NULL,
		(LPTHREAD_START_ROUTINE)LoadLibrary, ptr, 0, 0);
	if (!hRemoteThread) {
		return;
	}
	::WaitForSingleObject(hRemoteThread, -1);
	::CloseHandle(hRemoteThread);
	VirtualFreeEx(m_process, ptr, 0, MEM_FREE);


}


static void __declspec(naked) insertCompileStart()
{
	__asm
	{
		pushad 
		pushfd 
		mov ecx, g_vj_patch
		call YDJassHelperPatch::onStart
		popfd 
		popad

		jmp g_real_compile_start
	}

}


static void __declspec(naked) insertCompileEnd()
{
	__asm
	{
		pushad
		pushfd
		mov ecx, g_vj_patch
		call YDJassHelperPatch::onEnd
		popfd
		popad
		jmp g_real_compile_end
	}

}



void YDJassHelperPatch::attach() {
	if (m_attach) {
		return;
	}
	//进入jasshelper进程

	
	g_real_compile_start = getAddress(0x004A1EFB);
	g_real_compile_end = getAddress(0x004A21CD);

	hook::install(&g_real_compile_start, reinterpret_cast<uintptr_t>(&insertCompileStart), m_hookCompileStart);
	hook::install(&g_real_compile_end, reinterpret_cast<uintptr_t>(&insertCompileEnd), m_hookCompileEnd);

	m_attach = true;
}


void YDJassHelperPatch::detach() {
	if (!m_attach) {
		return;
	}
	m_attach = false;
	hook::uninstall(m_hookCompileStart);
	hook::uninstall(m_hookCompileEnd);
}


uintptr_t YDJassHelperPatch::getAddress(uintptr_t addr){
	const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
	return addr - 0x00400000 + base;
}


#define BEGIN "//trigger begin"
#define END "//trigger end"

void YDJassHelperPatch::onStart() {
	//MessageBoxA(0, "Start 1", "", MB_OK);
	//编译开始时 将纯t的代码抽出 等编译结束时 再将代码插回去
	std::ifstream file("logs\\currentmapscript.j", std::ios::binary);
	if (!file.is_open())
		return;
	std::string line;

	char buf[8 * 1024];

	file.rdbuf()->pubsetbuf(buf, sizeof buf);

	BinaryWriter writer;

	std::string* trigger_code_ptr = nullptr;

	while (std::getline(file, line)) {
		 if (line.substr(0, sizeof(BEGIN) - 1) == BEGIN) {
			m_triggers.push_back(std::string());
			trigger_code_ptr = &m_triggers.back();
			writer.write_string("//trigger:" + std::to_string(m_triggers.size()));
			writer.write_string("\n");
		} else if (line.substr(0, sizeof(END) - 1) == END) {
			trigger_code_ptr = nullptr;
		} else if (trigger_code_ptr) {
			*trigger_code_ptr += line + "\n";
		}else {
			writer.write_string(line);
			writer.write_string("\n");
		}
	}
	file.close();

	std::ofstream file2("logs\\currentmapscript.j", std::ios::binary);
	if (file2.is_open()) {
		writer.finish(file2);
		file2.close();
	}
	//MessageBoxA(0, "Start 2", "", MB_OK);
}


#define TRIGGER_POS "//trigger:"

void YDJassHelperPatch::onEnd() {
	//MessageBoxA(0, "End 1", "", MB_OK);
	std::ifstream file("logs\\currentmapscript.j", std::ios::binary);
	if (!file.is_open())
		return;

	m_func_map.clear();

	std::string line;

	char buf[8 * 1024];

	file.rdbuf()->pubsetbuf(buf, sizeof buf);

	BinaryWriter writer;

	std::string* trigger_code_ptr = nullptr;

	while (std::getline(file, line)) {
		if (line.substr(0, sizeof(TRIGGER_POS) - 1) == TRIGGER_POS) {
			uint32_t pos = sizeof(TRIGGER_POS) - 1;
			std::string index = line.substr(pos, line.size() - pos);
			uint32_t i = std::atoi(index.c_str());
			if (i > 0 && i <= m_triggers.size()) {
				auto& code = m_triggers[i - 1];
				std::string code2 = std::regex_replace(code, std::regex("/\\*[^\*]+\\*/"), "");
				writer.write_string(code2);

				std::regex reg("function\\s+(\\w+)\\s+takes");
				auto words_end = std::sregex_iterator();
				auto words_begin = std::sregex_iterator(code2.begin(), code2.end(), reg);
				for (; words_begin != words_end; ++words_begin) {
					m_func_map[words_begin->str(1)] = true;
				}
			}
		} else {
			std::regex reg("//Function not found:\\s+call\\s+(\\w+)");
			auto words_end = std::sregex_iterator();
			auto words_begin = std::sregex_iterator(line.begin(), line.end(), reg);

			if (words_begin != words_end) {
				auto func_name = words_begin->str(1);
				if (m_func_map.find(func_name) != m_func_map.end()) {
					writer.write_string(std::regex_replace(line, std::regex("//Function not found\\:\\s+"), "") + "\n");
				}
			} else {
				writer.write_string(line);
				writer.write_string("\n");
			}
		}
	}
	file.close();

	std::ofstream file2("logs\\currentmapscript.j", std::ios::binary);
	if (file2.is_open()) {
		writer.finish(file2);
		file2.close();
		fs::remove("logs\\outputwar3map.j");
		fs::copy_file("logs\\currentmapscript.j", "logs\\outputwar3map.j");
	}

	
	//MessageBoxA(0, "End 2", "", MB_OK);
} 


#undef BEGIN
#undef END
#undef TRIGGER_POS


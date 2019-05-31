// mapHelper.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mapHelper.h"
#include "detours.h"
#include "windows.h"
#include "inline.h"
#include <thread>

#include <mutex>
#include <functional>
#include "TriggerEditor.h"

//#define printf //

Helper* Helper::instance = NULL;

const char* g_path;
uintptr_t g_object;
uintptr_t g_addr;

Helper::Helper()
{
	char buffer[0x400];
	GetModuleFileNameA(NULL, buffer, 0x400);

	std::string name = fs::path(buffer).filename().string();
	if (name.find("worldedit") == std::string::npos)
		return;
	instance = this; 

	m_triggerEditor = NULL;

	
	

	enableConsole();

	hookSaveMapData();
;

	
	//sprintf(buffer, "0x%x", (uintptr_t)&test);
	//std::string str = {
	//	"push 20\n"
	//	"mov eax," + std::string(buffer) + "\n"
	//	"call eax\n"
	//	"add esp,4\n"
	//	"ret\n"
	//};
	//Bytes bytes;
	//
	//int ret = asmString(str, bytes);
	//
	//if (!ret)
	//{
	//	uintptr_t addr = (uintptr_t)&bytes[0];
	//	__asm
	//	{
	//		call addr
	//	}
	//	//std_call<void>(&bytes[0]);
	//}
	
}

Helper::~Helper()
{

}
void Helper::saveMap(const char* outPath)
{

	this_call<int>(getAddress(0x0055ccd0), getEditorCurrentObject(), outPath);
}

uintptr_t Helper::getAddress(uintptr_t addr)
{
	uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
	return addr - 0x00400000 + base;
}


uintptr_t Helper::getEditorCurrentObject()
{
	uintptr_t addr = *(uintptr_t*)getAddress(0x803cb0);

	uintptr_t count = *(uintptr_t*)(addr + 0x1b0);

	uintptr_t object = *(uintptr_t*)(*(uintptr_t*)(addr + 0x1a8) + count * 4);

	if (*(uintptr_t*)(object + 0x114))
	{
		uintptr_t uknow = *(uintptr_t*)(object + 0x118);

		return *(uintptr_t*)uknow;
	}
	MessageBoxA(0, "获取不到编辑器对象", "错误！！！", MB_OK);
	return 0;
}

const char* Helper::getCurrentMapPath()
{
	uintptr_t addr = *(uintptr_t*)getAddress(0x803cb0);

	uintptr_t count = *(uintptr_t*)(addr + 0x1b0);

	uintptr_t object = *(uintptr_t*)(*(uintptr_t*)(addr + 0x1a8) + count * 4);


	return (const char*) object;
}

const char* Helper::getTempSavePath()
{
	return g_path;
}
void Helper::enableConsole()
{
	HWND h = ::GetConsoleWindow();

	if (h)
	{
		::ShowWindow(h, SW_SHOW);
	}
	else
	{
		FILE* new_file;
		::AllocConsole();
		freopen_s(&new_file, "CONIN$", "r", stdin);
		freopen_s(&new_file, "CONOUT$", "w", stdout);
		freopen_s(&new_file, "CONOUT$", "w", stderr);
	}
}


static void __declspec(naked) insertSaveMapData()
{
	
	__asm
	{
		mov g_object, esi
		lea eax, dword ptr ss : [ebp - 0x10c];
		mov g_path,eax
		pushad 
		pushfd
		mov ecx,Helper::instance
		call Helper::onSaveMap
		mov g_addr,eax
		popfd
		popad 
		jmp g_addr
	}
}



void Helper::hookSaveMapData()
{
	uintptr_t addr = getAddress(0x0055CDE6);
	printf("hook : %x\n", addr);
	hook::install(&addr, (uintptr_t)&insertSaveMapData);

}

uintptr_t Helper::onMulSaveMap(uintptr_t func)
{
	std::mutex locking;

	locking.lock();

	printf("线程执行 %x\n", func);
	this_call<int>(func, this);

	m_passStep++;

	locking.unlock();

	return 0;
}
uintptr_t Helper::onSaveMap()
{

	printf("当前地图路径%s\n", getCurrentMapPath());
	printf("保存地图路径 %s\n", getTempSavePath());


	if (!m_triggerEditor)
		m_triggerEditor = new TriggerEditor();

	TriggerData* data = *(TriggerData**)(getEditorCurrentObject() + 0x38f0);


	m_triggerEditor->loadTriggers(data);



	clock_t start = clock();
	m_isMulThread = false; 
	if (m_isMulThread)
	{
		printf("多线程保存\n");

		uintptr_t funcs[] = {
			union_cast<uintptr_t>(&Helper::saveWts),
			union_cast<uintptr_t>(&Helper::saveW3i),
			union_cast<uintptr_t>(&Helper::saveImp),
			union_cast<uintptr_t>(&Helper::saveW3e),
			union_cast<uintptr_t>(&Helper::saveShd),
			union_cast<uintptr_t>(&Helper::saveWpm),
			union_cast<uintptr_t>(&Helper::saveMmp),
			union_cast<uintptr_t>(&Helper::saveRect),
			union_cast<uintptr_t>(&Helper::saveCamara),
			union_cast<uintptr_t>(&Helper::saveSound),

			union_cast<uintptr_t>(&Helper::saveTrigger),
		};

		m_passStep = 0;
		size_t count = sizeof(funcs) / sizeof(uintptr_t);

		for (int i = 0; i < count; i++)
		{
			printf("线程 %i 执行 %x\n", i, funcs[i]);
			std::thread* threads = new std::thread(std::bind(&Helper::onMulSaveMap, this, funcs[i]));

			threads->detach();
		}

		while (1)
		{
			if (m_passStep == count)
				break;
			printf("当前流程 %i / %i\n", m_passStep, count);
			Sleep(1000);
		}
		saveMiniMap();//不可多线程
		saveObject();//不可多线程
		saveDoodas();//不可多线程
		saveUnits(); //不可多线程
		saveScript();
		saveArchive();
	}
	else
	{
		printf("单线程保存\n");
		saveWts();
		saveW3i();
		saveImp();
		saveW3e();
		saveShd();
		saveWpm();
		saveMiniMap();//不可多线程
		saveMmp();
		saveObject();//不可多线程
		saveDoodas();//不可多线程
		saveUnits(); //不可多线程
		saveRect();
		saveCamara();
		saveSound();

		m_triggerEditor->saveTriggers(getTempSavePath());
		m_triggerEditor->saveScriptTriggers(getTempSavePath());

		//saveTrigger();
		saveScript();
		saveArchive();

		
	}
	printf("地图所有数据保存完成 总耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return getAddress(0x0055D175);
}

int Helper::saveWts()
{

	printf("保存wts文本数据\n");

	clock_t start = clock() ;

	int ret = this_call<int>(getAddress(0x0055DAF0), getEditorCurrentObject(), getTempSavePath());

	printf("wts保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveW3i()
{
	printf("保存w3i地图信息数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055D280), getEditorCurrentObject(), getTempSavePath(), 1);

	printf("w3i保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}
int Helper::saveImp()
{
	printf("保存imp文件列表数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x3904);
	this_call<int>(getAddress(0x0051CEB0), object, getTempSavePath());
	int ret = this_call<int>(getAddress(0x0055DFD0), getEditorCurrentObject(), getTempSavePath());

	printf("imp 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveW3e()
{
	printf("保存w3e地形纹理数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38e0);
	int ret = this_call<int>(getAddress(0x005B0C50), object, getTempSavePath());

	printf("w3e 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;

}

int Helper::saveShd()
{
	printf("保存shd地形阴影数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055d1f0), getEditorCurrentObject(), getTempSavePath());

	printf("shd 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int Helper::saveWpm()
{
	printf("保存wpm地形路径数据\n");

	clock_t start = clock();


	std::string path = std::string(getTempSavePath()) + ".wpm";
	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38e0);
	int ret = this_call<int>(getAddress(0x005E91C0), object, path.c_str());

	printf("wpm 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveMiniMap()
{
	printf("保存minimap小地图数据\n");

	clock_t start = clock();

	std::string path = std::string(getTempSavePath()) + "Map.tga";

	int ret = this_call<int>(getAddress(0x00583200), getEditorCurrentObject(), path.c_str(), 0);

	printf("minimap 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int Helper::saveMmp()
{
	printf("保存mmp预览小文件的数据\n");

	clock_t start = clock();
	int ret = this_call<int>(getAddress(0x00583D00), getEditorCurrentObject(), getTempSavePath());

	printf("mmp 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveObject()
{
	printf("保存物编数据\n");

	clock_t start = clock();

	std::string path = std::string(getTempSavePath()) + "Map.tga";


	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x3908);
	this_call<int>(getAddress(0x00518CA0), object, getTempSavePath());

	object = *(uintptr_t*)(getEditorCurrentObject() + 0x390c);
	this_call<int>(getAddress(0x00518CA0), object, getTempSavePath());

	object = *(uintptr_t*)(getEditorCurrentObject() + 0x38f8);
	int ret = this_call<int>(getAddress(0x0051B5B0), object, getTempSavePath(), 1);

	printf("物编 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;;
}

int Helper::saveDoodas()
{
	printf("保存war3map.doo地形装饰物\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38e4);
	int ret = this_call<int>(getAddress(0x0062BAE0), object, getTempSavePath(), 1);

	printf("war3map.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int Helper::saveUnits()
{
	printf("保存war3mapUnit.doo地形单位预设数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38e8);
	int ret = this_call<int>(getAddress(0x0062BAE0), object, getTempSavePath(), 1);

	printf("war3mapUnit.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveRect()
{
	printf("保存w3r地形区域数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38ec);
	int ret = this_call<int>(getAddress(0x0062ACF0), object, getTempSavePath());

	printf("war3map.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int Helper::saveCamara()
{
	printf("保存w3c预设镜头数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38f4);
	int ret = this_call<int>(getAddress(0x005AEBB0), object, getTempSavePath());

	printf("w3c 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}
int Helper::saveSound()
{
	printf("保存w3s声音数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38fc);
	int ret = this_call<int>(getAddress(0x005EACE0), object, getTempSavePath());

	printf("w3s 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveTrigger()
{
	printf("保存wtg触发数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)(getEditorCurrentObject() + 0x38f0);
	int ret = this_call<int>(getAddress(0x00520ED0), object, getTempSavePath(), 1);

	printf("wtg 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int Helper::saveScript()
{
	printf("保存war3map.j脚本文件\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055DA80), getEditorCurrentObject(), getTempSavePath());

	printf("war3map.j 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}



int Helper::saveArchive()
{


	fs::path path = fs::path(getTempSavePath());
	path.remove_filename();

	std::string name = path.filename().string();
	if (name.length() < 4)
		return 0;

	name = name.substr(0,name.length() - 4);

	fs::path pathTemp = path / name;

	printf("打包将文件夹打包成mpq结构\n");

	printf("路径 %s\n", path.string().c_str());

	clock_t start = clock();


	int ret = this_call<int>(getAddress(0x0055D720), getEditorCurrentObject(), pathTemp.string().c_str(), 1);

	if (ret)
	{
		path.remove_filename();

		fs::path path2 = path / name;

		//移动文件目录
		ret = fast_call<int>(getAddress(0x004D0F60), pathTemp.string().c_str(), path2.string().c_str(), 1, 0);

		printf("地图打包完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

		return ret;

	}

	
	return 0;
}
#include "stdafx.h"
#include "WorldEditor.h"
#include "TriggerEditor.h"

WorldEditor::WorldEditor()
{
	m_tempPath = NULL;

	TriggerEditor* triggerEditor = TriggerEditor::getInstance();

	TriggerConfigData* configData = std_call<TriggerConfigData*>(getAddress(0x004D4DA0));
	triggerEditor->loadTriggerConfig(configData);
}

WorldEditor::~WorldEditor()
{

}

WorldEditor* WorldEditor::getInstance()
{
	static WorldEditor instance;

	return &instance;
}

uintptr_t WorldEditor::getAddress(uintptr_t addr)
{
	uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
	return addr - 0x00400000 + base;
}


EditorData* WorldEditor::getEditorData()
{
	uintptr_t addr = *(uintptr_t*)getAddress(0x803cb0);

	uintptr_t count = *(uintptr_t*)(addr + 0x1b0);

	uintptr_t object = *(uintptr_t*)(*(uintptr_t*)(addr + 0x1a8) + count * 4);

	if (*(uintptr_t*)(object + 0x114))
	{
		uintptr_t uknow = *(uintptr_t*)(object + 0x118);

		return *(EditorData**)uknow;
	}
	MessageBoxA(0, "获取不到编辑器对象", "错误！！！", MB_OK);
	return 0;
}

void WorldEditor::saveMap(const char* outPath)
{
	this_call<int>(getAddress(0x0055ccd0), getEditorData(), outPath);
}






const char* WorldEditor::getCurrentMapPath()
{
	uintptr_t addr = *(uintptr_t*)getAddress(0x803cb0);

	uintptr_t count = *(uintptr_t*)(addr + 0x1b0);

	uintptr_t object = *(uintptr_t*)(*(uintptr_t*)(addr + 0x1a8) + count * 4);


	return (const char*)object;
}

const char* WorldEditor::getTempSavePath()
{
	return m_tempPath;
}

int WorldEditor::getSoundDuration(const char* path)
{
	uint32_t param[10];
	ZeroMemory(&param, sizeof param);
	fast_call<int>(WorldEditor::getInstance()->getAddress(0x004DCFA0), path, &param);
	return param[1];
}

bool WorldEditor::getSkillObjectData(uint32_t id,uint32_t level,std::string text, std::string& value)
{
	uint32_t data = std_call<uint32_t>(getAddress(0x004D4EE0));
	char buffer[0x400];
	bool ret = this_call<bool>(getAddress(0x0050B7B0), data, id, text.c_str(), buffer, 0x400, level, 1);
	if (ret) value = buffer;
	return ret;
}

void WorldEditor::onSaveMap(const char* tempPath)
{
	m_tempPath = tempPath;

	printf("当前地图路径%s\n", getCurrentMapPath());
	printf("保存地图路径 %s\n", getTempSavePath());

	TriggerEditor* triggerEditor = TriggerEditor::getInstance();

	TriggerData* triggerData = getEditorData()->triggers;

	triggerEditor->loadTriggers(triggerData);

	int ret = MessageBoxA(0, "是否用新的保存模式保存?", "问你", MB_YESNO);

	if (ret == 6)
		printf("自定义保存模式\n");
	else 
		printf("原始保存模式\n");
	
	

	clock_t start = clock();
		
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

	if (ret == 6)
	{
		triggerEditor->saveTriggers(getTempSavePath());
		triggerEditor->saveScriptTriggers(getTempSavePath());
		triggerEditor->saveSctipt(getTempSavePath());
	}
	else
	{
		saveTrigger();
	}

	saveScript();
	saveArchive();

		
	printf("地图所有数据保存完成 总耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	m_tempPath = NULL;
}

int WorldEditor::saveWts()
{

	printf("保存wts文本数据\n");

	clock_t start = clock() ;

	int ret = this_call<int>(getAddress(0x0055DAF0), getEditorData(), getTempSavePath());

	printf("wts保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveW3i()
{
	printf("保存w3i地图信息数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055D280), getEditorData(), getTempSavePath(), 1);

	printf("w3i保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}
int WorldEditor::saveImp()
{
	printf("保存imp文件列表数据\n");

	clock_t start = clock();

	uintptr_t object = *(uintptr_t*)((uintptr_t)getEditorData() + 0x3904);
	this_call<int>(getAddress(0x0051CEB0), object, getTempSavePath());
	int ret = this_call<int>(getAddress(0x0055DFD0), getEditorData(), getTempSavePath());

	printf("imp 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveW3e()
{
	printf("保存w3e地形纹理数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x005B0C50), getEditorData()->terrain, getTempSavePath());

	printf("w3e 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;

}

int WorldEditor::saveShd()
{
	printf("保存shd地形阴影数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055d1f0), getEditorData(), getTempSavePath());

	printf("shd 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int WorldEditor::saveWpm()
{
	printf("保存wpm地形路径数据\n");

	clock_t start = clock();


	std::string path = std::string(getTempSavePath()) + ".wpm";
	int ret = this_call<int>(getAddress(0x005E91C0), getEditorData()->terrain, path.c_str());

	printf("wpm 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveMiniMap()
{
	printf("保存minimap小地图数据\n");

	clock_t start = clock();

	std::string path = std::string(getTempSavePath()) + "Map.tga";

	int ret = this_call<int>(getAddress(0x00583200), getEditorData(), path.c_str(), 0);

	printf("minimap 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int WorldEditor::saveMmp()
{
	printf("保存mmp预览小文件的数据\n");

	clock_t start = clock();
	int ret = this_call<int>(getAddress(0x00583D00), getEditorData(), getTempSavePath());

	printf("mmp 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveObject()
{
	printf("保存物编数据\n");

	clock_t start = clock();

	std::string path = std::string(getTempSavePath()) + "Map.tga";


	uintptr_t object = *(uintptr_t*)((uintptr_t)getEditorData() + 0x3908);
	this_call<int>(getAddress(0x00518CA0), object, getTempSavePath());

	object = *(uintptr_t*)((uintptr_t)getEditorData() + 0x390c);
	this_call<int>(getAddress(0x00518CA0), object, getTempSavePath());

	int ret = this_call<int>(getAddress(0x0051B5B0), getEditorData()->objects, getTempSavePath(), 1);

	printf("物编 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;;
}

int WorldEditor::saveDoodas()
{
	printf("保存war3map.doo地形装饰物\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0062BAE0), getEditorData()->doodas, getTempSavePath(), 1);

	printf("war3map.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int WorldEditor::saveUnits()
{
	printf("保存war3mapUnit.doo地形单位预设数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0062BAE0), getEditorData()->units, getTempSavePath(), 1);

	printf("war3mapUnit.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveRect()
{
	printf("保存w3r地形区域数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0062ACF0), getEditorData()->rects, getTempSavePath());

	printf("war3map.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}


int WorldEditor::saveCamara()
{
	printf("保存w3c预设镜头数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x005AEBB0), getEditorData()->cameras, getTempSavePath());

	printf("w3c 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}
int WorldEditor::saveSound()
{
	printf("保存w3s声音数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x005EACE0), getEditorData()->sounds, getTempSavePath());

	printf("w3s 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveTrigger()
{
	printf("保存wtg + wct 触发数据\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x00520ED0), getEditorData()->triggers, getTempSavePath(), 1);

	printf("wtg + wct  保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}

int WorldEditor::saveScript()
{
	printf("保存war3map.j脚本文件\n");

	clock_t start = clock();

	int ret = this_call<int>(getAddress(0x0055DA80), getEditorData(), getTempSavePath());

	printf("war3map.j 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return ret;
}



int WorldEditor::saveArchive()
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


	int ret = this_call<int>(getAddress(0x0055D720), getEditorData(), pathTemp.string().c_str(), 1);

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
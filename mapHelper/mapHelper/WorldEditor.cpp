#include "stdafx.h"
#include "WorldEditor.h"
#include "TriggerEditor.h"
#include "singleton.h"
#include "mapHelper.h"

WorldEditor::WorldEditor()
{
	m_tempPath = nullptr;

	auto& triggerEditor = get_trigger_editor();

	const auto configData = std_call<TriggerConfigData*>(getAddress(0x004D4DA0));
	triggerEditor.loadTriggerConfig(configData);


}

WorldEditor::~WorldEditor()
= default;

//WorldEditor* WorldEditor::getInstance()
//{
//	static WorldEditor instance;
//
//	return &instance;
//}

uintptr_t WorldEditor::getAddress(uintptr_t addr)
{
	const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
	return addr - 0x00400000 + base;
}


EditorData* WorldEditor::getEditorData()
{
	uintptr_t addr = *(uintptr_t*)getAddress(0x803cb0);
	uintptr_t count = 0;
	if(!Helper::IsEixt())
	{
		count = *(uintptr_t*)(addr + 0x1b0);
	}
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

	printf("当前地图路径%X\n", object);
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
	auto& v_we = get_world_editor();
	fast_call<int>(v_we.getAddress(0x004DCFA0), path, &param);
	return param[1];
}

std::string WorldEditor::getConfigData(const std::string& parentKey, const std::string& childKey, int index)
{
	char buffer[0x100];
	bool result = fast_call<uint32_t>(getAddress(0x004D1EC0), parentKey.c_str(), childKey.c_str(),buffer, 0x100, index);
	if (result)
	{
		return buffer;
	}
	return std::string();
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
	//m_tmp_path = fs::path(tempPath);
	m_tempPath = tempPath;
	//printf("m_tmp_path%s\n", m_tmp_path.string().c_str());
	//memcpy(&m_tempPath, m_tmp_path.string().c_str(), m_tmp_path.string().size());

	printf("当前地图路径%s\n", getCurrentMapPath());
	printf("保存地图路径 %s\n", getTempSavePath());

	auto& triggerEditor = get_trigger_editor();

	TriggerData* triggerData = getEditorData()->triggers;

	triggerEditor.loadTriggers(triggerData);



#if defined(EMBED_YDWE)
	int ret = 6;
#else
	int ret = 0;
	auto& v_helper = get_helper();
	const auto result = v_helper.getConfig();
	if (result == -1)
	{
		ret = MessageBoxA(0, "是否用新的保存模式保存?", "问你", MB_YESNO);

		if (ret == 6)
			printf("自定义保存模式\n");
		else
			printf("原始保存模式\n");
	}
	else if (result == 1)
	{
		ret = 6;
	}
#endif

	clock_t start = clock();
		
	if (ret == 6)
	{
		
		//customSaveWts(getTempSavePath());//有bug 先不用了
		saveWts();

		saveW3i();
		saveImp();
		saveW3e();
		saveShd();
		saveWpm();
		saveMiniMap();
		saveMmp();
		saveObject();

		

		saveUnits();
		saveRect();
		saveCamara();
		saveSound();

		triggerEditor.saveTriggers(getTempSavePath());
		triggerEditor.saveScriptTriggers(getTempSavePath());
		triggerEditor.saveSctipt(getTempSavePath());

		customSaveDoodas(getTempSavePath());

		//更新标签
		updateSaveFlags();
	}
	else
	{
		saveWts();

		saveW3i();
		saveImp();
		saveW3e();
		saveShd();
		saveWpm();
		saveMiniMap();
		saveMmp();
		saveObject();

		saveDoodas();

		saveUnits();
		saveRect();
		saveCamara();
		saveSound();

		saveTrigger();
		saveScript();
	}

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

	int ret = this_call<int>(getAddress(0x0062ACF0), getEditorData()->regions, getTempSavePath());

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

	if (path.string().substr(path.string().size() - 1) == "\\")
		path = fs::path(path.string().substr(0, path.string().size() - 1));

	std::string name = path.filename().string();
	//if (name.length() < 4)
	//	return 0;

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



int WorldEditor::customSaveWts(const char* path)
{
	printf("自定义保存war3map.wts文本数据\n");

	clock_t start = clock();

	BinaryWriter writer;
	auto string_data = getEditorData()->strings;


	writer.write<uint8_t>(0xEF);
	writer.write<uint8_t>(0xBB);
	writer.write<uint8_t>(0xBF);

	std::vector<TriggerString*> list(string_data->count);
	for (size_t i = 0; i < string_data->count; i++)
	{
		TriggerString* info = &string_data->array[i];
		list[i] = info;
	}
	std::sort(list.begin(), list.end(), [&](TriggerString* a, TriggerString* b) 
	{
		return a->index < b->index;
	});

	for (auto& i : list)
	{
		writer.write_string("STRING " + std::to_string(i->index));
		writer.write_string("\r\n{\r\n");
		writer.write_string(std::string(i->str));
		writer.write_string("\r\n}\r\n\r\n");
	}
	std::ofstream out(std::string(path) + ".wts", std::ios::binary);
	writer.finish(out);
	out.close();
	printf("war3map.wts 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return 1;
}


int WorldEditor::customSaveDoodas(const char* path)
{
	printf("自定义保存war3map.doo地形装饰物\n");

	clock_t start = clock();
	auto doodas = getEditorData()->doodas; 

	auto& editor = get_trigger_editor();

	auto& variableTable = editor.variableTable;

	BinaryWriter writer; 


	writer.write_string("W3do"); 


	uint32_t write_version = 0x8; 

	writer.write<uint32_t>(write_version); 


	uint32_t write_subversion = 0xB; 

	writer.write<uint32_t>(write_subversion); 


	writer.write<uint32_t>(doodas->unit_count); 


	uint32_t object = *(uint32_t*)(((uint32_t)doodas) + 0xE0);
	uint32_t addr = *(uint32_t*)((*(uint32_t*)(doodas)) + 0xc8);

	char buffer[0x100];

	for (size_t i = 0;i<doodas->unit_count;i++)
	{
		Unit* unit = &doodas->array[i]; 

		writer.write_string(std::string(unit->name,0x4)); 
		writer.write<uint32_t>(unit->variation); 
		writer.write<float>(unit->x); 
		writer.write<float>(unit->y); 
		writer.write<float>(unit->z); 
		writer.write<float>(unit->angle);  
		writer.write<float>(unit->scale_x); 
		writer.write<float>(unit->scale_y); 
		writer.write<float>(unit->scale_z); 

		sprintf(buffer, "gg_dest_%.04s_%04d", unit->name, unit->index);

	
		//计算装饰物状态
		uint8_t flag = 0;

		
		//注释掉的这些是效率太低 没太大必要的判断 
		//判断是否在可用地图内  在边界为true

		//if (!this_call<int>(getAddress(0x005E73A0), object, &unit->x))
		//	flag = 1;
		
		//判断是否被全局变量引用 
		//if (!this_call<int>(addr, doodas, i)) 

		//这里应该判断有没有设置过掉落物品
		//if (variableTable.find(buffer) == variableTable.end())
		if (unit->item_setting_count <= 0 && unit->item_table_index == -1)
		{
			flag |= 0x2;
		}
		else
		{
			flag &= 0xfd;
		}

		//是否带飞行高度 在地形编辑器上用 ctrl + pageup or pagedown 设置过高度的装饰物
		if (*(uint8_t*)((uint32_t)unit + 0x84) & 0x8)
		{
			flag |= 0x4;
		}
		writer.write<uint8_t>(flag);
		writer.write<uint8_t>(unit->doodas_life); 
		writer.write<int32_t>(unit->item_table_index); 
		writer.write<uint32_t>(unit->item_setting_count2); 

		for (size_t a = 0; a < unit->item_setting_count2;a++)
		{
			ItemTableSetting* setting = unit->item_setting; 
			writer.write<uint32_t>(setting->info_count2); 

			for (size_t b = 0; b < setting->info_count2; b++)
			{
				ItemTableInfo* info = &setting->item_infos[b]; 
				// 空物品这坑爹玩意
				// 怕是其他位置还得改
				// 这边是直接物编单位，可破坏物那边设置的掉落
				if (*(uint32_t*)(info->name) > 0) {
					writer.write_string(std::string(info->name, info->name + 0x4));
				}
				else
					writer.write_string("\0\0\0\0");
				writer.write<uint32_t>(info->rate); 
			}
		}

		writer.write<uint32_t>(unit->index); 
	}
	uint32_t write_special_version = 0;
	writer.write<uint32_t>(write_special_version);
	writer.write<uint32_t>(doodas->special_table->special_doodas_count); 

	for (size_t i = 0; i< doodas->special_table->special_doodas_count;i++)
	{
		SpecialDoodas* unit = &doodas->special_table->special[i];
		std::string id = std::string(unit->name, 0x4);
		// 这边是装饰物
		writer.write_string(id);
		writer.write<uint32_t>(unit->variation); 
		writer.write<uint32_t>(unit->x); 
		writer.write<uint32_t>(unit->y); 
	}


	std::ofstream out(std::string(path) + ".doo", std::ios::binary);
	writer.finish(out);
	out.close();

	printf("war3map.doo 保存完成 耗时 : %f 秒\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	return 1;
}


void WorldEditor::updateSaveFlags()
{
	auto world_data = getEditorData();
	if (world_data->is_test)
		return;
	auto trigger_data = world_data->triggers;
	
	trigger_data->updage_flag = 0;
	trigger_data->variables->updage_flag = 0;
	for (size_t i = 0; i < trigger_data->categoriy_count; i++)
	{
		Categoriy* categoriy = trigger_data->categories[i];
		uint32_t trigger_count = categoriy->trigger_count;
		for (uint32_t n = 0; n < trigger_count; n++)
		{
			Trigger* trigger = categoriy->triggers[n];
			trigger->updage_flag = 0;
		}
	}

	world_data->doodas->updage_flag = 0;
}

WorldEditor& get_world_editor()
{
	return base::singleton<WorldEditor>::instance();
}

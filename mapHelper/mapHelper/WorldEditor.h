#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"


struct RandomGroup
{
	uint32_t rate;//0x0 百分比
	const char names[10][4]; //一个组可以有10个名字
};

struct RandomGroupData
{
	uint32_t unknow1;//0x0
	const char name[0x64];//0x4
	uint32_t param_count;//0x68//表示 这个组里名字的数量
	char unknow3[0x2c];//0x6c
	uint32_t group_count;//0x98
	RandomGroup* groups;//0x9c
	uint32_t unknow2;//0xA0
};//size 0xA4

struct ItemTableInfo
{
	const char name[0x4];//物品id
	uint32_t rate;//0x4 概率
};
struct ItemTableSetting
{
	uint32_t info_count;//0x0
	uint32_t info_count2;//0x4
	ItemTableInfo* item_infos;//0x8
	uint32_t unknow;//0xc
};//0x10

struct ItemTable
{
	uint32_t unknow1;//0x0
	const char name[0x64];//0x4;
	uint32_t setting_count;//0x68
	uint32_t setting_count2;//0x6c
	ItemTableSetting* item_setting;//0x70
	uint32_t unknow2;//0x74
};//size 0x78

struct EditorData
{
	char unknow1[0x38c4];// 0x0
	uint32_t random_group_count;//0x38c4随机组数量
	RandomGroupData* random_groups;//0x38c8//随机组
	char unknow2[0x8];//0x38cc
	uint32_t item_table_count;//0x38d4 物品列表数量
	ItemTable* item_table;//0x38d8		物品表
	char unknow3[0x4];//0x38dc
	void* terrain;//0x38e0
	void* doodas;//0x38e4
	void* units;//0x38e8
	void* rects;//0x38ec
	struct TriggerData* triggers;//0x38f0 //触发编辑器数据
	void* cameras; //0x38f4
	void* objects;//0x38f8
	void* sounds; //0x38fc
};

class WorldEditor
{
public:

	WorldEditor();
	~WorldEditor();
	
	static WorldEditor* getInstance();

	uintptr_t getAddress(uintptr_t addr);

	EditorData* getEditorData();

	const char* getCurrentMapPath();

	const char* getTempSavePath();

	void saveMap(const char* outPath);

	void onSaveMap(const char* tempPath);


private:

	int saveWts();
	int saveW3i();
	int saveImp();
	int saveW3e();
	int saveShd();
	int saveWpm();
	int saveMiniMap();
	int saveMmp();
	int saveObject();
	int saveDoodas();
	int saveUnits();
	int saveRect();
	int saveCamara();
	int saveSound();
	int saveTrigger();
	int saveScript();
	int saveArchive();

protected: 

	EditorData* editData;


	uintptr_t m_editorObject;
	const char* m_tempPath;
	bool m_bInit;

};
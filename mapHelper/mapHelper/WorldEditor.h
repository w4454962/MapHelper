#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"


struct Group
{
	uint32_t rate;//0x0 百分比
	const char names[10][4]; //一个组可以有10个名字
};

struct RandomGroup
{
	uint32_t unknow1;//0x0
	const char name[0x94];//0x4
	uint32_t group_count;//0x98
	Group* group;//0x9c
	uint32_t unknow2;//0xA0
};//size 0xA4

struct RandomGroupData
{
	uint32_t unknow1;//0x0
	uint32_t group_count;//0x4
	RandomGroup* array;
};

struct EditorData
{
	char unknow1[0x38c4];// 0x0
	uint32_t random_group_count;//0x38c4随机组数量
	RandomGroupData* random_groups;//0x38c8//随机组
	char unknow2[0x14];//0x38cc
	void* terrain;//0x38e0
	void* doodas;//0x38e4
	void* units;//0x38e8
	void* rects;//0x38ec
	struct TriggerData* triggers;//0x38f0
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
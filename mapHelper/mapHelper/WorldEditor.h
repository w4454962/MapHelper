#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"
#include "EditorData.h"


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

	std::string getConfigData(const std::string& parentKey, const std::string& childKey, int index = 0);

	//获取技能物编数据 技能id 等级 字段 返回值
	bool getSkillObjectData(uint32_t id, uint32_t level, std::string text, std::string& value);

	int getSoundDuration(const char* path);

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

	
	int customSaveDoodas(const char* path);

	void updateSaveFlags();
protected: 

	EditorData* editData;


	uintptr_t m_editorObject;
	const char* m_tempPath;
	bool m_bInit;

};
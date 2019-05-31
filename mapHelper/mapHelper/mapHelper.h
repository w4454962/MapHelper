#pragma once
#include "stdafx.h"




template<typename dst_type, typename src_type>
dst_type union_cast(src_type src)
{
	union {
		src_type s;
		dst_type d;
	}u;
	u.s = src;
	return u.d;
}


class Helper
{
	typedef std::vector<uint8_t> Bytes;
public:
	Helper();
	~Helper();
	
	uintptr_t getAddress(uintptr_t addr);
	uintptr_t getEditorCurrentObject();
	const char* getCurrentMapPath();
	const char* getTempSavePath();

	void saveMap(const char* outPath);

	uintptr_t onMulSaveMap(uintptr_t);
	uintptr_t onSaveMap();
	

	static Helper* instance;

private:
	

	void enableConsole();

	int asmString(std::string& input,Bytes& bytes);

	void hookSaveMapData();

	//save map
	
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
	int m_passStep;

	bool m_init;;
	bool m_isMulThread;

	uintptr_t m_editorObject;
	const char* m_outpath;

	class TriggerEditor* m_triggerEditor;
};

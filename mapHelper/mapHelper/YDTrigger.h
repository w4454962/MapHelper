#pragma once

#include "stdafx.h"
#include "TriggerEditor.h"
#include "EditorData.h"
#include "ActionNode.h"

//ר�Ÿ�������ydwe�Ĺ���

struct LocalVar
{
	std::string name;
	std::string type;
	std::string value;
};

class YDTrigger
{
public:
	YDTrigger();
	~YDTrigger();
	static YDTrigger* getInstance();

	//����ͷ�ļ�
	static void onGlobals(BinaryWriter& a_writer);
	static void onEndGlobals(BinaryWriter& a_writer);

	//ʵ��������� ����������
	bool onRegisterEvent(std::string& a_events,ActionNodePtr node);
	void onRegisterEvent2(std::string& a_events,ActionNodePtr node);

	//ÿ����������ʱ
	bool onActionToJass(std::string& a_output,ActionNodePtr a_node, std::string& a_pre_actions, bool a_nested);

	//ÿ����������ʱ
	bool onParamterToJass(Parameter* parameter, ActionNodePtr node, std::string& actions, std::string& pre_actions, bool nested);

	//���������ɺ�����ʼʱ д��ֲ�����
	void onActionsToFuncBegin(std::string& funcCode, ActionNodePtr node);
	
	//���������ɺ�������ʱ ����ֲ�����
	void onActionsToFuncEnd(std::string& funcCode, ActionNodePtr node);


	//�жϴ����Ƿ����� true Ϊ����״̬
	bool hasDisableRegister(Trigger* trigger);

	bool isEnable() const;
private:

	
	std::string setLocal(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& value, bool add = false);
	std::string getLocal(ActionNodePtr node, const std::string& name, const std::string& type);

	std::string setLocalArray(ActionNodePtr node, const  std::string& name, const std::string& type, const std::string& index, const std::string& value);
	std::string getLocalArray(ActionNodePtr node, const std::string& name, const std::string& type, const std::string& index);

	bool seachHashLocal(Parameter** parameters, uint32_t count, std::map<std::string, std::string>* mapPtr = NULL);
protected: 
	bool m_bEnable;
	bool m_isInYdweEnumUnit;
	bool m_hasAnyPlayer;

	int m_funcStack;
	std::map<Trigger*, bool> m_triggerHasDisable;


};
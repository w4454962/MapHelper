#pragma once

#include "stdafx.h"

struct TriggerType
{
	uint32_t flag;//0x0
	const char type[0x8c];//0x4
	const char value[0xc8];//0x90 配置在TriggerData文件里的TriggerTypeDefaults的默认值
	const char base_type[0x30];//0x158
	uint32_t is_import_path;//0x188
	char unknow[0x14];//0x18c
};//size 0x1a0

struct TriggerConfigData
{
	char unknow[0x1c];
	uint32_t type_count; //0x1c
	TriggerType* array;//0x20
};

struct Parameter
{
	enum Type {
		invalid = -1,
		preset,
		variable,
		function,
		string
	};
	uint32_t table; //0x0
	uint32_t unknow2; //0x4
	uint32_t typeId; //0x8 参数器类型 -1 ~3
	char type_name[0x40];//0xc 参数类型 字符串
	const char value[0x12c]; //0x4c
	struct Action* funcParam;//0x178 非0时表示 此参数包含参数
	Parameter* arrayParam;//0x17c 非0时 表示该参数是数组变量 并且拥有子参数
};

struct Action
{
	enum Type {
		event,
		condition,
		action,
		none
	};

	struct VritualTable
	{
		uint32_t unknow1;
		uint32_t unknow2;
		uint32_t(__thiscall* getType)(void* pThis);
	};
	VritualTable* table; //0x0
	char unknow1[0x8];	 //0x4
	uint32_t child_count;	//0xc
	Action** child_actions;//0x10
	char unknow2[0xc];	 //0x14
	const char name[0x100]; //0x20
	uint32_t unknow3;//0x120;
	uint32_t unknow32;//0x124;
	uint32_t param_count; // 0x128
	Parameter** parameters;//0x12c
	char unknow4[0xC];//0x130
	uint32_t enable;//0x13c 
	char unknow5[0x14];//0x140
	uint32_t child_flag;//0x154 当该条动作是子动作时为0 否则是-1
};

struct Trigger
{
	char unknow1[0xc];
	uint32_t line_count; //0xc
	Action** actions;	//0x10
	char unknow2[0x4];//0x14
	uint32_t is_comment; //0x18
	uint32_t unknow3; //0x1c
	uint32_t is_enable;  //0x20 
	uint32_t is_disable_init; //0x24
	uint32_t is_custom_srcipt;//0x28
	uint32_t is_initialize;//0x2c 应该默认都是1
	uint32_t unknow7;//0x30
	uint32_t custom_jass_size;//0x34
	const char* custom_jass_script;//0x38
	char unknow4[0x10]; //0x3c
	const char name[0x100];//0x4c
	uint32_t unknow5;//0x14c
	struct Categoriy* parent;//0x150 //该触发所在的文件夹
	const char text[0x1000];//0x154 触发文本注释 这里未知长度 随便填了个size

};

struct Categoriy
{
	uint32_t categoriy_id;
	const char categoriy_name[0x10C];
	uint32_t has_change; // 0x110
	uint32_t unknow2; // 0x114
	uint32_t is_comment; // 0x118
	char unknow[0x14];// 0x11c
	uint32_t trigger_count;//0x130 当前别类中的触发器数量
	Trigger** triggers;		//0x134
};

struct VariableData
{
	uint32_t unknow1;	//0x0 未知 总是1
	uint32_t is_array;	//0x4
	uint32_t array_size;//0x8
	uint32_t is_init;	//0xc
	const char type[0x1E];//0x10
	const char name[0x64];//0x2e
	const char value[0x12e];//0x92
};

struct Variable
{
	char unknow1[0x8];		//0x0
	uint32_t globals_count;//0x8 包含gg_ 触发器 地形预设数据的全局变量
	VariableData* array; //0xc
};

struct TriggerData
{
	uint32_t unknow1;		//0x0
	uint32_t trigger_count; // 0x4	所有触发器数量
	char unknow2[0xC];		// 0x8
	uint32_t categoriy_count; //0x14 
	Categoriy** categories;	  //0x18
	uint32_t unknow3;		 //0x1c
	Variable* variables;    //0x20
	char unknow4[0x10]; // 0x24
	const char global_jass_comment[0x800];//0x34
	uint32_t unknow5; //0x834
	uint32_t globals_jass_size; //0x838
	const char* globals_jass_script;//0x83c
};



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

struct UnitItem
{
	uint32_t slot_id;//0x0
	const char name[0x4];//0x4 物品id
};//size 0x8

struct UnitSkill
{
	const char name[0x4];//0x0 技能id
	uint32_t is_enable;//0x4 开关 是否初始化激活该技能
	uint32_t level;//0x8 初始化等级
};//size 0xc

struct Unit
{
	uint32_t unknow1;//0x0
	const char name[0x4];//0x4 单位物编id
	uint32_t variation;//0x8 样式
	float x;//0xc	//在地形中的坐标
	float y;//0x10
	float z;//0x14
	float angle;//0x18 弧度制 要转回角度 * 180 / pi = 角度制
	float scale_x;//0x1c 
	float scale_y;//0x20
	float scale_z;//0x24
	float sacle;//0x28
	char unknow2[0x54];//0x2c
	uint32_t color;//0x80 颜色 当警戒范围-1时是0xFFFFFFFF 是-2时是0xFF1010FF
	char unknow24[0x3]; //0x84
	uint8_t type;//0x87 类型 物品是1
	char unknow22[0x34];//0x88
	uint32_t player_id;//0xbc
	uint32_t unknow13;//0xc0
	uint32_t health;//0xc4	生命百分比 最小1 大于或等于100则为负1
	uint32_t mana;//0xc8	魔法百分比 最小1 大于或等于100则为负1
	uint32_t level;//0xcc	等级
	uint32_t state_str;//0xd0 属性 力量
	uint32_t state_agi;//0xd4 属性 敏捷
	uint32_t state_int;//0xd8 属性 智力
	uint32_t item_table_index;//0xdc 物品表的索引
	uint32_t item_setting_count;//0xe0
	uint32_t item_setting_count2;//0xe4
	ItemTableSetting* item_setting;//0xe8
	char unknow4[0x8];//0xec
	float warning_range;//0xf4 警戒范围 -1 是普通 -2 是营地
	uint32_t item_count;//0xf8
	uint32_t item_count2;//0xfc
	UnitItem* items;//0x100
	uint32_t unknow14;//0x104
	uint32_t skill_count;//0x108
	uint32_t skill_count2;//0x10c
	UnitSkill* skills;//0x110
	char unknow3[0x4];//0x114

	//0x118 随机物品模式 0 为任何物品 指定 等级跟类型  1 来自随机组 2 是来自自定义列表
	uint32_t random_item_mode;//0x118

	uint8_t random_item_level;//0x11c
	char unknow23[0x2];//0x11d
	uint8_t random_item_type;//0x11f

	uint32_t random_group_index;//0x120 随机组的id 当随机物品模式为1时取用
	uint32_t random_group_child_index; // 0x124随机组 子项位置 如果不存在则为-1

	uint32_t random_item_count;//0x128
	uint32_t random_item_count2;//0x12c
	ItemTableInfo* random_items;//0x130

	char unknow25[0x18];//0x134
	uint32_t doodas_life;// 0x14c 可破坏物的生命
	char unknow21[0x8];//0x150
	uint32_t index;//0x158 全局预设变量的id
	char unknow28[0x2c];//0x15c
};//size 0x188


struct SpecialDoodas //特殊装饰物  例如 破坏的地面 山洞悬崖之类的
{
	const char name[0x4];//0x0 id 
	uint32_t variation;//0x4 
	uint32_t x;//0x8
	uint32_t y;//0xc
	char unknow[0x18];//0x10
};//size 0x28

struct  SpecialDoodasTable
{
	char unknow2[0x140]; //0x0
	uint32_t special_doodas_count;//0x140
	SpecialDoodas* special;//0x144
};
struct UnitData
{
	char unknow[0x5c];//0x0
	uint32_t unit_count;//0x5c
	Unit* array;//0x60
	char unknow2[0x7c]; //0x64
	SpecialDoodasTable* special_table;//0xe0
};


struct Sound
{
	const char name[0x64];//0x0 带gg_snd_ 前缀的全局变量名
	const char file[0x104];//0x64
	const char effect[0x34];//0x168
	uint32_t flag;//0x19c  1 | 2 | 4   1是是否循环 2 是否3D音效 4 超出范围停止
	uint32_t fade_out_rate;//0x1a0 淡出率
	uint32_t fade_in_rate;//0x1a4 淡入率
	uint32_t volume;//0x1a8 音量
	float pitch;//0x1ac 速率 = 音调
	char unknow1[0x8];//0x1b0
	uint32_t channel;//0x1b8 通道
	float min_range;//0x1bc 最小衰减范围
	float max_range;//0x1c0 最大衰减范围
	float distance_cutoff;//0x1c4 截断距离
	float inside;//0x1c8
	float outside;//0x1cc
	uint32_t outsideVolume;//0x1d0
	float x;//0x1d4
	float y;//0x1d8
	float z;//0x1dc
};//size 0x1E0

struct SoundData
{
	uint32_t unknow1;//0x0
	uint32_t sound_count;//0x4
	Sound* array;//0x8
};


struct MapInfo
{
	char unknow1[0xc4];
	float minY;//0xc4 地图最小y
	float minX;//0xc8 地图最小x
	float maxY;//0xcc 地图最大Y
	float maxX;//0xd0 地图最大X
};

struct Region
{
	uint32_t unknow1;//0x0 
	const char name[0x80];//0x4;
	int bottom;//0x84 下 需要地图  * 32 - 地图最小坐标
	int left;//0x88 左
	int top;//0x8c上
	int right;//0x90右
	MapInfo* info;//0x94
	char unknow3[0x44];//98
	char weather_id[0x4];//0xdc
	char unknow[0x8];//0xe0
	char sound_name[0x64];//0xe8
	uint32_t color;//0x14c
};

struct RegionData
{
	char unknow[0x60];//0x0
	uint32_t region_count;//0x60
	Region** array; //0x64
};


struct Camera
{
	uint32_t unknow1;//0x0
	float x;//0x4 
	float y;//0x8
	float z_offset;//0xc 高度偏移
	float rotation;//0x10  z轴旋转
	float angle_of_attack;//0x14 x轴旋转水平
	float target_distance;//0x18 镜头距离
	float roll;//0x1c 滚动(横侧角)
	float of_view;//0x20 观察区域
	float farz;//0x24 远景截断
	float unknow2;//0x28
	const char name[0x50];//0x2c
};//size 0x7c

struct CameraData
{
	char unknow[0x144]; //0x0
	uint32_t camera_count; //0x144
	Camera* array;//0x148
};

struct PlayerData
{
	uint32_t internal_number;//0x0
	uint32_t controller_id;//0x4 控制者id  0无 1玩家 2电脑 3中立 4可营救的
	uint32_t race;//0x8 种族 
	uint32_t is_lock;//0xc 是否锁定开始点
	const char name[0x20];//0x10
	char unknow2[0x4c];//0x30
	uint32_t low_level;//0x7c 低优先级  & 玩家id的2次幂 判断该玩家是否是低优先级
	uint32_t height_level;//0x80 高优先级  & 玩家id的2次幂 判断该玩家是否是高优先级
}; //size 80

struct SteamData
{
	uint32_t force_flags;//0x0 队伍标签 记录各项同盟设置
	uint32_t player_masks;//0x4 玩家记录着 这个队伍里有哪些玩家
	const char name[0x64];//0x8
};//0x6c

struct EditorData
{
	uint32_t map_version;//0x0 地图版本
	uint32_t map_save_count;//0x4 地图保存次数
	const char map_name[0x6D];// 0x8  地图名字
	const char suggested_players[0x31];//0x75 建议玩家数
	const char author[0x61];//0xa6 作者名
	const char description[0x301];//0x107 地图说明
	char unknow13[0x18];//0x408

	//镜头区域的坐标
	float camera_left_bottom_x;//0x420
	float camera_left_bottom_y;//0x424
	float camera_right_top_x;//0x428
	float camera_right_top_y;//0x42c
	float camera_left_top_x;//0x430
	float camera_left_top_y;//0x434
	float camera_right_bottom_x;//0x438
	float camera_right_bottom_y;//0x43c

	uint32_t unknow14;//0x440
	uint8_t tileset;//0x444 
	char unknow1[0x343f];// 0x445
	uint32_t player_count;//0x3884
	PlayerData* players;//0x3888
	char unknow11[0x8];//0x388c
	uint32_t steam_count;//0x3894
	SteamData* steams;//0x3898
	char unknow12[0x28];//0x389c
	uint32_t random_group_count;//0x38c4随机组数量
	RandomGroupData* random_groups;//0x38c8//随机组
	char unknow2[0x8];//0x38cc
	uint32_t item_table_count;//0x38d4 物品列表数量
	ItemTable* item_table;//0x38d8		物品表
	char unknow3[0x4];//0x38dc
	void* terrain;//0x38e0
	UnitData* doodas;//0x38e4
	UnitData* units;//0x38e8
	RegionData* regions;//0x38ec
	struct TriggerData* triggers;//0x38f0 //触发编辑器数据
	CameraData* cameras; //0x38f4
	void* objects;//0x38f8
	SoundData* sounds; //0x38fc
};

#pragma once

#ifdef ENABLE_GROWTH_PET_SYSTEM
#include "char.h"

typedef std::tuple<float, float, BYTE> TGrowthPetBonusInc;
typedef std::map<BYTE, std::pair<DWORD, std::vector<BYTE>>> TGrowthPetSkill;

enum EGrowthPetSystemType
{
	GROWTH_PET_TYPE_CLUMSY,			// 0
	GROWTH_PET_TYPE_INQUISITIVE,	// 1
	GROWTH_PET_TYPE_PEPPY,			// 2
	GROWTH_PET_TYPE_BRAVE,			// 3
	GROWTH_PET_TYPE_VERY_BRAVE,		// 4
	GROWTH_PET_TYPE_LEGENDARY,		// 5
	GROWTH_PET_TYPE_MYTHICAL,		// 6
	GROWTH_PET_TYPE_PRISMATIC,		// 7
	
	GROWTH_PET_TYPE_MAX,
};

enum EGrowthPetSystemExpType
{
	GROWTH_PET_EXP_MOB,
	GROWTH_PET_EXP_ITEM,
	
	// GROWTH_PET_EXP_MAX,
};

enum EGrowthPetSystemFeedType
{
	GROWTH_PET_FEED_TIME,
	GROWTH_PET_FEED_EVOLUTION,
	GROWTH_PET_FEED_EXP_ITEM,
};

enum EGrowthPetSystemEvolutionType
{
	GROWTH_PET_EVOLUTION_YOUNG,
	GROWTH_PET_EVOLUTION_WILD,
	GROWTH_PET_EVOLUTION_BRAVE,
	GROWTH_PET_EVOLUTION_HEROIC,
	
	GROWTH_PET_EVOLUTION_MAX,
};

/// Configuration ///
enum EGrowthPetSystem
{
	///
	PET_BONUS_COUNT = 3,
	PET_SKILL_COUNT = 3,
	PET_EGG_ITEM_VNUM_DIFF = 300,
	PET_SOCKET_TIME = 0,
	PET_SOCKET_LEVEL = 1,
	PET_SOCKET_EVOLUTION = 2,
	PET_FEED_SLOT_COUNT = 10,
	///
	
	// Expire time (in days)
	PET_EXPIRE_TIME_MIN = 1,
	PET_EXPIRE_TIME_MAX = 14,
	
	// Attributes
	PET_ATTRIBUTE_1_TYPE = POINT_MAX_HP,
	PET_ATTRIBUTE_2_TYPE = POINT_DEF_GRADE_BONUS,
	PET_ATTRIBUTE_3_TYPE = POINT_ATT_GRADE_BONUS,
	
	// Skills
	PET_SKILL_MAX_VALUE = 20,
	PET_SKILL_TRAIN_CHANCE = 75,
	PET_SKILL_COST = 10000000LL,
	PET_CLEAR_SKILL_ITEM_VNUM = 30196,
	
	// Hatching
	PET_HATCHING_PRICE = 100000LL,
	
	// Exp
	PET_ITEM_EXP_PERC = 10,
	PET_ITEM_EXP_SPECIAL_VNUM = 55005,
	
	// Feeding
	PET_FEED_EGG_OR_SUMMONABLE_ITEM_PERC = 5,
	PET_FEED_NUTRIENT_PERC = 50,
	
	// Evolution
	PET_EVOLUTION_LEVEL_STAGE_1 = 40,
	PET_EVOLUTION_LEVEL_STAGE_2 = 80,
	PET_EVOLUTION_LEVEL_STAGE_3 = 81,
	
	PET_EVOLUTION_SKILL_LEARN_MIN_TYPE = GROWTH_PET_EVOLUTION_HEROIC,
	
	// Change name
	PET_CHANGE_NAME_ITEM_VNUM = 30195,
	PET_CHANGE_NAME_COST = 100000LL,
	
	// Change type
	PET_CHANGE_TYPE_ITEM_VNUM = 30197,
	
	// Revive
	PET_REVIVE_ITEM_VNUM = 30198,
};

// PET_TYPE { MIN_BONUS_JUMP, MAX_BONUS_JUMP, _BONUS_INCREASE_LVL_STEP }
static std::map<BYTE, TGrowthPetBonusInc> mapGrowthPetBonusInc = {
		{ GROWTH_PET_TYPE_CLUMSY, 		{0.1f, 0.2f, 5} },
		{ GROWTH_PET_TYPE_INQUISITIVE,	{0.1f, 0.3f, 5} },
		{ GROWTH_PET_TYPE_PEPPY,		{0.1f, 0.4f, 5} },
		{ GROWTH_PET_TYPE_BRAVE,		{0.2f, 0.4f, 5} },
		{ GROWTH_PET_TYPE_VERY_BRAVE,	{0.3f, 0.6f, 5} },
		{ GROWTH_PET_TYPE_LEGENDARY,	{0.4f, 0.6f, 5} },
		{ GROWTH_PET_TYPE_MYTHICAL,		{0.5f, 0.7f, 5} },
		{ GROWTH_PET_TYPE_PRISMATIC,	{0.5f, 0.8f, 5} },
	};
	
// CURRENT_EVOLUTION { ITEM_VNUM, ITEM_COUNT }
static std::map<BYTE, std::pair<std::vector<DWORD>, std::vector<WORD>>> mapGrowthPetEvolutionItems = {
		{ GROWTH_PET_EVOLUTION_YOUNG,	{ std::vector<DWORD> {55001, 27992, 30006, 30076, 30047, 30057, 30079, 30022, 30046},	std::vector<WORD> {10, 10, 10, 10, 10, 10, 10, 10, 10} }},
		{ GROWTH_PET_EVOLUTION_WILD,	{ std::vector<DWORD> {55002, 27993, 30015, 30091, 30055, 30090, 30009, 30014, 30060},	std::vector<WORD> {10, 10, 10, 10, 10, 10, 10, 10, 10} }},
		{ GROWTH_PET_EVOLUTION_BRAVE,	{ std::vector<DWORD> {55003, 27994, 30559, 70039, 30561, 30560, 70031, 30562, 30564},	std::vector<WORD> {10, 10, 200, 15, 25, 25, 2, 25, 25} }},
	};
	
static std::map<DWORD, TGrowthPetSkill> mapGrowthPetSkill = {
	// Pet summonable item ID
	//		 skill ID, apply, 				{ apply value }
	{ 55701, {
			{1, {POINT_RESIST_WARRIOR,		std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			{2, {POINT_RESIST_ASSASSIN,		std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			{3, {POINT_RESIST_SURA,			std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			}},
	{ 55702, {
			{1, {POINT_RESIST_WARRIOR,		std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			{2, {POINT_RESIST_ASSASSIN,		std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			{3, {POINT_RESIST_SURA,			std::vector<BYTE> {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8} }},
			}},
};

/// Namespace ///
namespace GrowthPetSystem
{
	bool LoadExpTable();
	void EggHatching(LPCHARACTER ch, const char * szName, WORD pos);
	void ProcessNewPet(LPCHARACTER ch, LPITEM item, const char * szName);
	void ChangeName(LPCHARACTER ch, const char * szName, WORD pos);
	void GetAge(LPCHARACTER ch, WORD pos);
	void Revive(LPCHARACTER ch, WORD petPos, int pos[10]);
}
///

/// Class ///
class CGrowthPetSystem
{
	public:
		CGrowthPetSystem(LPCHARACTER owner);
		~CGrowthPetSystem();

	public:
		LPCHARACTER		GetCharacter() const					{ return m_pkChar; }
		LPCHARACTER		GetOwner() const						{ return m_pkOwner; }
		LPITEM			GetSummonableItem() const				{ return m_pkItem; }
		bool			IsSummoned() const						{ return m_pkChar != nullptr; }
		
		void			Update();

		void			Summon(LPITEM pSummonItem, DWORD dwVnum);
		void			Unsummon();
		
		void			GiveExp(int iExp, BYTE bType = GROWTH_PET_EXP_MOB);
		void			Feed(BYTE bType, int wPos[PET_FEED_SLOT_COUNT]);
		void			TrainSkill(BYTE bSkillNum);
		void			ClearSkill(BYTE bSkillNum);
		void			LearnSkill(BYTE bSkillNum, WORD wItemPos);
		void			ChangeType(WORD wPetPos, WORD wItemPos);
		void			DetermineType();
		
		float			GetAttribute(BYTE idx) { return APetAttr.at(idx); }

	private:
		void			SetName(std::string name);
		void 			Follow(float fMinDistance = 50.f);
		bool			LoadPet(LPITEM pSummonItem);
		void			SavePet();
		void			SendPacket();
		void			LevelUp();
		void			SetExpireTime(time_t time);
		void			IncreaseEvolution();
		void			GiveBuff();
		
		BYTE			GetType()								{ return m_bType; };
		BYTE			GetLevel()								{ return m_bLevel; };
		BYTE			GetEvolution()							{ return m_bEvolution; };
		BYTE			GetLife()								{ return m_bLife; };
		DWORD			GetExp(BYTE expType);
		DWORD			GetMaxExp(BYTE expType);

	private:
		LPCHARACTER		m_pkChar;
		LPCHARACTER		m_pkOwner;
		LPITEM			m_pkItem;
		
		std::string		m_szName;
		BYTE			m_bType;
		BYTE			m_bLevel;
		BYTE			m_bEvolution;
		DWORD			m_dwExpMob;
		DWORD			m_dwExpItem;
		std::array<float, PET_BONUS_COUNT> APetAttr;
		std::array<TPetSkillLevel, PET_SKILL_COUNT> APetSkill;
		DWORD			m_tBirthTime;
		time_t			m_tExpireTime;
		BYTE			m_bLife;
		
		LPEVENT			m_pkGrowthPetSystemUpdateEvent;
};
///
#endif

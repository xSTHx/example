#include "stdafx.h"

#ifdef ENABLE_GROWTH_PET_SYSTEM
#include "../../common/tables.h"

#include "GrowthPetSystem.hpp"
#include "vector.h"
#include "utils.h"
#include "config.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "locale_service.h"
#include "mob_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "item.h"
#include "db.h"
#include "packet.h"

#include <time.h>
#include <fstream>

namespace GrowthPetSystem
{
	bool LoadExpTable()
	{	
		std::ifstream file(LocaleService_GetBasePath() + "/growth_pet_exp_table.txt");
		std::string line;
		
		if (!file.is_open())
			return false;
		
		int i = 0;
		while (!file.eof())
		{
			file >> line;
			str_to_number(GROWTH_PET_EXP_TABLE[i++], line.c_str());
		}
		
		file.close();
		return true;
	}
	
	void EggHatching(LPCHARACTER ch, const char * szName, WORD pos)
	{
		LPITEM pEgg = nullptr;
		
		if (pos >= INVENTORY_MAX_NUM || pos < 0)
			return;
		
		if (!(pEgg = ch->GetInventoryItem(pos)))
			return;
		
		if (pEgg->GetType() != ITEM_GROWTH_PET && pEgg->GetSubType() != GROWTH_PET_EGG)
			return;
		
		if (!check_name(szName))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_HATCHING_WRONG_NAME"));
			return;
		}
		
		if (ch->GetGold() < PET_HATCHING_PRICE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_HATCHING_NOT_ENOUGH_MONEY"));
			return;
		}
		
#ifdef __OFFLINE_SHOP_SYSTEM__
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner() || ch->GetOfflineShopGuest())
#else
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner())
#endif
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CLOSE_ALL_OTHER_WINDOWS_TO_CONTINUE"));
			return;
		}
		
		ITEM_MANAGER::instance().RemoveItem(pEgg);
		
#ifdef FULL_YANG
		ch->ChangeGold(-PET_HATCHING_PRICE);
#else
		ch->PointChange(POINT_GOLD, -PET_HATCHING_PRICE);
#endif

		LPITEM pItemReward = ch->AutoGiveItem(pEgg->GetVnum() + PET_EGG_ITEM_VNUM_DIFF);
		
		if (pItemReward)
			ProcessNewPet(ch, pItemReward, szName);
		
		ch->SetMyShopTime();
	}
	
	void ProcessNewPet(LPCHARACTER ch, LPITEM item, const char * szName)
	{
		// Expire days
		time_t expireDays = number(PET_EXPIRE_TIME_MIN, PET_EXPIRE_TIME_MAX);
		
		// Type
		BYTE bType = number(0, GROWTH_PET_TYPE_LEGENDARY);
		
		// All default attributes values
		float bonus_min = std::get<0>(mapGrowthPetBonusInc[bType]);
		float bonus_max = std::get<1>(mapGrowthPetBonusInc[bType]);
		std::vector<float> vAttr;
		
		for (int i = 0; i < PET_BONUS_COUNT; i++)
			vAttr.push_back(number(bonus_min*10, bonus_max*10));
		
		// Available skill slots
		std::vector<short int> vSkill;
		for (int i = 0; i < PET_SKILL_COUNT; i++)
			vSkill.push_back(-1);
		
		for (int i = 0; i < number(1, PET_SKILL_COUNT); i++)
			vSkill.at(i) = 0;
			
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO player%s.growth_pet_item (id, name, type, bonus0, bonus1, bonus2, skill0, skill1, skill2, birth_date, expire_date, life) VALUES (%u, '%s', %u, %f, %f, %f, %d, %d, %d, %u, %u, %d)", 
			get_table_postfix(), item->GetID(), szName, bType, (vAttr.at(0))/10, (vAttr.at(1))/10, (vAttr.at(2))/10, vSkill.at(0), vSkill.at(1), vSkill.at(2), get_global_time(),  get_global_time()+(expireDays*24*60*60), expireDays);

		delete DBManager::instance().DirectQuery(szQuery);
		
		// Item's sockets
		item->SetSocket(PET_SOCKET_TIME, get_global_time() + (expireDays*24*60*60)); // time
		item->SetSocket(PET_SOCKET_LEVEL, 1); // level
		item->SetSocket(PET_SOCKET_EVOLUTION, 0);
		
		// Item's attributes (standard attributes)
		item->AddAttribute(PET_ATTRIBUTE_1_TYPE, vAttr.at(0));
		item->AddAttribute(PET_ATTRIBUTE_2_TYPE, vAttr.at(1));
		item->AddAttribute(PET_ATTRIBUTE_3_TYPE, vAttr.at(2));
	}
	
	void ChangeName(LPCHARACTER ch, const char * szName, WORD pos)
	{
		LPITEM pItem = nullptr;
		
		if (pos >= INVENTORY_MAX_NUM || pos < 0)
			return;

		if (!(pItem = ch->GetInventoryItem(pos)))
			return;
		
		if (pItem->GetType() != ITEM_GROWTH_PET && pItem->GetSubType() != GROWTH_PET_SUMMONABLE_ITEM)
			return;

		if (!check_name(szName))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_HATCHING_WRONG_NAME"));
			return;
		}
		
		if (ch->GetGrowthPetSystem() && ch->GetGrowthPetSystem()->IsSummoned() && ch->GetGrowthPetSystem()->GetSummonableItem() == pItem)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_CHANGE_NAME_PET_IS_SUMMONED"));
			return;
		}
		
		// Gold
		if (ch->GetGold() < PET_CHANGE_NAME_COST)
			return;
		
		// Item
		if (ch->CountSpecifyItem(PET_CHANGE_NAME_ITEM_VNUM) < 1)
			return;
		
#ifdef __OFFLINE_SHOP_SYSTEM__
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner() || ch->GetOfflineShopGuest())
#else
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner())
#endif
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CLOSE_ALL_OTHER_WINDOWS_TO_CONTINUE"));
			return;
		}
		
#ifdef FULL_YANG
		ch->ChangeGold(-PET_CHANGE_NAME_COST);
#else
		ch->PointChange(POINT_GOLD, -PET_CHANGE_NAME_COST);
#endif

		ch->RemoveSpecifyItem(PET_CHANGE_NAME_ITEM_VNUM, 1);
		
		// Update query
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "UPDATE player%s.growth_pet_item SET name = '%s' WHERE id = %u LIMIT 1", get_table_postfix(), szName, pItem->GetID());
		delete DBManager::instance().DirectQuery(szQuery);
		
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_CHANGE_NAME_SUCCESS"));
		ch->SetMyShopTime();
	}
	
	void GetAge(LPCHARACTER ch, WORD pos)
	{
		LPITEM pItem = nullptr;

		if (pos >= INVENTORY_MAX_NUM || pos < 0)
			return;

		if (!(pItem = ch->GetInventoryItem(pos)))
			return;

		if (pItem->GetType() != ITEM_GROWTH_PET && pItem->GetSubType() != GROWTH_PET_SUMMONABLE_ITEM)
			return;

		if (ch->GetGrowthPetSystem() && ch->GetGrowthPetSystem()->IsSummoned() && ch->GetGrowthPetSystem()->GetSummonableItem() == pItem)
			return;

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "SELECT birth_date, expire_date FROM player%s.growth_pet_item WHERE id = %u LIMIT 1", get_table_postfix(), pItem->GetID());
		std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows <= 0)
			return;

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		time_t tBirthDate = 0;
		time_t tExpireDate = 0;

		str_to_number(tBirthDate, row[0]);
		str_to_number(tExpireDate, row[1]);

		// Return packet
		WORD days = (tExpireDate - tBirthDate) / 60 / 60 / 24;

		TPacketGCGrowthPetSetAge p;
		memset(&p, 0, sizeof(p));

		p.bHeader = HEADER_GC_GROWTH_PET_SET_AGE;
		p.wAge = days+1;

		if (ch->GetDesc())
			ch->GetDesc()->Packet(&p, sizeof(p));
	}
	
	void Revive(LPCHARACTER ch, WORD petPos, int pos[10])
	{
		LPITEM pItem = nullptr;
		
		if (petPos >= INVENTORY_MAX_NUM || petPos < 0)
			return;
		
		if (!(pItem = ch->GetInventoryItem(petPos)))
			return;
		
		if (pItem->GetType() != ITEM_GROWTH_PET && pItem->GetSubType() != GROWTH_PET_SUMMONABLE_ITEM)
			return;
		
#ifdef __OFFLINE_SHOP_SYSTEM__
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner() || ch->GetOfflineShopGuest())
#else
		if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetMyShop() || ch->GetShopOwner())
#endif
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CLOSE_ALL_OTHER_WINDOWS_TO_CONTINUE"));
			return;
		}
		
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "SELECT birth_date, expire_date, life FROM player%s.growth_pet_item WHERE id = %u LIMIT 1", get_table_postfix(), pItem->GetID());
		std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows <= 0)
			return;

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		
		time_t tBirthDate = 0;
		time_t tExpireDate = 0;
		BYTE bDays = 0;

		str_to_number(tBirthDate, row[0]);
		str_to_number(tExpireDate, row[1]);
		str_to_number(bDays, row[2]);
		
		if (get_global_time() < tExpireDate)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_REVIVE_PET_IS_NOT_DEAD"));
			return;
		}
		
		BYTE bTotal = 0;

		for (int i = 0; i < 10; i++)
		{
			if (pos[i] >= INVENTORY_MAX_NUM || pos[i] < 0)
				continue;
			
			LPITEM pNutrient = ch->GetInventoryItem(pos[i]);
			
			if (!pNutrient)
				continue;
			
			bTotal++;
			ITEM_MANAGER::instance().RemoveItem(pNutrient);
		}

		if (bTotal == 0)
			return;

		time_t tNewBirthDate = (tExpireDate - tBirthDate) * bTotal * 10 / 100;
		
		pItem->SetSocket(PET_SOCKET_TIME, get_global_time() + (bDays * 24 * 60 * 60));
		
		// Update query
		snprintf(szQuery, sizeof(szQuery), "UPDATE player%s.growth_pet_item SET birth_date = %u, expire_date = %u WHERE id = %u LIMIT 1", get_table_postfix(), get_global_time() - tNewBirthDate, get_global_time() + (bDays * 24 * 60 * 60), pItem->GetID());
		delete DBManager::instance().DirectQuery(szQuery);
		
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_REVIVE_SUCCESS"));
		ch->SetMyShopTime();
		
		ch->ChatPacket(CHAT_TYPE_COMMAND, "GrowthPetClearReviveWindow");
	}
}

// Event
extern int passes_per_sec;

EVENTINFO(growthpetsystem_event_info)
{
	CGrowthPetSystem * pGrowthPetSystem;
};

EVENTFUNC(growthpetsystem_update_event)
{
	growthpetsystem_event_info * info = dynamic_cast<growthpetsystem_event_info*>(event->info);
	
	if (!info)
	{
		sys_err("<growthpetsystem_event_info> <Factor> Null pointer");
		return 0;
	}

	CGrowthPetSystem * pGrowthPetSystem = info->pGrowthPetSystem;

	if (!pGrowthPetSystem)
		return 0;

	pGrowthPetSystem->Update();
	return PASSES_PER_SEC(1) / 2;
}
// End of event

CGrowthPetSystem::CGrowthPetSystem(LPCHARACTER owner)
{
	m_pkChar = nullptr;
	m_pkOwner = owner;
	m_pkItem = nullptr;
	m_szName = "";
	m_bLevel = 0;
	m_bType = 0;
	m_bEvolution = 0;
	m_dwExpMob = 0;
	m_dwExpItem = 0;
	APetAttr.fill(0);
	TPetSkillLevel temp;
	memset(&temp, 0, sizeof(temp));
	APetSkill.fill(temp);
	m_tBirthTime = 0;
	m_tExpireTime = 0;
	m_bLife = 0;
}

CGrowthPetSystem::~CGrowthPetSystem()
{
	if (IsSummoned())
		Unsummon();

	m_pkOwner = nullptr;
}

void CGrowthPetSystem::SetName(std::string name)
{
	if (IsSummoned())
		GetCharacter()->SetName(name);
}

bool CGrowthPetSystem::LoadPet(LPITEM pSummonItem)
{
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT name, level, type, evolution, exp_mob, exp_item, bonus0, bonus1, bonus2, skill0, skill0lv, skill1, skill1lv, skill2, skill2lv, birth_date, expire_date, life FROM player%s.growth_pet_item WHERE id = %u LIMIT 1", get_table_postfix(), pSummonItem->GetID());
	std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));
	
	if (pMsg->Get()->uiNumRows <= 0)
		return false;
	
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	int i = 0;
	
	m_szName = row[i++];
	str_to_number(m_bLevel, row[i++]);
	str_to_number(m_bType, row[i++]);
	str_to_number(m_bEvolution, row[i++]);
	str_to_number(m_dwExpMob, row[i++]);
	str_to_number(m_dwExpItem, row[i++]);
	str_to_number(APetAttr.at(0), row[i++]);
	str_to_number(APetAttr.at(1), row[i++]);
	str_to_number(APetAttr[2], row[i++]);
	str_to_number(APetSkill.at(0).dwVnum, row[i++]);
	str_to_number(APetSkill.at(0).bLevel, row[i++]);
	str_to_number(APetSkill.at(1).dwVnum, row[i++]);
	str_to_number(APetSkill.at(1).bLevel, row[i++]);
	str_to_number(APetSkill.at(2).dwVnum, row[i++]);
	str_to_number(APetSkill.at(2).bLevel, row[i++]);
	str_to_number(m_tBirthTime, row[i++]);
	str_to_number(m_tExpireTime, row[i++]);
	str_to_number(m_bLife, row[i++]);
	
	return true;
}

void CGrowthPetSystem::SavePet()
{
	char szQuery[QUERY_MAX_LEN];	
	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO player%s.growth_pet_item VALUES (%u, '%s', %u, %u, %u, %u, %u, %f, %f, %f, %d, %u, %d, %u, %d, %u, %u, %u, %u)", 
								get_table_postfix(), GetSummonableItem()->GetID(), m_szName.c_str(), GetLevel(), GetType(), m_bEvolution, m_dwExpMob, m_dwExpItem, APetAttr.at(0), 
								APetAttr.at(1), APetAttr.at(2), APetSkill.at(0).dwVnum, APetSkill.at(0).bLevel, APetSkill.at(1).dwVnum, APetSkill.at(1).bLevel, APetSkill.at(2).dwVnum, APetSkill.at(2).bLevel, 
								m_tBirthTime, m_tExpireTime, m_bLife);

	delete DBManager::instance().DirectQuery(szQuery);
}

void CGrowthPetSystem::Unsummon()
{
	if (!IsSummoned() || !GetSummonableItem())
		return;
	
	SavePet();
	GetSummonableItem()->Lock(false);

	if (m_pkChar)
		M2_DESTROY_CHARACTER(m_pkChar);
	
	if (GetOwner() && GetOwner()->FindAffect(AFFECT_GROWTH_PET))
		GetOwner()->RemoveAffect(AFFECT_GROWTH_PET);
		
	if (GetOwner())
		GetOwner()->ComputePoints();

	m_pkChar = nullptr;
	m_pkItem = nullptr;
	
	event_cancel(&m_pkGrowthPetSystemUpdateEvent);
	m_pkGrowthPetSystemUpdateEvent = nullptr;
}

void CGrowthPetSystem::Summon(LPITEM pSummonItem, DWORD dwVnum)
{
	if (!GetOwner() || GetCharacter() || !pSummonItem)
		return;
	
	if (!GetOwner()->CanSummonPet())
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_CANNOT_SUMMON_TIME"));
		return;
	}
	
	const long x = GetOwner()->GetX() + number(-100, 100);
	const long y = GetOwner()->GetY() + number(-100, 100);
	const long z = GetOwner()->GetZ();
	
	// Checking if pet is alive
	if (get_global_time() >= pSummonItem->GetSocket(PET_SOCKET_TIME))
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SUMMON_PET_IS_DEAD"));
		return;
	}
	
	// Loading pet from database
	if (!LoadPet(pSummonItem))
	{
		sys_err("Failed to load the pet from database. (mob vnum: %u)", dwVnum);
		return;
	}

	// Spawning mob
	if (!(m_pkChar = CHARACTER_MANAGER::instance().SpawnMob(dwVnum, GetOwner()->GetMapIndex(), x, y, z, false, (int)(GetOwner()->GetRotation()+180), false)))
	{
		sys_err("Failed to summon the pet. (mob vnum: %u)", dwVnum);
		return;
	}
	
	m_pkItem = pSummonItem;

	SetName(m_szName);
	m_pkChar->SetPet();
	GetCharacter()->SetEmpire(GetOwner()->GetEmpire());
	GetCharacter()->SetLevel(GetLevel());
	GetCharacter()->Show(GetOwner()->GetMapIndex(), x, y, z);
	
	// Update event (every 0.5 sec)
	if (!m_pkGrowthPetSystemUpdateEvent)
	{
		growthpetsystem_event_info * info = AllocEventInfo<growthpetsystem_event_info>();

		info->pGrowthPetSystem = this;
		m_pkGrowthPetSystemUpdateEvent = event_create(growthpetsystem_update_event, info, PASSES_PER_SEC(1) / 2);
	}
	
	GetSummonableItem()->Lock(true);
	
	GiveBuff();
	SendPacket();
	
	GetOwner()->SetPetSummonTime();
}

void CGrowthPetSystem::GiveBuff()
{
	if (mapGrowthPetSkill.find(GetSummonableItem()->GetVnum()) == mapGrowthPetSkill.end())
		return;
	
	TGrowthPetSkill skillInfo = mapGrowthPetSkill[GetSummonableItem()->GetVnum()];
	
	if (GetOwner()->FindAffect(AFFECT_GROWTH_PET))
		GetOwner()->RemoveAffect(AFFECT_GROWTH_PET);
	
	for (BYTE i = 0; i < PET_SKILL_COUNT; i++)
	{
		if (APetSkill.at(i).dwVnum <= 0)
			continue;

		auto it = skillInfo.find(APetSkill.at(i).dwVnum);
		if (it != skillInfo.end())
		{
			auto apply_id = std::get<0>(it->second);
			auto apply_value = std::get<1>(it->second);
			
			BYTE value = apply_value.at(APetSkill.at(i).bLevel - 1);
			
			GetOwner()->AddAffect(AFFECT_GROWTH_PET, apply_id, value, 0, INFINITE_AFFECT_DURATION, 0, true, true);
		}
	}

	// hp
	GetOwner()->AddAffect(AFFECT_GROWTH_PET, PET_ATTRIBUTE_1_TYPE, GetOwner()->GetHP()*APetAttr.at(0)/100, 0, INFINITE_AFFECT_DURATION, 0, true, true);

	// def
	GetOwner()->AddAffect(AFFECT_GROWTH_PET, PET_ATTRIBUTE_2_TYPE, GetOwner()->GetPoint(POINT_DEF_GRADE)*APetAttr.at(1)/100, 0, INFINITE_AFFECT_DURATION, 0, true, true);
	
	// sp
	// GetOwner()->AddAffect(AFFECT_GROWTH_PET, PET_ATTRIBUTE_3_TYPE, GetOwner()->GetSP()*APetAttr.at(2)/100, 0, INFINITE_AFFECT_DURATION, 0, true, true);

	GetOwner()->ComputePoints();
}

void CGrowthPetSystem::SendPacket()
{
	// server -> client packet
	TPacketGCGrowthPet p;
	memset(&p, 0, sizeof(p));
	
	p.bHeader = HEADER_GC_GROWTH_PET;
	p.wCell = GetSummonableItem()->GetCell();
	strcpy(p.szPetName, m_szName.c_str());
	p.bLevel = GetLevel();
	p.bEvolution = GetEvolution();
	p.dwExpMob = m_dwExpMob;
	p.dwExpMobMax = GROWTH_PET_EXP_TABLE[GetLevel()];
	p.dwExpItem = m_dwExpItem;
	p.dwExpItemMax = GROWTH_PET_EXP_TABLE[GetLevel()] / PET_ITEM_EXP_PERC;
	std::copy(std::begin(APetAttr), std::end(APetAttr), std::begin(p.APetAttr));
	std::copy(std::begin(APetSkill), std::end(APetSkill), std::begin(p.APetSkill));

	TGrowthPetSkill skillInfo = mapGrowthPetSkill[GetSummonableItem()->GetVnum()];
	
	for (BYTE i = 0; i < PET_SKILL_COUNT; i++)
	{
		if (APetSkill.at(i).dwVnum <= 0)
		{
			p.ASkillValue[i] = 0;
			continue;
		}
	
		auto apply_value = skillInfo[APetSkill.at(i).dwVnum].second;
		BYTE value = apply_value.at(APetSkill.at(i).bLevel - 1);
		
		p.ASkillValue[i] = value;
	}

	p.tBirthTime = m_tBirthTime;
	p.tExpireTime = m_tExpireTime;
	p.bLife = GetLife();

	if (GetOwner() && GetOwner()->GetDesc())
		GetOwner()->GetDesc()->Packet(&p, sizeof(p));
}

void CGrowthPetSystem::Update()
{
	if (!IsSummoned() || !GetOwner())
		return;

	float fDist = DISTANCE_APPROX(GetCharacter()->GetX() - GetOwner()->GetX(), GetCharacter()->GetY() - GetOwner()->GetY());
	int APPROACH = 200;
	bool bRun = false;

	if (fDist >= 4500.f)
	{
		float fOwnerRot = GetOwner()->GetRotation() * 3.141592f / 180.f;
		GetCharacter()->Show(GetOwner()->GetMapIndex(), GetOwner()->GetX() + (-APPROACH * cos(fOwnerRot)), GetOwner()->GetY() + (-APPROACH * sin(fOwnerRot)));
	}
	else if (fDist >= 300.0f)
	{
		if (fDist >= 900.0f)
			bRun = true;

		GetCharacter()->SetNowWalking(!bRun);
		GetCharacter()->SetLastAttacked(get_dword_time());
		
		Follow(APPROACH);
	}
	else 
		GetCharacter()->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
}

void CGrowthPetSystem::Follow(float fMinDistance)
{
	float fDist = DISTANCE_SQRT(GetOwner()->GetX() - GetCharacter()->GetX(), GetOwner()->GetY() - GetCharacter()->GetY());
	if (fDist <= fMinDistance)
		return;

	GetCharacter()->SetRotationToXY(GetOwner()->GetX(), GetOwner()->GetY());

	float fDistToGo = fDist - fMinDistance;
	
	float fx, fy;
	GetDeltaByDegree(GetCharacter()->GetRotation(), fDistToGo, &fx, &fy);
	
	if (!GetCharacter()->Goto((int)(GetCharacter()->GetX()+fx+0.5f), (int)(GetCharacter()->GetY()+fy+0.5f)) )
		return;

	GetCharacter()->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);
}

void CGrowthPetSystem::GiveExp(int iExp, BYTE bType)
{
	if (GetLevel() >= GROWTH_PET_MAX_LEVEL)
		return;
	
	if (GetExp(bType) >= GetMaxExp(bType))
		return;
	
	switch (bType)
	{
		case GROWTH_PET_EXP_MOB:
			m_dwExpMob = MIN(m_dwExpMob + iExp, GROWTH_PET_EXP_TABLE[GetLevel()]);
			break;

		case GROWTH_PET_EXP_ITEM:
			m_dwExpItem = MIN(m_dwExpItem + iExp, GROWTH_PET_EXP_TABLE[GetLevel()] / PET_ITEM_EXP_PERC);
			break;
	}
	
	// Evolution
	if ((GetLevel() == PET_EVOLUTION_LEVEL_STAGE_1 || GetLevel() == PET_EVOLUTION_LEVEL_STAGE_2) && GetExp(bType) >= GetMaxExp(bType))
		return;
	
	if (GetExp(GROWTH_PET_EXP_MOB) >= GetMaxExp(GROWTH_PET_EXP_MOB) && GetExp(GROWTH_PET_EXP_ITEM) >= GetMaxExp(GROWTH_PET_EXP_ITEM))
		LevelUp();
	
	SendPacket();
}

void CGrowthPetSystem::LevelUp()
{	
	m_bLevel++;
	
	m_dwExpMob = 0;
	m_dwExpItem = 0;

	GetCharacter()->SetLevel(GetLevel());
	GetCharacter()->UpdatePacket();

	GetSummonableItem()->SetSocket(PET_SOCKET_LEVEL, GetLevel());

	// Increasing attributes	
	float bonus_min = std::get<0>(mapGrowthPetBonusInc[GetType()]);
	float bonus_max = std::get<1>(mapGrowthPetBonusInc[GetType()]);
	BYTE inc_step = std::get<2>(mapGrowthPetBonusInc[GetType()]);
	
	if (GetLevel() % inc_step == 0)
	{
		for (BYTE i = 0; i < PET_BONUS_COUNT; i++)
		{
			DWORD dwRandom = number(bonus_min*10, bonus_max*10);
			float increase = static_cast<float>(dwRandom)/10.f;
			
			APetAttr.at(i) += increase;
			GetSummonableItem()->SetForceAttribute(i, GetSummonableItem()->GetAttributeType(i), APetAttr.at(i)*10);
		}
	}
	
	// server -> client packet
	TPacketGCPointChange p;
	memset(&p, 0, sizeof(p));

	p.header = HEADER_GC_CHARACTER_POINT_CHANGE;
	p.dwVID = GetCharacter()->GetVID();
	p.type = 1;
	p.value = GetLevel();
	p.amount = 1;

	GetCharacter()->PacketAround(&p, sizeof(p));
}

void CGrowthPetSystem::SetExpireTime(time_t time)
{
	m_tExpireTime = time;
	
	if (GetSummonableItem())
		GetSummonableItem()->SetSocket(PET_SOCKET_TIME, m_tExpireTime);
}

DWORD CGrowthPetSystem::GetExp(BYTE expType)
{
	switch (expType)
	{
		case GROWTH_PET_EXP_MOB:
			return m_dwExpMob;
		case GROWTH_PET_EXP_ITEM:
			return m_dwExpItem;
	}
	
	return 0;
}

DWORD CGrowthPetSystem::GetMaxExp(BYTE expType)
{	
	switch (expType)
	{
		case GROWTH_PET_EXP_MOB:
			return GROWTH_PET_EXP_TABLE[GetLevel()];
		case GROWTH_PET_EXP_ITEM:
			return GROWTH_PET_EXP_TABLE[GetLevel()] / PET_ITEM_EXP_PERC;
	}
	
	return 0;
}

void CGrowthPetSystem::IncreaseEvolution()
{
	if (GetEvolution() < GROWTH_PET_EVOLUTION_BRAVE)
		LevelUp();
	
	m_bEvolution++;
	GetSummonableItem()->SetSocket(PET_SOCKET_EVOLUTION, GetEvolution());
	GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_SUCCESS"));
}

void CGrowthPetSystem::Feed(BYTE bType, int wPos[PET_FEED_SLOT_COUNT])
{
	switch (bType)
	{
		// Time
		case GROWTH_PET_FEED_TIME:
		{
			for (int i = 0; i < PET_FEED_SLOT_COUNT; i++)
			{
				if (wPos[i] <= -1 || wPos[i] >= INVENTORY_MAX_NUM)
					continue;
	
				LPITEM pItem = GetOwner()->GetInventoryItem(wPos[i]);
				
				if (!pItem || pItem->isLocked())
					continue;
				
				if (pItem->GetType() != ITEM_GROWTH_PET)
					continue;
				
				if (pItem->GetSubType() != GROWTH_PET_SUMMONABLE_ITEM && pItem->GetSubType() != GROWTH_PET_EGG && pItem->GetSubType() != GROWTH_PET_NUTRIENT)
					continue;
				
				// Egg & summonable item
				if (pItem->GetSubType() == GROWTH_PET_SUMMONABLE_ITEM || pItem->GetSubType() == GROWTH_PET_EGG)
				{
					time_t tFeedTime = GetLife() * 24 * 60 * 60 * PET_FEED_EGG_OR_SUMMONABLE_ITEM_PERC / 100;
					SetExpireTime(MIN(get_global_time() + GetLife() * 24 * 60 * 60, m_tExpireTime + tFeedTime));
					
					// Delete old pet from DB
					if (pItem->GetSubType() == GROWTH_PET_SUMMONABLE_ITEM)
					{
						char szQuery[QUERY_MAX_LEN];
						snprintf(szQuery, sizeof(szQuery), "DELETE FROM player%s.growth_pet_item WHERE id = %u LIMIT 1", get_table_postfix(), pItem->GetID());
						delete DBManager::instance().DirectQuery(szQuery);
						
					}
				}
				// Nutrient
				else if (pItem->GetSubType() == GROWTH_PET_NUTRIENT)
				{
					time_t tFeedTime = GetLife() * 24 * 60 * 60 * PET_FEED_NUTRIENT_PERC / 100;
					SetExpireTime(MIN(get_global_time() + GetLife() * 24 * 60 * 60, m_tExpireTime + tFeedTime));
				}
				
				ITEM_MANAGER::instance().RemoveItem(pItem);
			}
		}
		break;
		
		// Evolution
		case GROWTH_PET_FEED_EVOLUTION:
		{
			// Check if evolution is less than max level
			if (GetEvolution() == GROWTH_PET_EVOLUTION_MAX - 1)
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_MAX_LEVEL"));
				return;
			}
			
			// Checking pet's level
			if (GetEvolution() == GROWTH_PET_EVOLUTION_YOUNG && GetLevel() < PET_EVOLUTION_LEVEL_STAGE_1)
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_LEVEL_IS_TOO_LOW_REQUIRED_LEVEL_%d"), PET_EVOLUTION_LEVEL_STAGE_1);
				return;
			}
			else if (GetEvolution() == GROWTH_PET_EVOLUTION_WILD && GetLevel() < PET_EVOLUTION_LEVEL_STAGE_2)
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_LEVEL_IS_TOO_LOW_REQUIRED_LEVEL_%d"), PET_EVOLUTION_LEVEL_STAGE_2);
				return;
			}
			else if (GetEvolution() == GROWTH_PET_EVOLUTION_BRAVE && GetLevel() < PET_EVOLUTION_LEVEL_STAGE_3)
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_LEVEL_IS_TOO_LOW_REQUIRED_LEVEL_%d"), PET_EVOLUTION_LEVEL_STAGE_3);
				return;
			}

			// Exp types have to be full
			if (GetEvolution() < GROWTH_PET_EVOLUTION_BRAVE && (GetExp(GROWTH_PET_EXP_MOB) < GetMaxExp(GROWTH_PET_EXP_MOB) || GetExp(GROWTH_PET_EXP_ITEM) < GetMaxExp(GROWTH_PET_EXP_ITEM)))
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_EXP_IS_NOT_FULL"));
				return;
			}

			// Main process 
			// auto [item_vnum, item_count] = mapGrowthPetEvolutionItems[GetEvolution()]; // c++17 only
			
			auto it = mapGrowthPetEvolutionItems.find(GetEvolution());
			if (it == mapGrowthPetEvolutionItems.end())
				return;
			
			auto item_vnum = std::get<0>(it->second);
			auto item_count = std::get<1>(it->second);
			
			// Check items
			BYTE itemCount = 0;
			for (BYTE i = 0; i < item_vnum.size(); i++)
			{
				for (int j = 0; j < PET_FEED_SLOT_COUNT; j++)
				{
					if (wPos[j] <= -1 || wPos[j] >= INVENTORY_MAX_NUM)
						continue;
					
					LPITEM pItem = GetOwner()->GetInventoryItem(wPos[j]);
					
					if (!pItem || pItem->isLocked())
						continue;
					
					if (pItem->GetVnum() == item_vnum.at(i) && pItem->GetCount() >= item_count.at(i))
					{
						itemCount++;
						continue;
					}
				}
			}
			
			if (itemCount < item_vnum.size())
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_EVOLUTION_WRONG_ITEMS"));
				return;
			}
			
			// Removing items and increasing evolution
			for (BYTE i = 0; i < item_vnum.size(); i++)
				GetOwner()->RemoveSpecifyItem(item_vnum.at(i), item_count.at(i));
			
			IncreaseEvolution();
		}
		break;
		
		case GROWTH_PET_FEED_EXP_ITEM:
		{
			DWORD totalExp = 0;
			
			for (int j = 0; j < PET_FEED_SLOT_COUNT; j++)
			{
				if (wPos[j] <= -1 || wPos[j] >= INVENTORY_MAX_NUM)
					continue;
				
				LPITEM pItem = GetOwner()->GetInventoryItem(wPos[j]);
				
				if (!pItem || pItem->isLocked())
					continue;

				if (pItem->GetType() != ITEM_GROWTH_PET || pItem->GetSubType() != GROWTH_PET_EXP)
				{
					if (pItem->GetType() != ITEM_WEAPON && pItem->GetType() != ITEM_ARMOR)
						continue;
				}

#ifdef FULL_YANG
				int exp = static_cast<int>(pItem->GetShopBuyPrice() / PET_ITEM_EXP_PERC);
#else
				int exp = pItem->GetShopBuyPrice() / PET_ITEM_EXP_PERC;
#endif

				if (pItem->GetVnum() == PET_ITEM_EXP_SPECIAL_VNUM)
					exp = GetMaxExp(GROWTH_PET_EXP_ITEM);
				
				ITEM_MANAGER::instance().RemoveItem(pItem);
				GiveExp(exp, GROWTH_PET_EXP_ITEM);
				totalExp += exp;
			}
			
			if (totalExp)
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_FEED_ITEM_SUCCESS_TOTAL_%d"), totalExp);
		}
		break;
		
		default:
			return;
	}
	
	GetOwner()->SetMyShopTime();
	
	// Refreshing all stats
	SendPacket();
	
	GetOwner()->ChatPacket(CHAT_TYPE_COMMAND, "GrowthPetClearFeedWindow");
}

void CGrowthPetSystem::TrainSkill(BYTE bSkillNum)
{
	if (!GetOwner())
		return;
	
	if (bSkillNum >= PET_SKILL_COUNT)
		return;
	
	if (APetSkill.at(bSkillNum).dwVnum <= 0)
		return;
	
	if (APetSkill.at(bSkillNum).bLevel >= PET_SKILL_MAX_VALUE)
		return;
	
	if (GetOwner()->GetGold() < PET_SKILL_COST)
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_TRAIN_NO_MONEY_%d"), PET_SKILL_COST);
		return;
	}
	
#ifdef FULL_YANG
		GetOwner()->ChangeGold(-PET_SKILL_COST);
#else
		GetOwner()->PointChange(POINT_GOLD, -PET_SKILL_COST);
#endif
	
	if (number(1, 100) <= PET_SKILL_TRAIN_CHANCE)
	{
		APetSkill.at(bSkillNum).bLevel++;

		SendPacket();
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_TRAIN_SUCCESS"));
	}
	else
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_TRAIN_FAIL"));
	
	GetOwner()->SetMyShopTime();
}

void CGrowthPetSystem::ClearSkill(BYTE bSkillNum)
{
	if (!GetOwner())
		return;
	
	if (bSkillNum >= PET_SKILL_COUNT)
		return;
	
	if (APetSkill.at(bSkillNum).dwVnum <= 0)
		return;
	
	if (GetOwner()->CountSpecifyItem(PET_CLEAR_SKILL_ITEM_VNUM) < 1)
		return;
	
	APetSkill.at(bSkillNum).dwVnum = 0;
	APetSkill.at(bSkillNum).bLevel = 0;
	
	GetOwner()->RemoveSpecifyItem(PET_CLEAR_SKILL_ITEM_VNUM, 1);
	GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_CLEAR_SUCCESS"));
	GetOwner()->SetMyShopTime();
	
	SendPacket();
}

void CGrowthPetSystem::LearnSkill(BYTE bSkillNum, WORD wItemPos)
{
	LPITEM pItem = nullptr;
	
	if (!GetOwner())
		return;
	
	if (bSkillNum >= PET_SKILL_COUNT)
		return;
	
	if (APetSkill.at(bSkillNum).dwVnum != 0)
		return;
	
	if (wItemPos >= INVENTORY_MAX_NUM || wItemPos < 0)
		return;
	
	if (!(pItem = GetOwner()->GetInventoryItem(wItemPos)))
		return;
	
	if (pItem->GetType() != ITEM_GROWTH_PET && pItem->GetSubType() != GROWTH_PET_SKILL_BOOK)
		return;
	
	if (GetEvolution() < PET_EVOLUTION_SKILL_LEARN_MIN_TYPE)
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_LEARN_SKILL_WRONG_EVOLUTION_TYPE"));
		return;
	}
	
	for (const auto & s : APetSkill)
	{
		if (s.dwVnum == pItem->GetValue(0))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_ALREADY_KNOWN"));
			return;
		}
	}
	
	APetSkill.at(bSkillNum).dwVnum = pItem->GetValue(0);
	APetSkill.at(bSkillNum).bLevel = 1;
	
	pItem->SetCount(pItem->GetCount() - 1);

	GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_SKILL_LEARN_SUCCESS"));
	GetOwner()->SetMyShopTime();
	
	SendPacket();
}

void CGrowthPetSystem::ChangeType(WORD wPetPos, WORD wItemPos)
{
	LPITEM pPetItem = nullptr;
	LPITEM pItem = nullptr;
	
	if (!GetOwner() || !GetSummonableItem())
		return;
	
	if (wPetPos >= INVENTORY_MAX_NUM || wPetPos < 0)
		return;
	
	if (wItemPos >= INVENTORY_MAX_NUM || wItemPos < 0)
		return;
	
	if (!(pPetItem = GetOwner()->GetInventoryItem(wPetPos)))
		return;
	
	if (!(pItem = GetOwner()->GetInventoryItem(wItemPos)))
		return;
	
	if (pPetItem->GetType() != ITEM_GROWTH_PET && pPetItem->GetSubType() != GROWTH_PET_SUMMONABLE_ITEM)
		return;
	
	if (pItem->GetVnum() != PET_CHANGE_TYPE_ITEM_VNUM)
		return;
	
	if (pPetItem != GetSummonableItem())
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("GROWTH_PET_CHANGE_TYPE_PET_IS_NOT_SUMMONED"));
		return;
	}
	
	BYTE bNewType = number(0, GROWTH_PET_TYPE_MAX - 1);
	
	m_bType = bNewType;
	
	// New attributes values
	float bonus_min = std::get<0>(mapGrowthPetBonusInc[bNewType]);
	float bonus_max = std::get<1>(mapGrowthPetBonusInc[bNewType]);
	BYTE inc_step = std::get<2>(mapGrowthPetBonusInc[bNewType]);
	
	for (int i = 0; i < PET_BONUS_COUNT; i++)
		APetAttr.at(i) = number(bonus_min*10, bonus_max*10) / 10.0f;

	
	for (BYTE level = 1; level <= GetLevel(); level++)
	{
		if (level % inc_step == 0)
		{
			for (BYTE i = 0; i < PET_BONUS_COUNT; i++)
			{
				DWORD dwRandom = number(bonus_min*10, bonus_max*10);
				float increase = static_cast<float>(dwRandom)/10.f;
				
				APetAttr.at(i) += increase;
			}
		}
	}
	
	GetSummonableItem()->ClearAttribute();

	GetSummonableItem()->AddAttribute(PET_ATTRIBUTE_1_TYPE, APetAttr.at(0)*10);
	GetSummonableItem()->AddAttribute(PET_ATTRIBUTE_2_TYPE, APetAttr.at(1)*10);
	GetSummonableItem()->AddAttribute(PET_ATTRIBUTE_3_TYPE, APetAttr.at(2)*10);
	
	ITEM_MANAGER::instance().RemoveItem(pItem);
	
	SendPacket();
	
	GetOwner()->SetMyShopTime();
	GetOwner()->ChatPacket(CHAT_TYPE_COMMAND, "GrowthPetChangeType %d", bNewType);
}

void CGrowthPetSystem::DetermineType()
{
	GetOwner()->ChatPacket(CHAT_TYPE_COMMAND, "GrowthPetChangeType %d", GetType());
}
#endif

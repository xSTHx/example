#include "stdafx.h"

#ifdef __ALASTOR_DUNGEON__
#include "AlastorDungeon.hpp"
#include "char.h"
#include "dungeon.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "item.h"
#include "item_manager.h"
#include "party.h"
#include "questmanager.h"
#ifdef __DUNGEON_MANAGEMENT_PANEL_ENABLE__
	#include "questlua.h"
#endif

namespace AlastorDungeonInfo
{
	const DWORD ALASTOR_EGG_VNUM = 4199;
	
	const long ALASTOR_EGG_1_SPAWN_X = 373;
	const long ALASTOR_EGG_2_SPAWN_X = 357;
	const long ALASTOR_EGG_3_SPAWN_X = 381;
	const long ALASTOR_EGG_4_SPAWN_X = 408;
	
	const long ALASTOR_EGG_1_SPAWN_Y = 153;
	const long ALASTOR_EGG_2_SPAWN_Y = 130;
	const long ALASTOR_EGG_3_SPAWN_Y = 102;
	const long ALASTOR_EGG_4_SPAWN_Y = 149;
	
	const DWORD MOB_GROUB_VNUM_START = 4101;
	const DWORD MOB_GROUB_VNUM_END = 4104;
	const BYTE REQUIRED_MOB_COUNT = 150;
	
	const DWORD MINIBOSS_1_VNUM = 4191;
	const DWORD MINIBOSS_2_VNUM = 4192;
	
	const DWORD OPEN_ITEM_VNUM = 30730;
	const int REQUIRED_NUMBER_OF_ITEM_USES = 3;
	
	const DWORD KILL_ITEM_VNUM = 30731;
	const BYTE KILL_ITEM_CHANCE = 80;
	
	const int REQUIRED_ARCHERS_COUNT = 50;
	const DWORD ARCHER_VNUM = 4104;
	
	const DWORD ALASTOR_BOSS_VNUM = 4190;
	
	time_t EXIT_TIME = 60;
	time_t TIMEOUT = 30*60;
	
}

CAlastorDungeon::CAlastorDungeon(const long & _dwMapIndex, LPDUNGEON _pDungeon, LPPARTY _pParty) : dwMapIndex(_dwMapIndex), pDungeon(_pDungeon), pParty(_pParty), iState(-1), bEnd(false), b_IsWelcomed(false)
{
	// Move to state 0
	NextState();
}

CAlastorDungeon::~CAlastorDungeon()
{
	for (auto const & s_event : s_event_collector)
		CEventFunctionHandler::instance().RemoveEvent(s_event);
}

LPCHARACTER CAlastorDungeon::SpawnMonster(const DWORD & dwVnum, const int & x, const int & y, const int & dir)
{
	LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, x, y, dir);
	if (ch)
	{
		if (dwVnum == AlastorDungeonInfo::ALASTOR_EGG_VNUM)
			ch->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_EGG, 3600, 0, true);

		s_monster_list.insert(ch->GetVID());
		return ch;
	}

	return nullptr;
}

void CAlastorDungeon::KillMonster(LPCHARACTER pMonster)
{
	if (!pMonster)
		return;

	s_monster_list.erase(pMonster->GetVID());
	pMonster->Dead();
	//M2_DESTROY_CHARACTER(pMonster);
}

void CAlastorDungeon::NextState()
{
	iState++;
	RunState(iState);
}

void CAlastorDungeon::RunState(const int & StateNum)
{
	using namespace AlastorDungeonInfo;
	
	// STATE_WELCOME
	if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_WELCOME)
	{
		RegisterEvent([this](SArgumentSupportImpl *) { if (pDungeon) {
				bEnd = true; pDungeon->ExitAll();} 
				}, ("ALASTOR_DUNGEON_FAIL_" + std::to_string(dwMapIndex)), TIMEOUT);
		
		NextState();
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_150_MOBS)
	{
		iKilledMobs = 0;
		// Spawning mobs
		for (int i = 0; i < REQUIRED_MOB_COUNT; i++)
			SpawnMonster(number(MOB_GROUB_VNUM_START, MOB_GROUB_VNUM_END), 380+number(-15, 15), 130+number(-15, 15), number(0, 3));
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_EGGS_80)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_DESTROY_EGGS_80"));

		SpawnMonster(ALASTOR_EGG_VNUM, ALASTOR_EGG_1_SPAWN_X, ALASTOR_EGG_1_SPAWN_Y, 1);
		SpawnMonster(ALASTOR_EGG_VNUM, ALASTOR_EGG_2_SPAWN_X, ALASTOR_EGG_2_SPAWN_Y, 1);
		SpawnMonster(ALASTOR_EGG_VNUM, ALASTOR_EGG_3_SPAWN_X, ALASTOR_EGG_3_SPAWN_Y, 1);
		SpawnMonster(ALASTOR_EGG_VNUM, ALASTOR_EGG_4_SPAWN_X, ALASTOR_EGG_4_SPAWN_Y, 1);
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_KILL_MINIBOSS_1)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_MINIBOSS_1"));
		
		SpawnMonster(MINIBOSS_1_VNUM, 380, 130, number(0, 3));
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_EGGS_60)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_DESTROY_EGGS_60"));

		UnlockAllStatues();
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_ITEMS)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_DROP_ITEMS"));

		// Spawning mobs
		for (int i = 0; i < REQUIRED_MOB_COUNT; i++)
			SpawnMonster(number(MOB_GROUB_VNUM_START, MOB_GROUB_VNUM_END), 380+number(-15, 15), 130+number(-15, 15), number(0, 3));
		
		iKilledMobs = 0;
		iItemUseCount = 0;
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_EGGS_40)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_DESTROY_EGGS_40"));
		
		UnlockAllStatues();
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_KILL_MINIBOSS_2)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_MINIBOSS_2"));
		
		SpawnMonster(MINIBOSS_2_VNUM, 380, 130, number(0, 3));
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_EGGS_20)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_DESTROY_EGGS_20"));
		
		UnlockAllStatues();
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_KILL_ALL_EGGS)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_ALL_EGGS"));
		
		// Spawning mobs
		for (int i = 0; i < REQUIRED_ARCHERS_COUNT; i++)
			SpawnMonster(ARCHER_VNUM, 380+number(-15, 15), 130+number(-15, 15), number(0, 3));
		
		for (auto const & s_statue : s_locked_statues_list)
			s_statue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_EGG, 3600, 0, true);;
		
		iKilledMobs = 0;
		iItemUseCount = 0;
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::STATE_KILL_ALASTOR)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_ALASTOR"));
		
		pDungeon->JumpAll_NEW(dwMapIndex, 2304+134, 3584+178);
		KillAll();
				
		SpawnMonster(ALASTOR_BOSS_VNUM, 135, 106, number(0, 3));
	}
	else if (static_cast<EAlastorDungeonStateEnum>(StateNum) == EAlastorDungeonStateEnum::END)
	{
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_END_NOTICE"));

		bEnd = true;

#ifdef __DUNGEON_MANAGEMENT_PANEL_ENABLE__
		pDungeon->SetCompletionTime();
		
		quest::FIncreaseQuestFlag f;
		f.flagname = m_dungeon_management_flag_info[11][0] + ".completed";
		f.flagname2 = m_dungeon_management_flag_info[11][0] + ".best_time";
		f.flagname2_value = pDungeon->GetCompletionTime();
		f.iDungeonID = 11;

		pDungeon->ForEachMember(f);
#endif

		CEventFunctionHandler::instance().RemoveEvent(("ALASTOR_DUNGEON_FAIL_" + std::to_string(dwMapIndex)));
		RegisterEvent([this](SArgumentSupportImpl *) { if (pDungeon) { pDungeon->ExitAll(); } }, ("ALASTOR_DUNGEON_SUCCESS_" + std::to_string(dwMapIndex)), EXIT_TIME);
	}
}

void CAlastorDungeon::RegisterHit(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	using namespace AlastorDungeonInfo;
	
	// Not started yet
	if (iState < 0)
		return;
	
	if (!pAttacker || !pVictim)
		return;
	
	if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_150_MOBS)
	{
		if (pVictim->GetHP() <= 0)
		{
			iKilledMobs++;
			s_monster_list.erase(pVictim->GetVID());
		}
		
		if (iKilledMobs >= REQUIRED_MOB_COUNT)
		{
			NextState(); //Move to state STATE_EGGS_80
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_EGGS_80 && pVictim->GetRaceNum() == ALASTOR_EGG_VNUM)
	{
		if (IsEggLocked(pAttacker, pVictim))
			return;
		
		if (pVictim->GetHPPct() <= 80)
		{
			s_locked_statues_list.insert(pVictim);

			//make it full 80%
			pVictim->SetHP(pVictim->GetMaxHP()*80/100);

			pVictim->RemoveAffect(AFFECT_STATUE);

			if (CountLockedStatues() == 4)
				NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_KILL_MINIBOSS_1 && pVictim->GetRaceNum() == MINIBOSS_1_VNUM)
	{
		if (pVictim->GetHP() <= 0)
		{
			s_monster_list.erase(pVictim->GetVID());
			NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_EGGS_60 && pVictim->GetRaceNum() == ALASTOR_EGG_VNUM)
	{
		if (IsEggLocked(pAttacker, pVictim))
			return;
		
		if (pVictim->GetHPPct() <= 60)
		{
			s_locked_statues_list.insert(pVictim);

			//make it full 60%
			pVictim->SetHP(pVictim->GetMaxHP()*60/100);

			pVictim->RemoveAffect(AFFECT_STATUE);

			if (CountLockedStatues() == 4)
				NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_ITEMS)
	{
		if (pVictim->GetHP() <= 0)
		{
			iKilledMobs++;
			s_monster_list.erase(pVictim->GetVID());
		}
		
		if (iKilledMobs >= REQUIRED_MOB_COUNT-5)
		{
			iKilledMobs = 0;
			
			LPITEM pItem = ITEM_MANAGER::instance().CreateItem(OPEN_ITEM_VNUM, 1);
			
			if (pItem)
			{
				PIXEL_POSITION pos;
				pos.x = pAttacker->GetX() + number(-200, 200);
				pos.y = pAttacker->GetY() + number(-200, 200);

				pItem->AddToGround(pAttacker->GetMapIndex(), pos);
				pItem->StartDestroyEvent();
				pItem->SetOwnership(pAttacker, 60);
			}
			
			// Spawning mobs
			for (int i = 0; i < AlastorDungeonInfo::REQUIRED_MOB_COUNT; i++)
				SpawnMonster(number(AlastorDungeonInfo::MOB_GROUB_VNUM_START, AlastorDungeonInfo::MOB_GROUB_VNUM_END), 380+number(-15, 15), 130+number(-15, 15), number(0, 3));
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_EGGS_40 && pVictim->GetRaceNum() == ALASTOR_EGG_VNUM)
	{
		if (IsEggLocked(pAttacker, pVictim))
			return;
		
		if (pVictim->GetHPPct() <= 40)
		{
			s_locked_statues_list.insert(pVictim);

			//make it full 40%
			pVictim->SetHP(pVictim->GetMaxHP()*40/100);
			
			pVictim->RemoveAffect(AFFECT_STATUE);

			if (CountLockedStatues() == 4)
				NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_KILL_MINIBOSS_2 && pVictim->GetRaceNum() == MINIBOSS_2_VNUM)
	{
		if (pVictim->GetHP() <= 0)
		{
			s_monster_list.erase(pVictim->GetVID());
			NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_EGGS_20 && pVictim->GetRaceNum() == ALASTOR_EGG_VNUM)
	{
		if (IsEggLocked(pAttacker, pVictim))
			return;
		
		if (pVictim->GetHPPct() <= 20)
		{
			s_locked_statues_list.insert(pVictim);

			//make it full 20%
			pVictim->SetHP(pVictim->GetMaxHP()*20/100);
			
			pVictim->RemoveAffect(AFFECT_STATUE);

			if (CountLockedStatues() == 4)
				NextState();
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_KILL_ALL_EGGS)
	{
		
		if (pVictim->GetHP() <= 0)
		{
			iKilledMobs++;
			s_monster_list.erase(pVictim->GetVID());
		}
		
		if (iKilledMobs >= REQUIRED_ARCHERS_COUNT-5)
		{
			iKilledMobs = 0;
			
			LPITEM pItem = ITEM_MANAGER::instance().CreateItem(KILL_ITEM_VNUM, 1);
			
			if (pItem)
			{
				PIXEL_POSITION pos;
				pos.x = pAttacker->GetX() + number(-200, 200);
				pos.y = pAttacker->GetY() + number(-200, 200);

				pItem->AddToGround(pAttacker->GetMapIndex(), pos);
				pItem->StartDestroyEvent();
				pItem->SetOwnership(pAttacker, 60);
			}
			
			// Spawning mobs
			for (int i = 0; i < REQUIRED_ARCHERS_COUNT; i++)
				SpawnMonster(ARCHER_VNUM, 380+number(-15, 15), 130+number(-15, 15), number(0, 3));
		}
	}
	else if (static_cast<EAlastorDungeonStateEnum>(iState) == EAlastorDungeonStateEnum::STATE_KILL_ALASTOR && pVictim->GetRaceNum() == ALASTOR_BOSS_VNUM)
	{
		if (pVictim->GetHP() <= 0)
		{
			s_monster_list.erase(pVictim->GetVID());
			
			if (quest::CQuestManager::instance().GetEventFlag("double_boss_dung_event") && pDungeon->GetFlag("EVENT") != 1 && number(0, 1) == 1)
			{
				pDungeon->SetFlag("EVENT", 1);
				SendNotice(LC_TEXT("ALASTOR_DUNGEON_DOUBLE_BOSS_EVENT"));
				SpawnMonster(ALASTOR_BOSS_VNUM, 135, 106, number(0, 3));
				return;
			}
			
#ifdef __DUNGEON_REWARD_ENABLE__
			if (pParty && pParty->GetLeader())
				pDungeon->DungeonRewardInitialize(pParty->GetLeader());
			else
				pDungeon->DungeonRewardInitialize(pAttacker);
#endif

			NextState();
			
			char buf[255];
			if (pParty && pParty->GetLeader())
				snprintf(buf, sizeof(buf), LC_TEXT("ALASTOR_NOTICE_ALL_PARTY %s"), pParty->GetLeader()->GetName());
			else
				snprintf(buf, sizeof(buf), LC_TEXT("ALASTOR_NOTICE_ALL %s"), pAttacker->GetName());
			BroadcastNotice(buf);
			
			pDungeon->SaveCoords(pAttacker, true);
		}
	}
}

bool CAlastorDungeon::FindFieldMonster(LPCHARACTER pMonster)
{
	if (!pMonster)
		return false;
	
	return (s_monster_list.find(pMonster->GetVID()) != s_monster_list.end() && pMonster->GetMapIndex() == dwMapIndex);
}

void CAlastorDungeon::SendNotice(const std::string & notice)
{	
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

	std::for_each(c_ref_set.begin(), c_ref_set.end(), [this, notice](LPDESC d) {
		if (d->GetCharacter() != NULL && d->GetCharacter()->IsPC() && d->GetCharacter()->GetMapIndex() == dwMapIndex)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_BIG_NOTICE, notice.c_str());});
}

void CAlastorDungeon::SendWelcomeMessage(LPCHARACTER ch, const std::string & notice)
{	
	if (ch && static_cast<AlastorDungeonInfo::EAlastorDungeonStateEnum>(iState) <= AlastorDungeonInfo::EAlastorDungeonStateEnum::STATE_150_MOBS)
		ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, notice.c_str());
	
	if (!b_IsWelcomed)
		SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_ALL_MOBS"));
	
	b_IsWelcomed = true;
}

bool CAlastorDungeon::IsEggLocked(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (!pVictim || !pAttacker)
		return true;

	if (s_locked_statues_list.find(pVictim) != s_locked_statues_list.end())
		return true;
	
	return false;
}

BYTE CAlastorDungeon::CountLockedStatues()
{
	return s_locked_statues_list.size();
}

void CAlastorDungeon::UnlockAllStatues()
{
	for (auto const & s_statue : s_locked_statues_list)
		s_statue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_EGG, 3600, 0, true);;

	s_locked_statues_list.clear();
}

void CAlastorDungeon::KillMonsterByVnum(const DWORD & dwVnum)
{
	for (auto const & VID : s_monster_list)
	{
		auto pMonster = CHARACTER_MANAGER::instance().Find(VID);

		if (pMonster && pMonster->GetRaceNum() == dwVnum)
			KillMonster(pMonster);
	}
}

bool CAlastorDungeon::UseItem(DWORD dwVnum)
{
	switch(dwVnum)
	{
		case AlastorDungeonInfo::OPEN_ITEM_VNUM:
		{
			if (static_cast<AlastorDungeonInfo::EAlastorDungeonStateEnum>(iState) != AlastorDungeonInfo::EAlastorDungeonStateEnum::STATE_ITEMS)
				return false;

			iItemUseCount++;

			if (iItemUseCount >= AlastorDungeonInfo::REQUIRED_NUMBER_OF_ITEM_USES)
			{
				NextState();
				KillAll();
			}
			else
			{
				SendNotice(LC_TEXT("ALASTOR_DUNGEON_ITEM_USE_SUCCESS"));
			}
			return true;
		}
		break;
			
		case AlastorDungeonInfo::KILL_ITEM_VNUM:
		{
			if (static_cast<AlastorDungeonInfo::EAlastorDungeonStateEnum>(iState) != AlastorDungeonInfo::EAlastorDungeonStateEnum::STATE_KILL_ALL_EGGS)
				return false;
			
			if (number(1, 100) <= AlastorDungeonInfo::KILL_ITEM_CHANCE)
			{
				SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_ITEM_USE_SUCCESS"));
				iItemUseCount++;
				
				if (iItemUseCount >= 4)
					NextState();

				return true;
			}
			else
			{
				SendNotice(LC_TEXT("ALASTOR_DUNGEON_KILL_ITEM_USE_FAIL"));
				return false;
			}
		}
		break;
	}

	return false;
}

namespace
{
	struct FKillAll
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch && !ch->IsPC() && !ch->IsPet() && !ch->IsNewPet() && ch->GetRaceNum() != AlastorDungeonInfo::ALASTOR_EGG_VNUM)
					ch->Dead();
			}
		}
	};
}

void CAlastorDungeon::KillAll()
{
	LPSECTREE_MAP pkMap = SECTREE_MANAGER::instance().GetMap(dwMapIndex);
		if (pkMap == NULL) {
			sys_err("CDungeon: SECTREE_MAP not found for #%ld", dwMapIndex);
			return;
		}
	
	FKillAll f;
	pkMap->for_each(f);
	
	//s_monster_list.clear();
	
}
#endif
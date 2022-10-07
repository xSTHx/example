#pragma once

#include <set>
#include "EventFunctionHandler.hpp"
#include "char_manager.h"

namespace AlastorDungeonInfo
{
	extern const DWORD ALASTOR_EGG_VNUM;
	extern const long ALASTOR_EGG_1_SPAWN_X;
	extern const long ALASTOR_EGG_2_SPAWN_X;
	extern const long ALASTOR_EGG_3_SPAWN_X;
	extern const long ALASTOR_EGG_4_SPAWN_X;
	extern const long ALASTOR_EGG_1_SPAWN_Y;
	extern const long ALASTOR_EGG_2_SPAWN_Y;
	extern const long ALASTOR_EGG_3_SPAWN_Y;
	extern const long ALASTOR_EGG_4_SPAWN_Y;
	extern const DWORD MOB_GROUB_VNUM_START;
	extern const DWORD MOB_GROUB_VNUM_END;
	extern const BYTE REQUIRED_MOB_COUNT;
	extern const DWORD OPEN_ITEM_VNUM;
	extern const int REQUIRED_NUMBER_OF_ITEM_USES;
	extern const int REQUIRED_ARCHERS_COUNT;
	extern const DWORD ARCHER_VNUM;
	extern const DWORD ALASTOR_BOSS_VNUM;
	
	enum class EAlastorDungeonStateEnum : int
	{
		STATE_WELCOME,
		STATE_150_MOBS,
		STATE_EGGS_80,
		STATE_KILL_MINIBOSS_1,
		STATE_EGGS_60,
		STATE_ITEMS,
		STATE_EGGS_40,
		STATE_KILL_MINIBOSS_2,
		STATE_EGGS_20,
		STATE_KILL_ALL_EGGS,
		STATE_KILL_ALASTOR,
		END,
	};
}


class CAlastorDungeon
{
	public:
		CAlastorDungeon(const long & _dwMapIndex, LPDUNGEON _pDungeon, LPPARTY _pParty);
		~CAlastorDungeon();
		
		LPDUNGEON GetDungeon() const { return pDungeon; }
		void RegisterHit(LPCHARACTER pAttacker, LPCHARACTER pVictim);
		void KillMonster(LPCHARACTER pMonster);
		void SendWelcomeMessage(LPCHARACTER ch, const std::string & notice);
		bool FindFieldMonster(LPCHARACTER pMonster);
		bool UseItem(DWORD dwVnum);
		bool IsEggLocked(LPCHARACTER pAttacker, LPCHARACTER pVictim);
		bool IsEnded() const { return bEnd; }
		
	private:
		LPCHARACTER SpawnMonster(const DWORD & dwVnum, const int & x, const int & y, const int & dir);
		void SpawnMonster(const DWORD & dwVnum, LPCHARACTER pVictim);
		void NextState();
		void SendNotice(const std::string & notice);
		void RunState(const int & StateNum);
		void UnlockAllStatues();
		void KillAll();
		
		BYTE CountLockedStatues();
		void KillMonsterByVnum(const DWORD & dwVnum);
		template <typename F>
		void RegisterEvent(F func, const std::string & s_event_name, const size_t & t_delay);

	private:
		std::set<DWORD> s_monster_list;
		std::set<LPCHARACTER> s_locked_statues_list;
		std::set<std::string> s_event_collector;
		int iDestroyedEggs;
		int iKilledMobs;
		int iKilledIdallBosses;
		long dwMapIndex;
		LPDUNGEON pDungeon;
		LPPARTY pParty;
		int iState;
		int iItemUseCount;
		bool bEnd;
		bool b_IsWelcomed;
};

// Wrapper for event handler
template <typename F>
void CAlastorDungeon::RegisterEvent(F func, const std::string & s_event_name, const size_t & t_delay)
{
	CEventFunctionHandler::instance().AddEvent(func, s_event_name, t_delay);
	s_event_collector.insert(s_event_name);
}
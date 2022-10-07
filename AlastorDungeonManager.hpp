#pragma once
#ifdef __ALASTOR_DUNGEON__
#include <unordered_map>
#include <memory>
#include "AlastorDungeon.hpp"

class CDungeon;
class CHARACTER;

class CAlastorDungeonManager : public singleton<CAlastorDungeonManager>
{
	// Helper
	struct SAlastorStruct
	{
		bool bParty;
		std::unique_ptr<CAlastorDungeon> pDungeon;

		SAlastorStruct() : bParty(false), pDungeon()
		{}

		SAlastorStruct(const bool & _bParty, CAlastorDungeon * _pDungeon) : bParty(_bParty), pDungeon(_pDungeon)
		{}

		SAlastorStruct(SAlastorStruct && o)
		{
			bParty = std::move(o.bParty);
			pDungeon = std::move(o.pDungeon);
		}

		SAlastorStruct(const SAlastorStruct & o) = delete;
	};

	public:
		CAlastorDungeonManager();
		virtual ~CAlastorDungeonManager();
		void Intialize();

		CAlastorDungeon * FindDungeonByPID(LPCHARACTER ch);
		CAlastorDungeon * FindDungeonByVID(LPCHARACTER ch);
		bool RegisterAttender(LPCHARACTER ch, LPDUNGEON pDungeon);
#ifdef __DUNGEON_MANAGEMENT_PANEL_ENABLE__
		bool RegisterAttenderByPID(DWORD PID, LPDUNGEON pDungeon);
#endif
		void RegisterHit(LPCHARACTER pAttacker, LPCHARACTER pVictim);
		bool IsEggLocked(LPCHARACTER pAttacker, LPCHARACTER pVictim);
		void EraseAttender(LPCHARACTER ch);
		void EraseDungeon(LPDUNGEON pDungeon);

	private:
		std::unordered_map<DWORD, SAlastorStruct> m_attenders_list;
};
#endif
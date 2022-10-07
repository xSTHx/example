#include "stdafx.h"

#ifdef __ALASTOR_DUNGEON__
#include "char.h"
#include "party.h"
#include "dungeon.h"
#include "AlastorDungeon.hpp"
#include "AlastorDungeonManager.hpp"

CAlastorDungeonManager::CAlastorDungeonManager()
{}

CAlastorDungeonManager::~CAlastorDungeonManager()
{
	m_attenders_list.clear();
}

void CAlastorDungeonManager::Intialize()
{
}

CAlastorDungeon * CAlastorDungeonManager::FindDungeonByPID(LPCHARACTER ch)
{
	DWORD s_val = ch->GetParty() ? ch->GetParty()->GetLeaderPID() : ch->GetPlayerID();
	auto fIt = m_attenders_list.find(s_val);
	return (fIt == m_attenders_list.end()) ? nullptr : fIt->second.pDungeon.get();
}

CAlastorDungeon * CAlastorDungeonManager::FindDungeonByVID(LPCHARACTER ch)
{
	for (auto const & rec : m_attenders_list)
	{
		if ((rec.second).pDungeon.get()->FindFieldMonster(ch))
			return (rec.second).pDungeon.get();
	}

	return nullptr;
}

void CAlastorDungeonManager::RegisterHit(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (!pAttacker || !pVictim)
		return;

	if (pVictim->IsPC())
		return;

	auto pVic = FindDungeonByVID(pVictim);
	if (pVic)
	{
		auto pAttack = pAttacker->IsPC() ? FindDungeonByPID(pAttacker) : FindDungeonByVID(pAttacker);
		if (pAttack && pVic && pAttack == pVic)
			pAttack->RegisterHit(pAttacker, pVictim);
	}
}

bool CAlastorDungeonManager::IsEggLocked(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (!pAttacker || !pVictim)
		return true;

	if (pVictim->GetRaceNum() != AlastorDungeonInfo::ALASTOR_EGG_VNUM)
		return false;

	auto pVic = FindDungeonByVID(pVictim);
	if (pVic)
	{
		auto pAttack = pAttacker->IsPC() ? FindDungeonByPID(pAttacker) : FindDungeonByVID(pAttacker);
		if (pAttack && pVic && pAttack == pVic)
			if (pAttack->IsEggLocked(pAttacker, pVictim))
				return true;
	}
	
	return false;
}

bool CAlastorDungeonManager::RegisterAttender(LPCHARACTER ch, LPDUNGEON pDungeon)
{
	if (!ch || !pDungeon)
		return false;

	auto it = m_attenders_list.find(ch->GetPlayerID());
	if (it == m_attenders_list.end())
	{
		auto pInstance = new CAlastorDungeon(pDungeon->GetMapIndex(), pDungeon, ch->GetParty() ? ch->GetParty() : nullptr);
		if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
			m_attenders_list.emplace(std::piecewise_construct, std::forward_as_tuple(ch->GetPlayerID()), std::forward_as_tuple(true, pInstance));
		else
			m_attenders_list.emplace(std::piecewise_construct, std::forward_as_tuple(ch->GetPlayerID()), std::forward_as_tuple(false, pInstance));
	}
	else
	{
		auto pParty = ch->GetParty();
		
		if (((it->second).bParty && (!pParty || m_attenders_list.find(pParty->GetLeaderPID()) == m_attenders_list.end()))
		|| (!(it->second).bParty && pParty))
			return false;
	}

	return true;
}

//DO NOT USE, only for dungeon management panel purpose
#ifdef __DUNGEON_MANAGEMENT_PANEL_ENABLE__
bool CAlastorDungeonManager::RegisterAttenderByPID(DWORD PID, LPDUNGEON pDungeon)
{
	if (!pDungeon)
		return false;

	auto it = m_attenders_list.find(PID);
	if (it == m_attenders_list.end())
	{
		auto pInstance = new CAlastorDungeon(pDungeon->GetMapIndex(), pDungeon, nullptr);
		m_attenders_list.emplace(std::piecewise_construct, std::forward_as_tuple(PID), std::forward_as_tuple(false, pInstance));
	}
	else
	{
		m_attenders_list.erase(PID);
		auto pInstance = new CAlastorDungeon(pDungeon->GetMapIndex(), pDungeon, nullptr);
		m_attenders_list.emplace(std::piecewise_construct, std::forward_as_tuple(PID), std::forward_as_tuple(false, pInstance));
	}

	return true;
}
#endif

void CAlastorDungeonManager::EraseAttender(LPCHARACTER ch)
{
	if (!ch)
		return;

	auto it = m_attenders_list.find(ch->GetPlayerID());
	if (it != m_attenders_list.end())
	{
		if ((it->second).pDungeon.get()->IsEnded())	
			m_attenders_list.erase(ch->GetPlayerID());
	}
}

void CAlastorDungeonManager::EraseDungeon(LPDUNGEON pDungeon)
{
	auto it = std::find_if(m_attenders_list.begin(), m_attenders_list.end(), [&](const std::pair<const DWORD, SAlastorStruct> & val) { return val.second.pDungeon.get()->GetDungeon() == pDungeon; });
	if (it != m_attenders_list.end())
		m_attenders_list.erase(it);
}
#endif
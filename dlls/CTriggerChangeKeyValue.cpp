/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

const int MAX_CHANGE_KEYVALUES = 16;

/**
*	@brief Changes up to ::MAX_CHANGE_KEYVALUES key values in entities with the targetname specified in this entity's target keyvalue
*	Can also change the target entity's target by specifying a changetarget value
*/
class CTriggerChangeKeyValue : public CBaseDelay
{
public:
	int ObjectCaps() override { return 0; }

	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_changeTargetName;
	int m_cTargets;
	string_t m_iKey[MAX_CHANGE_KEYVALUES];
	string_t m_iValue[MAX_CHANGE_KEYVALUES];
};

LINK_ENTITY_TO_CLASS(trigger_changekeyvalue, CTriggerChangeKeyValue);

TYPEDESCRIPTION CTriggerChangeKeyValue::m_SaveData[] =
	{
		DEFINE_FIELD(CTriggerChangeKeyValue, m_cTargets, FIELD_INTEGER),
		DEFINE_ARRAY(CTriggerChangeKeyValue, m_iKey, FIELD_STRING, MAX_CHANGE_KEYVALUES),
		DEFINE_ARRAY(CTriggerChangeKeyValue, m_iValue, FIELD_STRING, MAX_CHANGE_KEYVALUES),
		DEFINE_FIELD(CTriggerChangeKeyValue, m_changeTargetName, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeKeyValue, CBaseDelay);

bool CTriggerChangeKeyValue::KeyValue(KeyValueData* pkvd)
{
	//Make sure base class keys are handled properly
	if (FStrEq("origin", pkvd->szKeyName) || FStrEq("target", pkvd->szKeyName) || FStrEq("targetname", pkvd->szKeyName) || FStrEq("classname", pkvd->szKeyName))
	{
		return CBaseDelay::KeyValue(pkvd);
	}
	else if (FStrEq("changetarget", pkvd->szKeyName))
	{
		m_changeTargetName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	//TODO: this was missing an else and caused "changetarget" to be added here
	else if (m_cTargets < MAX_CHANGE_KEYVALUES)
	{
		char temp[256];

		UTIL_StripToken(pkvd->szKeyName, temp);
		m_iKey[m_cTargets] = ALLOC_STRING(temp);

		UTIL_StripToken(pkvd->szValue, temp);
		m_iValue[m_cTargets] = ALLOC_STRING(temp);

		++m_cTargets;

		return true;
	}

	return false;
}

void CTriggerChangeKeyValue::Spawn()
{
	//Nothing
}

void CTriggerChangeKeyValue::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	char tmpkey[128];
	char tmpvalue[128];

	CBaseEntity* target = nullptr;

	while ((target = UTIL_FindEntityByString(target, "targetname", STRING(pev->target))) != nullptr)
	{
		for (int i = 0; i < m_cTargets; ++i)
		{
			//Original code used strcpy here (unsafe)
			strncpy(tmpkey, STRING(m_iKey[i]), sizeof(tmpkey) - 1);
			tmpkey[sizeof(tmpkey) - 1] = '\0';

			strncpy(tmpvalue, STRING(m_iValue[i]), sizeof(tmpvalue) - 1);
			tmpvalue[sizeof(tmpvalue) - 1] = '\0';

			//Original code dynamically allocated kvd here (unnecessary)
			KeyValueData kvd{};

			kvd.szKeyName = tmpkey;
			kvd.szValue = tmpvalue;
			kvd.fHandled = false;
			kvd.szClassName = STRING(target->pev->classname);

			DispatchKeyValue(target->pev->pContainingEntity, &kvd);
		}

		if (!FStringNull(m_changeTargetName))
		{
			target->pev->target = m_changeTargetName;
		}
	}
}

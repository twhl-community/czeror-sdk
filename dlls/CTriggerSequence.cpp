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

const auto SF_SEQUENCE_FIRE_ONCE = 1 << 0;

class CTriggerSequence : public CBaseDelay
{
public:
	static const auto MAX_NAME_LENGTH = 255;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	CTriggerSequence();

	int ObjectCaps() override { return CLASS_NONE; }

	void KeyValue( KeyValueData* pkvd ) override;
	void Precache() override;
	void Spawn() override;

	void Restart() override;

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;

	void Think() override;

	void GoToFirstCommand();

	void AdvanceAndCheckForRepeat( int repeatCount );

	void FastForward();

	void StopSequence();

	void ExecuteFireTargets( char* fireTargetNames );

	void ExecuteKillTargets( char* killTargetNames );

	void ExecuteTextMessage( client_textmessage_t* message, int textChannel );

	void ExecuteSound( char* soundName );

	void ExecuteSpeech( char* soundName, char* speakerName, char* listenerName, float duration );

	void ExecuteSoundOrSpeech( char* soundName, char* speakerName, char* listenerName, float duration );

	void ExecuteSequenceCommand( sequenceCommandLine_s* command );

	char m_sequenceEntryName[ MAX_NAME_LENGTH + 1 ];
	float m_sequenceStartTime;

	sequenceEntry_s* m_sequenceEntry;
	sequenceCommandLine_s* m_nextCommand;

	CBaseEntity* m_activator;
	CBaseEntity* m_caller;
	USE_TYPE m_useType;
	float m_value;

	BOOL m_isBusy;
	BOOL m_isDisabled;
	BOOL m_fastForward;
};

TYPEDESCRIPTION	CTriggerSequence::m_SaveData[] =
{
	DEFINE_ARRAY( CTriggerSequence, m_sequenceEntryName, FIELD_CHARACTER, CTriggerSequence::MAX_NAME_LENGTH + 1 ),
	DEFINE_FIELD( CTriggerSequence, m_sequenceStartTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerSequence, m_activator, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerSequence, m_useType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerSequence, m_value, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerSequence, m_isBusy, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerSequence, m_isDisabled, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CTriggerSequence, CBaseDelay );

LINK_ENTITY_TO_CLASS( trigger_sequence, CTriggerSequence );

CTriggerSequence::CTriggerSequence()
{
	StopSequence();

	//CZDS specific
	/*
	deathfadedelay = 2;
	fadespeed = 7;
	shotPlayerRecently = 0;
	shotPlayerEver = 0;
	shotConeTime = 0;
	leapConeFromFull = 0;
	m_classtype = 0;
	*/

	m_sequenceEntryName[ 0 ] = '\0';

	m_isDisabled = false;
	m_fastForward = false;
}

void CTriggerSequence::KeyValue( KeyValueData* pkvd )
{
	if( !stricmp( pkvd->szKeyName, "sequence_id" ) )
	{
		const auto length = strlen( pkvd->szValue );

		if( length > MAX_NAME_LENGTH )
		{
			ALERT( at_error, "Sequence entry label '%s' too long (was %d chars, max is %d)\n", pkvd->szValue, length, MAX_NAME_LENGTH );
		}

		strcpy( m_sequenceEntryName, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CTriggerSequence::Precache()
{
}

void CTriggerSequence::Spawn()
{
	m_sequenceEntry = g_engfuncs.pfnSequenceGet( nullptr, m_sequenceEntryName );
}

void CTriggerSequence::Restart()
{
	m_nextCommand = nullptr;
	m_sequenceEntry = g_engfuncs.pfnSequenceGet( nullptr, m_sequenceEntryName );
	m_fastForward = m_isBusy;
}

void CTriggerSequence::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if( !m_sequenceEntry )
	{
		StopSequence();

		UTIL_LogPrintf( "CTriggerSequence: unknown sequence \"%s\"; must be specified \n", m_sequenceEntryName );
		return;
	}

	if( m_isDisabled )
	{
		UTIL_LogPrintf( "CTriggerSequence: sequence \"%s\" is disabled, not firing.\n", m_sequenceEntryName );
		return;
	}

	if( pev->spawnflags & SF_SEQUENCE_FIRE_ONCE )
		m_isDisabled = true;

	StopSequence();

	if( m_sequenceEntry->firstCommand )
	{
		m_value = value;
		m_useType = useType;
		m_activator = pActivator;
		m_caller = pCaller;

		m_isBusy = true;

		GoToFirstCommand();

		Think();
	}
	else
	{
		UTIL_LogPrintf( "CTriggerSequence: sequence \"%s\" (in file \"%s\") was empty!\n", m_sequenceEntryName, m_sequenceEntry->fileName );
	}
}

void CTriggerSequence::Think()
{
	if( m_fastForward )
	{
		FastForward();

		m_fastForward = false;
	}

	auto timeNext = gpGlobals->time;

	while( m_nextCommand )
	{
		timeNext += m_nextCommand->delay;

		ExecuteSequenceCommand( m_nextCommand );

		if( gpGlobals->time < timeNext )
			break;
	}

	if( m_nextCommand && pev )
	{
		pev->nextthink = timeNext;
	}
	else
	{
		StopSequence();
	}
}

void CTriggerSequence::GoToFirstCommand()
{
	m_sequenceStartTime = gpGlobals->time;
	m_nextCommand = m_sequenceEntry->firstCommand;
}

void CTriggerSequence::AdvanceAndCheckForRepeat( int repeatCount )
{
	if( repeatCount )
	{
		GoToFirstCommand();
	}
	else
	{
		m_nextCommand = m_nextCommand->nextCommandLine;
	}
}

void CTriggerSequence::FastForward()
{
	m_nextCommand = m_sequenceEntry->firstCommand;

	for( auto timeNext = m_sequenceStartTime; ( timeNext + 0.0001 ) < gpGlobals->time && m_nextCommand; )
	{
		timeNext += m_nextCommand->delay;

		m_nextCommand = m_nextCommand->nextCommandLine;
	}
}

void CTriggerSequence::StopSequence()
{
	m_nextCommand = nullptr;
	m_activator = nullptr;
	m_caller = nullptr;
	m_sequenceStartTime = 0;
	m_useType = USE_OFF;
	m_value = 0;
	m_isBusy = false;

	//This method can be called before the entity has been initialized, so check this
	if( pev )
		pev->nextthink = 0;
}

void CTriggerSequence::ExecuteFireTargets( char* fireTargetNames )
{
	if( fireTargetNames )
		FireTargets( fireTargetNames, m_activator, this, m_useType, m_value );
}

void CTriggerSequence::ExecuteKillTargets( char* killTargetNames )
{
	if( killTargetNames )
	{
		for( CBaseEntity* pEntity = nullptr; ( pEntity = UTIL_FindEntityByTargetname( pEntity, killTargetNames ) ); )
		{
			pEntity->PreKillTarget();

			UTIL_Remove( pEntity );
		}
	}
}

void CTriggerSequence::ExecuteTextMessage( client_textmessage_t* message, int textChannel )
{
	if( message && message->pMessage )
	{
		hudtextparms_t hudTextParms;

		hudTextParms.holdTime = message->holdtime;
		hudTextParms.x = message->x;
		hudTextParms.y = message->y;
		hudTextParms.effect = message->effect;
		hudTextParms.r1 = message->r1;
		hudTextParms.g1 = message->g1;
		hudTextParms.b1 = message->b1;
		hudTextParms.a1 = message->a1;
		hudTextParms.r2 = message->r2;
		hudTextParms.g2 = message->g2;
		hudTextParms.b2 = message->b2;
		hudTextParms.a2 = message->a2;
		hudTextParms.fadeinTime = message->fadein;
		hudTextParms.fadeoutTime = message->fadeout;
		hudTextParms.fxTime = message->fxtime;
		hudTextParms.channel = textChannel;

		if( message->holdtime == -1 )
		{
			hudTextParms.holdTime = ( strlen( message->pMessage ) * 0.05 ) + 1.0;
		}

		UTIL_HudMessageAll( hudTextParms, message->pMessage );
	}
}

void CTriggerSequence::ExecuteSound( char* soundName )
{
	if( soundName )
		SENTENCEG_PlayRndSz( edict(), soundName, VOL_NORM, ATTN_NONE, 0, PITCH_NORM );
}

void CTriggerSequence::ExecuteSpeech( char* soundName, char* speakerName, char* listenerName, float duration )
{
	if( speakerName && soundName )
	{
		auto pTarget = UTIL_FindEntityByTargetname( nullptr, speakerName );

		if( pTarget )
		{
			auto pMonsterTarget = pTarget->MyMonsterPointer();

			if( pMonsterTarget && pev )
			{
				auto pListener = UTIL_FindEntityGeneric( listenerName, pev->origin, 4096 );

				pMonsterTarget->PlayScriptedSentence( soundName, duration, VOL_NORM, ATTN_NORM, true, pListener );
			}
		}
	}
}

void CTriggerSequence::ExecuteSoundOrSpeech( char* soundName, char* speakerName, char* listenerName, float duration )
{
	if( soundName )
	{
		if( speakerName && strcmp( "none", speakerName ) )
			CTriggerSequence::ExecuteSpeech( soundName, speakerName, listenerName, duration );
		else
			ExecuteSound( soundName );
	}
}

void CTriggerSequence::ExecuteSequenceCommand( sequenceCommandLine_s* command )
{
	if( command )
	{
		ExecuteFireTargets( command->fireTargetNames );

		ExecuteKillTargets( command->killTargetNames );
		ExecuteTextMessage( &command->clientMessage, command->textChannel );

		ExecuteSoundOrSpeech( command->soundFileName, command->speakerName, command->listenerName, command->delay );

		AdvanceAndCheckForRepeat( command->repeatCount );
	}
}

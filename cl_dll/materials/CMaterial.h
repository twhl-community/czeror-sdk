/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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

#pragma once

struct materials_t
{
	char m_chMaterialType;
	char m_szTextName[32];
	int m_iGibMultiplier;
	int m_iDebriSpriteNum;
	float m_flSize;
	float m_flScaleSpeed;
	float m_flBrightness;
	float m_flFadeSpeed;
	float m_flLife;
	float m_flGravity;
	float m_flAfterDampGravity;
	float m_flDampingVelocity;
	int m_iFrame;
	Vector m_vVelocity;
	float m_flVelMod;
	int m_iRendermode;
	Vector m_vColor;
	float m_flDampingTime;
	int m_iCollisionFlags;
	int m_iAfterDampFlags;
	int m_iSpawnDebris;
	int m_iSpawnSmoke;
	int m_iSpawnSparks;
	int m_iSpawnImpacts;
	Vector m_vSmokeColor;
	char m_szBulletImpactName[32];
	float m_flMinScale;
	float m_flMaxScale;
	int m_iDecalNum;
	int m_bRandomFlip;
	int m_iIndex;
	char m_szOriginalName[32];
};

class CMaterial
{
public:
	int m_iIndex;
	char m_szName[64];
	CMaterial* pNext;
	materials_t* pMaterial;
};

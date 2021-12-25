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

#include "CMaterial.h"

enum class MaterialIndex
{
	None = 0,
	Concrete = 1,
	Metal,
	Dirt,
	Tile,
	Slosh,
	Glass,
	Snow,
	Paper,
	Grass,
	Custom,
	Generic,
	Wood,
};

const char* MAT_FindMaterialFromTxt(char* szOriginalName);

MaterialIndex MAT_GetMaterialIndexByType(const char* pMaterialType);

int MAT_GetRenderMode(const char* token);

bool MAT_SetDefaults(MaterialIndex iMaterialType, materials_t* pMat);

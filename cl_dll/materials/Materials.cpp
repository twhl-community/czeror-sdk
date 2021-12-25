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

#include "hud.h"
#include "cl_util.h"
#include "pm_shared.h"
#include "CMaterial.h"
#include "Materials.h"
#include "pm_materials.h"

#include "filesystem_utils.h"

//TODO: add support for all material types.

void COM_FileBase(const char* in, char* out);

const char* MAT_FindMaterialFromTxt(char* szOriginalName)
{
	const auto type = PM_FindTextureType(szOriginalName);

	switch (type)
	{
	case CHAR_TEX_CONCRETE: return "concrete";
	case CHAR_TEX_DIRT: return "dirt";
	case CHAR_TEX_METAL: return "metal";
	case CHAR_TEX_SNOW: return "snow";
	case CHAR_TEX_SLOSH: return "slosh";
	case CHAR_TEX_TILE: return "tile";
	case CHAR_TEX_VENT: return "metal";
	case CHAR_TEX_WOOD: return "wood";
	case CHAR_TEX_PAPER: return "paper";
	case CHAR_TEX_GLASS: return "glass";

	default: return "generic";
	}
}

MaterialIndex MAT_GetMaterialIndexByType(const char* pMaterialType)
{
	if (0 == stricmp(pMaterialType, "concrete"))
	{
		return MaterialIndex::Concrete;
	}
	else if (0 == stricmp(pMaterialType, "metal"))
	{
		return MaterialIndex::Metal;
	}
	else if (0 == stricmp(pMaterialType, "dirt"))
	{
		return MaterialIndex::Dirt;
	}
	else if (0 == stricmp(pMaterialType, "tile"))
	{
		return MaterialIndex::Tile;
	}
	else if (0 == stricmp(pMaterialType, "slosh"))
	{
		return MaterialIndex::Slosh;
	}
	else if (0 == stricmp(pMaterialType, "glass"))
	{
		return MaterialIndex::Glass;
	}
	else if (0 == stricmp(pMaterialType, "snow"))
	{
		return MaterialIndex::Snow;
	}
	else if (0 == stricmp(pMaterialType, "paper"))
	{
		return MaterialIndex::Paper;
	}
	else if (0 == stricmp(pMaterialType, "grass"))
	{
		return MaterialIndex::Grass;
	}
	else if (0 == stricmp(pMaterialType, "custom"))
	{
		return MaterialIndex::Custom;
	}
	else if (0 == stricmp(pMaterialType, "generic"))
	{
		return MaterialIndex::Generic;
	}
	else if (0 == stricmp(pMaterialType, "wood"))
	{
		return MaterialIndex::Wood;
	}
	else
	{
		return MaterialIndex::Generic;
	}
}

int MAT_GetRenderMode(const char* token)
{
	if (0 == stricmp(token, "kRenderNormal"))
	{
		return kRenderNormal;
	}
	else if (0 == stricmp(token, "kRenderTransColor"))
	{
		return kRenderTransColor;
	}
	else if (0 == stricmp(token, "kRenderTransTexture"))
	{
		return kRenderTransTexture;
	}
	else if (0 == stricmp(token, "kRenderGlow"))
	{
		return kRenderGlow;
	}
	else if (0 == stricmp(token, "kRenderTransAlpha"))
	{
		return kRenderTransAlpha;
	}
	else if (0 == stricmp(token, "kRenderTransAdd"))
	{
		return kRenderTransAdd;
	}

	//Treat as mode index, falls back to kRenderNormal (0) if it couldn't be parsed.
	return atoi(token);
}

bool MAT_SetDefaults(MaterialIndex iMaterialType, materials_t* pMat)
{
	char szMaterial[64];

	switch (iMaterialType)
	{
	default:
		break;
	case MaterialIndex::Concrete:
		strcpy(szMaterial, "concrete");
		break;
	case MaterialIndex::Metal:
		strcpy(szMaterial, "metal");
		break;
	case MaterialIndex::Dirt:
		strcpy(szMaterial, "dirt");
		break;
	case MaterialIndex::Tile:
		strcpy(szMaterial, "tile");
		break;
	case MaterialIndex::Slosh:
		strcpy(szMaterial, "slosh");
		break;
	case MaterialIndex::Glass:
		strcpy(szMaterial, "glass");
		break;
	case MaterialIndex::Snow:
		strcpy(szMaterial, "snow");
		break;
	case MaterialIndex::Paper:
		strcpy(szMaterial, "paper");
		break;
	case MaterialIndex::Grass:
		strcpy(szMaterial, "grass");
		break;
	case MaterialIndex::Custom:
		strcpy(szMaterial, "custom");
		break;
	case MaterialIndex::Generic:
		strcpy(szMaterial, "generic");
		break;
	case MaterialIndex::Wood:
		strcpy(szMaterial, "wood");
		break;
	}

	char DefaultMaterialFolder[128];
	snprintf(DefaultMaterialFolder, sizeof(DefaultMaterialFolder), "materials/defaults/%s.mat", szMaterial);

	auto data = FileSystem_LoadFileIntoBuffer(DefaultMaterialFolder);

	if (data.empty())
	{
		return false;
	}

	char* nextData = reinterpret_cast<char*>(data.data());

	char token[1024];

	while (true)
	{
		nextData = gEngfuncs.COM_ParseFile(nextData, token);

		if (nullptr == nextData)
		{
			break;
		}

		if (0 == stricmp(token, "debris"))
		{
			nextData = gEngfuncs.COM_ParseFile(nextData, token);

			if (0 != stricmp(token, "{"))
			{
				gEngfuncs.Con_Printf("Error parsing material file %s. ( expected { )\n", szMaterial);
				return false;
			}

			nextData = gEngfuncs.COM_ParseFile(nextData, token);

			while (0 != stricmp(token, "}"))
			{
				if (0 == stricmp(token, "size"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flSize = atof(token);
				}
				else if (0 == stricmp(token, "spritenum"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iDebriSpriteNum = atoi(token);
				}
				else if (0 == stricmp(token, "scalespeed"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flScaleSpeed = atof(token);
				}
				else if (0 == stricmp(token, "brightness"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flBrightness = atof(token);
				}
				else if (0 == stricmp(token, "fadespeed"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flFadeSpeed = atof(token);
				}
				else if (0 == stricmp(token, "life"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flLife = atof(token);
				}
				else if (0 == stricmp(token, "original_gravity"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flGravity = atof(token);
				}
				else if (0 == stricmp(token, "afterdamp_gravity"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flAfterDampGravity = atof(token);
				}
				else if (0 == stricmp(token, "damp_vel_mod"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flDampingVelocity = atof(token);
				}
				else if (0 == stricmp(token, "sprite"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);

					char szSprite[64]{};

					COM_FileBase(token, szSprite);

					const int lastCharacterIndex = strlen(szSprite) - 1;

					const char lastCharacter = szSprite[lastCharacterIndex] - '0';

					if (lastCharacter >= 0 && lastCharacter <= 9)
					{
						szSprite[lastCharacterIndex] = '\0';
					}

					snprintf(pMat->m_szTextName, sizeof(pMat->m_szTextName), "sprites/%s.spr", szSprite);
				}
				else if (0 == stricmp(token, "rendermode"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iRendermode = MAT_GetRenderMode(token);
				}
				else if (0 == stricmp(token, "flags"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iCollisionFlags = atoi(token);
				}
				else if (0 == stricmp(token, "afterdamp_flags"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iAfterDampFlags = atoi(token);
				}
				else if (0 == stricmp(token, "velmod"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flVelMod = atof(token);
				}
				else if (0 == stricmp(token, "color"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vColor.x = atof(token);

					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vColor.y = atof(token);

					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vColor.z = atof(token);
				}
				else if (0 == stricmp(token, "damping_time"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flDampingTime = atof(token);
				}
				else if (0 == stricmp(token, "allow"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iSpawnDebris = atoi(token);
				}
				else
				{
					gEngfuncs.Con_Printf("Error parsing material file %s. ( %s unknown )\n", szMaterial, token);
					return false;
				}

				nextData = gEngfuncs.COM_ParseFile(nextData, token);
			}
		}
		else if (0 == stricmp(token, "smoke"))
		{
			nextData = gEngfuncs.COM_ParseFile(nextData, token);

			if (0 != stricmp(token, "{"))
			{
				gEngfuncs.Con_DPrintf("Error parsing material file %s. ( expected { )\n", szMaterial);
				return false;
			}

			while (true)
			{
				nextData = gEngfuncs.COM_ParseFile(nextData, token);

				if (0 == stricmp(token, "}"))
				{
					break;
				}

				if (0 == stricmp(token, "allow"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iSpawnSmoke = atoi(token);
				}
				else if (0 == stricmp(token, "color"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vSmokeColor.x = atof(token);

					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vSmokeColor.y = atof(token);

					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_vSmokeColor.z = atof(token);
				}
				else
				{
					gEngfuncs.Con_DPrintf("Error parsing material file %s. ( %s unknown )\n", szMaterial, token);
					return false;
				}
			}
		}
		else if (0 == stricmp(token, "sparks"))
		{
			nextData = gEngfuncs.COM_ParseFile(nextData, token);

			if (0 != stricmp(token, "{"))
			{
				gEngfuncs.Con_DPrintf("Error parsing material file %s. ( expected { )\n", szMaterial);
				return false;
			}

			while (true)
			{
				nextData = gEngfuncs.COM_ParseFile(nextData, token);

				if (0 == stricmp(token, "}"))
				{
					break;
				}

				if (0 == stricmp(token, "allow"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iSpawnSparks = atoi(token);
				}
				else
				{
					gEngfuncs.Con_DPrintf("Error parsing material file %s. ( %s unknown )\n", szMaterial, token);
					return false;
				}
			}
		}
		else if (0 == stricmp(token, "impacts"))
		{
			nextData = gEngfuncs.COM_ParseFile(nextData, token);

			if (0 != stricmp(token, "{"))
			{
				gEngfuncs.Con_DPrintf("Error parsing material file %s. ( expected { )\n", szMaterial);
				return false;
			}

			while (true)
			{
				nextData = gEngfuncs.COM_ParseFile(nextData, token);

				if (0 == stricmp(token, "}"))
				{
					break;
				}

				if (0 == stricmp(token, "allow"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iSpawnImpacts = atoi(token);
				}
				else if (0 == stricmp(token, "decalname"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					strcpy(pMat->m_szBulletImpactName, token);
				}
				else if (0 == stricmp(token, "minscale"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flMinScale = atof(token);
				}
				else if (0 == stricmp(token, "maxscale"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_flMaxScale = atof(token);
				}
				else if (0 == stricmp(token, "decalnum"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_iDecalNum = atoi(token);
				}
				else if (0 == stricmp(token, "randomflip"))
				{
					nextData = gEngfuncs.COM_ParseFile(nextData, token);
					pMat->m_bRandomFlip = atoi(token);
				}
				else
				{
					gEngfuncs.Con_DPrintf("Error parsing material file %s. ( %s unknown )\n", szMaterial, token);
					return false;
				}
			}
		}
	}

	switch (iMaterialType)
	{
	default:
		pMat->m_chMaterialType = PM_FindTextureType(pMat->m_szOriginalName);
		break;
	case MaterialIndex::Concrete:
		pMat->m_chMaterialType = CHAR_TEX_CONCRETE;
		break;
	case MaterialIndex::Metal:
		pMat->m_chMaterialType = CHAR_TEX_METAL;
		break;
	case MaterialIndex::Dirt:
		pMat->m_chMaterialType = CHAR_TEX_DIRT;
		break;
	case MaterialIndex::Tile:
		pMat->m_chMaterialType = CHAR_TEX_TILE;
		break;
	case MaterialIndex::Slosh:
		pMat->m_chMaterialType = CHAR_TEX_SLOSH;
		break;
	case MaterialIndex::Glass:
		pMat->m_chMaterialType = CHAR_TEX_GLASS;
		break;
	case MaterialIndex::Snow:
		pMat->m_chMaterialType = CHAR_TEX_SNOW;
		break;
	case MaterialIndex::Paper:
		pMat->m_chMaterialType = CHAR_TEX_PAPER;
		break;
	case MaterialIndex::Grass:
		pMat->m_chMaterialType = CHAR_TEX_GRASS;
		break;
	case MaterialIndex::Custom:
		pMat->m_chMaterialType = CHAR_TEX_CUSTOM;
		break;
	case MaterialIndex::Wood:
		pMat->m_chMaterialType = CHAR_TEX_WOOD;
		break;
	}

	return true;
}

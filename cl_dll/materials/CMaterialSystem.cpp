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

#include <memory>
#include <string>

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "CMaterial.h"
#include "CMaterialSystem.h"
#include "Materials.h"

#include "filesystem_utils.h"

//Use the tools bspfile definitions. They still use vec3_t, so define it for them.
#define vec3_t Vector

#include "../../utils/common/bspfile.h"

void COM_FileBase(const char* in, char* out);

CMaterialSystem g_pMaterialSystem;

static int g_iDesiredMaterials = 0;
static char g_szDesiredMaterialNames[CTEXTURESMAX][64];

CMaterialSystem::~CMaterialSystem()
{
	ClearMaterials();
}

bool CMaterialSystem::AddMaterial(const char* szTexName)
{
	if (m_iElements < MAX_MATERIALS && '\0' != *szTexName)
	{
		auto material = new CMaterial();

		material->pNext = nullptr;
		material->pMaterial = nullptr;

		strncpy(material->m_szName, szTexName, sizeof(material->m_szName) - 1);
		material->m_szName[sizeof(material->m_szName) - 1] = '\0';

		material->m_iIndex = ++m_iElements;

		auto link = pFirst ? &pCurrent->pNext : &pFirst;

		*link = material;

		pCurrent = material;

		return true;
	}

	return false;
}

void CMaterialSystem::ClearMaterials()
{
	pCurrent = nullptr;

	for (CMaterial *material = pFirst, *next = nullptr; material; material = next)
	{
		next = pFirst->pNext;

		delete pFirst->pMaterial;
		delete pFirst;
		--m_iElements;
	}

	pFirst = nullptr;
}

bool CMaterialSystem::CheckMaterial(const char* szName)
{
	for (auto material = pFirst; material; material = material->pNext)
	{
		if (0 == stricmp(material->m_szName, szName))
		{
			return true;
		}
	}

	return false;
}

materials_t* CMaterialSystem::GetMaterialByIndex(int iIndex)
{
	for (auto material = pFirst; material; material = material->pNext)
	{
		if (material->m_iIndex == iIndex)
		{
			return material->pMaterial;
		}
	}

	return nullptr;
}

int CMaterialSystem::GetMaterialIndex(const char* szName) const
{
	for (auto material = pFirst; material; material = material->pNext)
	{
		if (0 == stricmp(material->m_szName, szName))
		{
			return material->m_iIndex;
		}
	}

	return -1;
}

void CMaterialSystem::FixTextureName(const char* pTextureIn, char* pTextOut) const
{
	if (*pTextureIn == '+' || *pTextureIn == '-')
	{
		pTextureIn += 2;
	}

	if (*pTextureIn == '!' || *pTextureIn == '{' || *pTextureIn == '~' || *pTextureIn == ' ')
	{
		strcpy(pTextOut, pTextureIn + 1);
	}
	else
	{
		strcpy(pTextOut, pTextureIn);
	}
}

int CMaterialSystem::GetMaterialIndexAtPos(pmtrace_t* ptr, float* vecSrc, float* vecEnd)
{
	auto texture = gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

	if (!texture)
	{
		return -1;
	}

	char pTextName[64];
	FixTextureName(texture, pTextName);

	return GetMaterialIndex(pTextName);
}

bool CMaterialSystem::IsValidMaterialName(const char* pTextureName) const
{
	if (0 != stricmp(pTextureName, "aaatrigger") && 0 != stricmp(pTextureName, "invisible") && 0 != stricmp(pTextureName, "sky") && 0 != stricmp(pTextureName, "hint") && 0 != stricmp(pTextureName, "skip") && 0 != stricmp(pTextureName, "black") && 0 != stricmp(pTextureName, "white") && 0 != stricmp(pTextureName, "yellow") && 0 != stricmp(pTextureName, "red") && 0 != stricmp(pTextureName, "blue") && 0 != stricmp(pTextureName, "translucent") && 0 != stricmp(pTextureName, "fade") && 0 != stricmp(pTextureName, "fade2") && 0 != stricmp(pTextureName, "clip") && 0 != stricmp(pTextureName, "origin"))
	{
		return true;
	}

	return false;
}

bool CMaterialSystem::ShouldLoad(const char* pTextureName)
{
	if (!IsValidMaterialName(pTextureName))
	{
		return false;
	}

	return GetMaterialIndex(pTextureName) == -1;
}

void CMaterialSystem::CopyMaterials(CMaterialSystem& materialList)
{
	for (auto source = materialList.pFirst; source; source = source->pNext)
	{
		if (GetMaterialIndex(source->m_szName) != -1)
		{
			continue;
		}

		if (!AddMaterial(source->m_szName))
		{
			continue;
		}

		auto dest = pCurrent;

		dest->pMaterial = new materials_t();

		std::memcpy(dest->pMaterial, source->pMaterial, sizeof(*dest->pMaterial));
	}
}

bool CMaterialSystem::LoadMaterial(const char* pTextureName, char*& data)
{
	if ('\0' == *pTextureName)
	{
		m_MatFIO.WriteToErrorFile(pTextureName, "Trying to load material with no name!");
		return false;
	}

	if (nullptr == data)
	{
		m_MatFIO.WriteToErrorFile(pTextureName, "Couldn't find material (enable mat_autocreate to create it automatically)");
		return false;
	}

	char token[1024];

	data = gEngfuncs.COM_ParseFile(data, token);

	if (!AddMaterial(pTextureName))
	{
		m_MatFIO.WriteToErrorFile(pTextureName, "Couldn't create material (God knows why)");
		return false;
	}

	pCurrent->pMaterial = new materials_t();

	std::memset(pCurrent->pMaterial, 0, sizeof(*pCurrent->pMaterial));

	auto pNewMaterial = pCurrent->pMaterial;

	pNewMaterial->m_iIndex = m_iElements;
	strcpy(pNewMaterial->m_szOriginalName, pTextureName);

	MaterialIndex iMaterialType = MaterialIndex::None;

	if (nullptr != data)
	{
		while (true)
		{
			//TODO: this parsing code has no failure condition for unexpected EOF and will loop forever

			data = gEngfuncs.COM_ParseFile(data, token);

			if (0 == stricmp(token, "material"))
			{
				data = gEngfuncs.COM_ParseFile(data, token);

				if (0 != stricmp(token, "{"))
				{
					m_MatFIO.WriteToErrorFile(pTextureName, "expected { found %s", token);
					return false;
				}

				data = gEngfuncs.COM_ParseFile(data, token);

				while (true)
				{
					if (0 == stricmp(token, "}"))
					{
						break;
					}

					if (0 == stricmp(token, "type"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);

						char szMaterialType[64];
						strcpy(szMaterialType, token);

						iMaterialType = MAT_GetMaterialIndexByType(szMaterialType);
						MAT_SetDefaults(iMaterialType, pNewMaterial);

						data = gEngfuncs.COM_ParseFile(data, token);
					}
					else
					{
						m_MatFIO.WriteToErrorFile(pTextureName, "%s unknown", token);
						return false;
					}
				}
			}
			else if (0 == stricmp(token, "debris"))
			{
				data = gEngfuncs.COM_ParseFile(data, token);

				if (0 != stricmp(token, "{"))
				{
					m_MatFIO.WriteToErrorFile(pTextureName, "expected { found %s", token);
					return false;
				}

				data = gEngfuncs.COM_ParseFile(data, token);

				while (0 != stricmp(token, "}"))
				{
					if (0 == stricmp(token, "size"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flSize = atof(token);
					}
					else if (0 == stricmp(token, "spritenum"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iDebriSpriteNum = atoi(token);
					}
					else if (0 == stricmp(token, "scalespeed"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flScaleSpeed = atof(token);
					}
					else if (0 == stricmp(token, "brightness"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flBrightness = atof(token);
					}
					else if (0 == stricmp(token, "fadespeed"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flFadeSpeed = atof(token);
					}
					else if (0 == stricmp(token, "life"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flLife = atof(token);
					}
					else if (0 == stricmp(token, "original_gravity"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flGravity = atof(token);
					}
					else if (0 == stricmp(token, "afterdamp_gravity"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flAfterDampGravity = atof(token);
					}
					else if (0 == stricmp(token, "damp_vel_mod"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flDampingVelocity = atof(token);
					}
					else if (0 == stricmp(token, "sprite"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);

						char szSprite[64]{};

						COM_FileBase(token, szSprite);

						//TODO: what if it's an empty string?
						const int lastCharacterIndex = strlen(szSprite) - 1;

						const char lastCharacter = szSprite[lastCharacterIndex] - '0';

						if (lastCharacter >= 0 && lastCharacter <= 9)
						{
							szSprite[lastCharacterIndex] = '\0';
						}

						snprintf(pNewMaterial->m_szTextName, sizeof(pNewMaterial->m_szTextName), "sprites/%s", szSprite);
					}
					else if (0 == stricmp(token, "rendermode"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iRendermode = MAT_GetRenderMode(token);
					}
					else if (0 == stricmp(token, "flags"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iCollisionFlags = atoi(token);
					}
					else if (0 == stricmp(token, "afterdamp_flags"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iAfterDampFlags = atoi(token);
					}
					else if (0 == stricmp(token, "velmod"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flVelMod = atof(token);
					}
					else if (0 == stricmp(token, "color"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_vColor.x = atof(token);

						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_vColor.y = atof(token);

						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_vColor.z = atof(token);
					}
					else if (0 == stricmp(token, "damping_time"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flDampingTime = atof(token);
					}
					else
					{
						if (0 == stricmp(token, "allow"))
						{
							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_iSpawnDebris = atoi(token);
						}
						else
						{
							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_iSpawnDebris = atoi(token);
						}
					}

					data = gEngfuncs.COM_ParseFile(data, token);
				}
			}
			else if (0 == stricmp(token, "smoke"))
			{
				data = gEngfuncs.COM_ParseFile(data, token);

				if (0 != stricmp(token, "{"))
				{
					m_MatFIO.WriteToErrorFile(pTextureName, "expected { found %s", token);
					return false;
				}

				data = gEngfuncs.COM_ParseFile(data, token);

				while (0 != stricmp(token, "}"))
				{
					if (0 == stricmp(token, "allow"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iSpawnSmoke = atoi(token);
					}
					else
					{
						if (0 == stricmp(token, "color"))
						{
							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_vSmokeColor.x = atof(token);

							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_vSmokeColor.y = atof(token);

							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_vSmokeColor.z = atof(token);
						}
						else
						{
							m_MatFIO.WriteToErrorFile(pTextureName, pTextureName, "( %s unknown )", token);
							return false;
						}
					}

					data = gEngfuncs.COM_ParseFile(data, token);
				}
			}
			else if (0 == stricmp(token, "sparks"))
			{
				data = gEngfuncs.COM_ParseFile(data, token);

				if (0 != stricmp(token, "{"))
				{
					m_MatFIO.WriteToErrorFile(pTextureName, "expected { found %s", token);
					return false;
				}

				data = gEngfuncs.COM_ParseFile(data, token);

				while (0 != stricmp(token, "}"))
				{
					if (0 == stricmp(token, "allow"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iSpawnSparks = atoi(token);
						data = gEngfuncs.COM_ParseFile(data, token);
					}
					else
					{
						m_MatFIO.WriteToErrorFile(pTextureName, pTextureName, "( %s unknown )", token);
						return false;
					}
				}
			}
			else if (0 == stricmp(token, "impacts"))
			{
				data = gEngfuncs.COM_ParseFile(data, token);

				if (0 != stricmp(token, "{"))
				{
					m_MatFIO.WriteToErrorFile(pTextureName, "expected { found %s", token);
					return false;
				}

				data = gEngfuncs.COM_ParseFile(data, token);

				while (0 != stricmp(token, "}"))
				{
					if (0 == stricmp(token, "allow"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iSpawnImpacts = atoi(token);
					}
					else if (0 == stricmp(token, "decalname"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						strcpy(pNewMaterial->m_szBulletImpactName, token);
					}
					else if (0 == stricmp(token, "minscale"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flMinScale = atof(token);
					}
					else if (0 == stricmp(token, "maxscale"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_flMaxScale = atof(token);
					}
					else if (0 == stricmp(token, "decalnum"))
					{
						data = gEngfuncs.COM_ParseFile(data, token);
						pNewMaterial->m_iDecalNum = atoi(token);
					}
					else
					{
						if (0 == stricmp(token, "randomflip"))
						{
							data = gEngfuncs.COM_ParseFile(data, token);
							pNewMaterial->m_bRandomFlip = atoi(token);
						}
						else
						{
							m_MatFIO.WriteToErrorFile(pTextureName, pTextureName, "( %s unknown )", token);
							return false;
						}
					}

					data = gEngfuncs.COM_ParseFile(data, token);
				}
			}

			if (0 == stricmp(token, "]"))
			{
				break;
			}
		}
	}

	switch (iMaterialType)
	{
	default:
		pNewMaterial->m_chMaterialType = PM_FindTextureType(pTextureName);
		break;
	case MaterialIndex::Wood:
		pNewMaterial->m_chMaterialType = 'W';
		break;
	case MaterialIndex::Custom:
		pNewMaterial->m_chMaterialType = 'K';
		break;
	case MaterialIndex::Grass:
		pNewMaterial->m_chMaterialType = 'A';
		break;
	case MaterialIndex::Paper:
		pNewMaterial->m_chMaterialType = 'X';
		break;
	case MaterialIndex::Snow:
		pNewMaterial->m_chMaterialType = 'N';
		break;
	case MaterialIndex::Glass:
		pNewMaterial->m_chMaterialType = 'Y';
		break;
	case MaterialIndex::Slosh:
		pNewMaterial->m_chMaterialType = 'S';
		break;
	case MaterialIndex::Tile:
		pNewMaterial->m_chMaterialType = 'T';
		break;
	case MaterialIndex::Dirt:
		pNewMaterial->m_chMaterialType = 'D';
		break;
	case MaterialIndex::Metal:
		pNewMaterial->m_chMaterialType = 'M';
		break;
	case MaterialIndex::Concrete:
		pNewMaterial->m_chMaterialType = 'C';
		break;
	}

	return true;
}

void LoadTextures(const CMaterialSystem& materialSystem, const std::byte* modBase, const lump_t& l)
{
	g_iDesiredMaterials = 0;

	auto lump = reinterpret_cast<const dmiptexlump_t*>(modBase + l.fileofs);

	std::string name;

	for (int i = 0; i < lump->nummiptex; ++i)
	{
		const auto miptex = reinterpret_cast<const miptex_t*>(reinterpret_cast<const std::byte*>(lump) + lump->dataofs[i]);

		if (!miptex)
		{
			continue;
		}

		name.resize(strlen(miptex->name));

		materialSystem.FixTextureName(miptex->name, name.data());

		int j;

		for (j = 0; j < g_iDesiredMaterials; ++j)
		{
			if (0 == strcmp(name.c_str(), g_szDesiredMaterialNames[j]))
			{
				break;
			}
		}

		if (j == g_iDesiredMaterials)
		{
			if (materialSystem.IsValidMaterialName(name.c_str()))
			{
				strcpy(g_szDesiredMaterialNames[g_iDesiredMaterials++], name.c_str());
			}
		}
	}
}

void LoadTextureLump(const CMaterialSystem& materialSystem, const char* map)
{
	const auto data = FileSystem_LoadFileIntoBuffer(map);

	if (data.empty())
	{
		return;
	}

	const auto header = reinterpret_cast<const dheader_t*>(data.data());

	LoadTextures(materialSystem, data.data(), header->lumps[LUMP_TEXTURES]);
}

bool TextureNameInList(const char* pTextureName)
{
	for (int i = 0; i < g_iDesiredMaterials; ++i)
	{
		if (0 == strcmp(pTextureName, g_szDesiredMaterialNames[i]))
		{
			return true;
		}
	}

	return false;
}

void LoadMaterialFromNameList(CMaterialSystem& materialSystem, char* data)
{
	char token[1024];

	while (nullptr != data)
	{
		data = gEngfuncs.COM_ParseFile(data, token);

		if (!stricmp(token, "texture"))
		{
			data = gEngfuncs.COM_ParseFile(data, token);

			auto name = token;

			//Strip animated or random texture prefix.
			if (name[0] == '+' || (name[0] == '-'))
			{
				name = &token[2];
			}

			//Strip modifiers.
			if (name[0] == '!' || name[0] == '{' || name[0] == '~' || name[0] == ' ')
			{
				++name;
			}

			//TODO: is the stripped name supposed to be passed to LoadMaterial here?
			if (TextureNameInList(name) && !materialSystem.LoadMaterial(token, data))
			{
				materialSystem.GetIO()->WriteToErrorFile(token, "Couldn't Load Material\n");
			}
		}
	}
}

void CreateMaterialFile(CMaterialSystem& materialSystem, char* levelname)
{
	//TODO: replace with std::string
	char filename[256];
	snprintf(filename, sizeof(filename), "materials/%s.mat", levelname);

	if (FSFile file{filename, "w+"}; file)
	{
		for (int i = 0; i < g_iDesiredMaterials; ++i)
		{
			auto name = g_szDesiredMaterialNames[i];

			if (materialSystem.IsValidMaterialName(name) && materialSystem.GetMaterialIndex(name) == -1)
			{
				auto type = MAT_FindMaterialFromTxt(name);
				file.Printf("texture \"%s\"\n[\n\tmaterial\n\t{\n\t\ttype \"%s\"\n\t}\n]\n", name, type);
				materialSystem.IncrementNewMaterialsCount();
			}
		}
	}
}

void LoadLevelMaterials()
{
	auto mapName = gEngfuncs.pfnGetLevelName();

	if ('\0' == *mapName)
	{
		return;
	}

	gEngfuncs.pfnAddCommand("mat_reload", MAT_FlushAndReload);

	const auto tempMaterialSystem = std::make_unique<CMaterialSystem>();

	auto materialsData = FileSystem_LoadFileIntoBuffer("materials/czero.mat");

	LoadTextureLump(*tempMaterialSystem, mapName);
	LoadMaterialFromNameList(*tempMaterialSystem, reinterpret_cast<char*>(materialsData.data()));

	//TODO: replace these buffers with std::string
	char current_level[32];
	COM_FileBase(mapName, current_level);

	char szFileName[128];
	sprintf(szFileName, "materials/%s.mat", current_level);

	materialsData = FileSystem_LoadFileIntoBuffer(szFileName);

	if (materialsData.empty() && mat_autocreate->value == 1)
	{
		CreateMaterialFile(*tempMaterialSystem, current_level);

		//Now open the newly generated file.
		//TODO: use std::string
		char filename[256];
		snprintf(filename, sizeof(filename), "materials/%s.mat", current_level);
		materialsData = FileSystem_LoadFileIntoBuffer(filename);
	}

	g_pMaterialSystem.ClearMaterials();

	LoadMaterialFromNameList(g_pMaterialSystem, reinterpret_cast<char*>(materialsData.data()));

	g_pMaterialSystem.CopyMaterials(*tempMaterialSystem);

	if (mat_autocreate->value == 1)
	{
		gEngfuncs.Con_DPrintf(
			"--------------------------------------------\n%d Materials Loaded Successfully.\n%d New Materials Created.\n%d Materials Failed to Load.\n--------------------------------------------\n",
			g_pMaterialSystem.GetElementCount(),
			g_pMaterialSystem.GetNewMaterialsCount(),
			g_iDesiredMaterials - g_pMaterialSystem.GetElementCount());

		gEngfuncs.Cvar_SetValue("mat_autocreate", 0);
	}
	else
	{
		gEngfuncs.Con_DPrintf(
			"--------------------------------------------\n%d Materials Loaded Successfully.\n%d Materials Failed to Load.\n--------------------------------------------\n",
			g_pMaterialSystem.GetElementCount(),
			g_iDesiredMaterials - g_pMaterialSystem.GetElementCount());
	}
}

void MAT_FlushAndReload()
{
	gEngfuncs.Con_DPrintf("\nFlushing All Materials...\n");

	LoadLevelMaterials();

	if (mat_autocreate->value == 1)
	{
		gEngfuncs.Cvar_SetValue("mat_autocreate", 0);
	}
}

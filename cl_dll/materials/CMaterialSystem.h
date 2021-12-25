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

#include "pmtrace.h"
#include "CMaterialFIO.h"

class CMaterial;
struct materials_t;

inline cvar_t* mat_autocreate = nullptr;

class CMaterialSystem
{
public:
	static const int MAX_MATERIALS = 512;

	CMaterialSystem() = default;
	~CMaterialSystem();

	int GetElementCount() const { return m_iElements; }

	int GetNewMaterialsCount() const { return m_iNewMaterials; }

	void IncrementNewMaterialsCount() { ++m_iNewMaterials; }

	const CMaterialFIO* GetIO() const { return &m_MatFIO; }

	bool AddMaterial(const char* szTexName);
	void ClearMaterials();
	bool CheckMaterial(const char* szName);
	materials_t* GetMaterialByIndex(int iIndex);
	int GetMaterialIndex(const char* szName) const;
	void FixTextureName(const char* pTextureIn, char* pTextOut) const;
	int GetMaterialIndexAtPos(pmtrace_t* ptr, float* vecSrc, float* vecEnd);
	bool IsValidMaterialName(const char* pTextureName) const;
	bool ShouldLoad(const char* pTextureName);

	/**
	*	@brief Copies all materials in @p materialList that are not yet in this list to this list
	*/
	void CopyMaterials(CMaterialSystem& materialList);

	bool LoadMaterial(const char* pTextureName, char*& data);

private:
	int m_iElements = 0;
	int m_iNewMaterials = 0;
	CMaterial* pFirst = nullptr;
	CMaterial* pCurrent = nullptr;
	CMaterialFIO m_MatFIO;
};

extern CMaterialSystem g_pMaterialSystem;

void LoadLevelMaterials();

void MAT_FlushAndReload();

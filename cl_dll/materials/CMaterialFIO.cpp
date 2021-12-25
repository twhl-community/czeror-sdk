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
#include "CMaterialFIO.h"
#include "filesystem_utils.h"

void COM_FileBase(const char* in, char* out);

void CMaterialFIO::WriteToErrorFile(const char* pTextureName, const char* szError, ...) const
{
    va_list va;

    va_start(va, szError);

    if (0 != mat_writerrorfile->value)
    {
        //TODO: precalculate the log filename at the start of processing so we can store it in a std::string without incurring overhead every time this function is called
        static char string[1024];

        char current_level[32];
        COM_FileBase(gEngfuncs.pfnGetLevelName(), current_level);

        char filename[256];
        snprintf(filename, sizeof(filename), "materials/%s.error", current_level);
        vsnprintf(string, sizeof(string), szError, va);

        if (FSFile file{filename, "a+", "GAMECONFIG"}; file)
        {
            file.Printf("%s: %s\n", pTextureName, string);
        }
    }

    va_end(va);
}

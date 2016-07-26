/*
 * Process Hacker Extended Tools -
 *   notification icon extensions
 *
 * Copyright (C) 2011 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "exttools.h"
//#include <notifico.h>

#define GPU_ICON_ID 1
#define DISK_ICON_ID 2
#define NETWORK_ICON_ID 3

VOID EtpGpuIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtpDiskIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtpNetworkIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtRegisterNotifyIcons(
    VOID
    )
{
    PH_NF_ICON_REGISTRATION_DATA data;

    data.MessageCallback = NULL;



}

VOID EtpGpuIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{

}

VOID EtpDiskIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{

}

VOID EtpNetworkIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
   
}

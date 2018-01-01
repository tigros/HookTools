/*
 * Process Hacker Extended Tools -
 *   main program
 *
 * Copyright (C) 2017 tigros
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

#include "gethooks.h"
#include "hookstabp.h"
#include "phappresource.h"
#include <windowsx.h>

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

static HANDLE ModuleProcessId;

LRESULT CALLBACK MainWndPluginSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
)
{
    switch (uMsg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, MainWndPluginSubclassProc, uIdSubclass);
        break;
    case WM_COMMAND:
    {
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case PHAPP_ID_VIEW_REFRESH:
            if (HookTreeNewCreated && WinVerOK)
            {
                refill();
            }
            break;
        }
    }
    break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME2, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Hook Tools";
            info->Author = L"tigros";
            info->Description = L"Hook Tools";
            info->Url = L"https://github.com/tigros/HookTools";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { StringSettingType, SETTING_NAME_HOOK_TREE_LIST_COLUMNS, L"" },
                    { IntegerPairSettingType, SETTING_NAME_HOOK_TREE_LIST_SORT, L"2,1" } // 4, DescendingSortOrder
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE)); 
            }
        }
        break;
    }

    return TRUE;
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	EtSaveSettingsHookTreeList();
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    BOOL res = SetWindowSubclass(PhMainWndHandle, MainWndPluginSubclassProc, 0, 0);
	EtInitializeHooksTab();
}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	if (HookTreeNewCreated && IsWindowVisible(HookTreeNewHandle) && WinVerOK && Isdiffhooks())
	{
        refill();
	}
}


/*
 * Process Hacker Extended Tools -
 *   main program
 *
 * Copyright (C) 2010-2015 wj32
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

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI HandlePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI NetworkItemsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID NTAPI ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID NTAPI NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID NTAPI NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

PPH_PLUGIN PluginInstance;
LIST_ENTRY EtProcessBlockListHead;
LIST_ENTRY EtNetworkBlockListHead;
HWND ProcessTreeNewHandle;
HWND NetworkTreeNewHandle;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION HandlePropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkItemsUpdatedCallbackRegistration;

static HANDLE ModuleProcessId;

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
            info->Author = L"colin";
            info->Description = L"Hook Tools";
            info->Url = L"http://processhacker.sf.net";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &PluginTreeNewMessageCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
                ProcessPropertiesInitializingCallback,
                NULL,
                &ProcessPropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackHandlePropertiesInitializing),
                HandlePropertiesInitializingCallback,
                NULL,
                &HandlePropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing),
                ThreadMenuInitializingCallback,
                NULL,
                &ThreadMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing),
                ModuleMenuInitializingCallback,
                NULL,
                &ModuleMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
                ProcessTreeNewInitializingCallback,
                NULL,
                &ProcessTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing),
                NetworkTreeNewInitializingCallback,
                NULL,
                &NetworkTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSystemInformationInitializing),
                SystemInformationInitializingCallback,
                NULL,
                &SystemInformationInitializingCallbackRegistration
                );

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                &PhNetworkItemsUpdatedEvent,
                NetworkItemsUpdatedCallback,
                NULL,
                &NetworkItemsUpdatedCallbackRegistration
                );

            InitializeListHead(&EtProcessBlockListHead);
            InitializeListHead(&EtNetworkBlockListHead);

            PhPluginSetObjectExtension(
                PluginInstance,
                EmProcessItemType,
                sizeof(ET_PROCESS_BLOCK),
                ProcessItemCreateCallback,
                ProcessItemDeleteCallback
                );
            PhPluginSetObjectExtension(
                PluginInstance,
                EmNetworkItemType,
                sizeof(ET_NETWORK_BLOCK),
                NetworkItemCreateCallback,
                NetworkItemDeleteCallback
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

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	EtSaveSettingsHookTreeList();
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtShowOptionsDialog((HWND)Parameter);
}

void SetStdOutToNewConsole()
{
	// allocate a console for this app
	AllocConsole();

	// redirect unbuffered STDOUT to the console
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	int fileDescriptor = _open_osfhandle((intptr_t)consoleHandle, _O_TEXT);
	FILE *fp = _fdopen(fileDescriptor, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// give the console window a nicer title
	SetConsoleTitle(L"Debug Output");

	// give the console window a bigger buffer size
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(consoleHandle, &csbi))
	{
		COORD bufferSize;
		bufferSize.X = csbi.dwSize.X;
		bufferSize.Y = 9999;
		SetConsoleScreenBufferSize(consoleHandle, bufferSize);
	}
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_PROCESS_HOOKS:
        {
		//	gethooks();
        }
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{

}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	EtInitializeHooksTab();
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{

}

VOID NTAPI HandlePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    
}

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	return;

    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    ULONG flags;
    PPH_EMENU_ITEM miscMenu;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    flags = 0;

    if (!processItem)
        flags = PH_EMENU_DISABLED;

    miscMenu = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);

    if (miscMenu)
    {
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_HOOKS, L"Hooks", processItem), -1);
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_THREAD_ITEM threadItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Resume", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_CANCELIO,
        L"Cancel I/O", threadItem), insertIndex);

    if (!threadItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    BOOLEAN addMenuItem;
    PPH_MODULE_ITEM moduleItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    addMenuItem = FALSE;

    if (processItem = PhReferenceProcessItem(menuInfo->u.Module.ProcessId))
    {
        if (processItem->ServiceList && processItem->ServiceList->Count != 0)
            addMenuItem = TRUE;

        PhDereferenceObject(processItem);
    }

    if (!addMenuItem)
        return;

    if (menuInfo->u.Module.NumberOfModules == 1)
        moduleItem = menuInfo->u.Module.Modules[0];
    else
        moduleItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, PH_EMENU_FIND_STARTSWITH, L"Inspect", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    ModuleProcessId = menuInfo->u.Module.ProcessId;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_MODULE_SERVICES,
        L"Services", moduleItem), insertIndex);

    if (!moduleItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    ProcessTreeNewHandle = treeNewInfo->TreeNewHandle;
    
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{

}

VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{

}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
	if (HookTreeNewCreated && Isdiffhooks())
	{
		PPH_TREENEW_CONTEXT context;

		context = (PPH_TREENEW_CONTEXT)GetWindowLongPtr(HookTreeNewHandle, 0);

		TreeNew_SetRedraw(HookTreeNewHandle, FALSE);

		clearallrows();

		WepAddHooks(context);

		TreeNew_NodesStructured(HookTreeNewHandle);

		TreeNew_SetRedraw(HookTreeNewHandle, TRUE);
	}
}

static VOID NTAPI NetworkItemsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtNetworkBlockListHead.Flink;

    while (listEntry != &EtNetworkBlockListHead)
    {
        PET_NETWORK_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_NETWORK_BLOCK, ListEntry);

        // Invalidate all text.

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);
        memset(block->TextCacheValid, 0, sizeof(block->TextCacheValid));
        PhReleaseQueuedLockExclusive(&block->TextCacheLock);

        listEntry = listEntry->Flink;
    }
}

PET_PROCESS_BLOCK EtGetProcessBlock(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, ProcessItem, EmProcessItemType);
}

PET_NETWORK_BLOCK EtGetNetworkBlock(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, NetworkItem, EmNetworkItemType);
}

VOID EtInitializeProcessBlock(
    _Out_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    memset(Block, 0, sizeof(ET_PROCESS_BLOCK));
    Block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtProcessBlockListHead, &Block->ListEntry);
}

VOID EtDeleteProcessBlock(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;

    EtProcIconNotifyProcessDelete(Block);

    for (i = 1; i <= ETPRTNC_MAXIMUM; i++)
    {
        PhClearReference(&Block->TextCache[i]);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID EtInitializeNetworkBlock(
    _Out_ PET_NETWORK_BLOCK Block,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    memset(Block, 0, sizeof(ET_NETWORK_BLOCK));
    Block->NetworkItem = NetworkItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtNetworkBlockListHead, &Block->ListEntry);
}

VOID EtDeleteNetworkBlock(
    _In_ PET_NETWORK_BLOCK Block
    )
{
    ULONG i;

    for (i = 1; i <= ETNETNC_MAXIMUM; i++)
    {
        PhClearReference(&Block->TextCache[i]);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID NTAPI ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtInitializeProcessBlock(Extension, Object);
}

VOID NTAPI ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtDeleteProcessBlock(Extension);
}

VOID NTAPI NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtInitializeNetworkBlock(Extension, Object);
}

VOID NTAPI NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtDeleteNetworkBlock(Extension);
}

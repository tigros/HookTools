#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#define PHNT_VERSION PHNT_VISTA
#include <phdk.h>
#include <settings.h>

extern PPH_PLUGIN PluginInstance;
extern LIST_ENTRY EtProcessBlockListHead;
extern LIST_ENTRY EtNetworkBlockListHead;
extern HWND ProcessTreeNewHandle;
extern HWND NetworkTreeNewHandle;

#define PLUGIN_NAME2 L"ProcessHacker.HookTools"
#define SETTING_NAME_HOOK_TREE_LIST_COLUMNS (PLUGIN_NAME2 L".HookTreeListColumns")
#define SETTING_NAME_HOOK_TREE_LIST_SORT (PLUGIN_NAME2 L".HookTreeListSort")

#define ETHKTNC_TYPE 0
#define ETHKTNC_PID 1
#define ETHKTNC_PROCESS 2
#define ETHKTNC_PATH 4
#define ETHKTNC_STARTTIME 5
#define ETHKTNC_RELATIVESTARTTIME 6
#define ETHKTNC_FLAGS 3
#define ETHKTNC_COMMANDLINE 7
#define ETHKTNC_MAXIMUM 8

// main
VOID EtInitializeHooksTab(
	VOID
	);

#endif

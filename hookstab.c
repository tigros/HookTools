/*
 * Process Hacker Extended Tools -
 *   ETW hook monitoring
 *
 * Copyright (C) 2011-2015 wj32
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
#include "resource.h"
#include <colmgr.h>
#include <toolstatusintf.h>
#include "hookstabp.h"
#include "gethooks.h"
#include <stdlib.h>
#include <psapi.h>

BOOLEAN HookTreeNewCreated = FALSE;
HWND HookTreeNewHandle;

static ULONG HookTreeNewSortColumn;
static PH_SORT_ORDER HookTreeNewSortOrder;

static PPH_HASHTABLE HookNodeHashtable; // hashtable of all nodes
static PPH_LIST HookNodeList; // list of all nodes

static PH_CALLBACK_REGISTRATION HookItemAddedRegistration;
static PH_CALLBACK_REGISTRATION HookItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION HookItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION HookItemsUpdatedRegistration;
static BOOLEAN HookNeedsRedraw = FALSE;

static PH_TN_FILTER_SUPPORT FilterSupport;
static PTOOLSTATUS_INTERFACE ToolStatusInterface;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration;

#define WEWNTLC_MAXIMUM 4

typedef enum _WE_HOOK_SELECTOR_TYPE
{
	WeWindowSelectorAll,
	WeWindowSelectorProcess,
	WeWindowSelectorThread,
	WeWindowSelectorDesktop
} WE_HOOK_SELECTOR_TYPE;

typedef struct _WE_HOOK_SELECTOR
{
	WE_HOOK_SELECTOR_TYPE Type;
	union
	{
		struct
		{
			HANDLE ProcessId;
		} Process;
		struct
		{
			HANDLE ThreadId;
		} Thread;
		struct
		{
			PPH_STRING DesktopName;
		} Desktop;
	};
} WE_HOOK_SELECTOR, *PWE_HOOK_SELECTOR;

typedef struct _WE_HOOK_NODE
{
	PH_TREENEW_NODE Node;

	PH_STRINGREF TextCache[WEWNTLC_MAXIMUM];

	PPH_STRING flagstext;
	PPH_STRING pidtext;
	PPH_STRING pidpath;
    PPH_STRING StartTimeText;
    PPH_STRING RelativeStartTimeText;

	struct hook ahook;

} WE_HOOK_NODE, *PWE_HOOK_NODE;

typedef struct _WE_HOOK_TREE_CONTEXT
{
	HWND ParentWindowHandle;
	HWND TreeNewHandle;
	ULONG TreeNewSortColumn;
	PH_SORT_ORDER TreeNewSortOrder;

	PPH_HASHTABLE NodeHashtable;
	PPH_LIST NodeList;
	PPH_LIST NodeRootList;
} WE_HOOK_TREE_CONTEXT, *PWE_HOOK_TREE_CONTEXT;

typedef struct _HOOKS_CONTEXT
{
	HWND TreeNewHandle;
	WE_HOOK_TREE_CONTEXT TreeContext;
	WE_HOOK_SELECTOR Selector;

	PH_LAYOUT_MANAGER LayoutManager;

	HWND HighlightingWindow;
	ULONG HighlightingWindowCount;
} HOOKS_CONTEXT, *PHOOKS_CONTEXT;

VOID EtInitializeHooksTab(
    VOID
    )
{
    PH_ADDITIONAL_TAB_PAGE tabPage;
    PPH_ADDITIONAL_TAB_PAGE addedTabPage;
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version < TOOLSTATUS_INTERFACE_VERSION)
            ToolStatusInterface = NULL;
    }

    memset(&tabPage, 0, sizeof(PH_ADDITIONAL_TAB_PAGE));
    tabPage.Text = L"Hooks";
    tabPage.CreateFunction = EtpHookTabCreateFunction;
    tabPage.Index = MAXINT;
    tabPage.SelectionChangedCallback = EtpHookTabSelectionChangedCallback;
    tabPage.SaveContentCallback = EtpHookTabSaveContentCallback;
    tabPage.FontChangedCallback = EtpHookTabFontChangedCallback;
    addedTabPage = ProcessHacker_AddTabPage(PhMainWndHandle, &tabPage);

    if (ToolStatusInterface)
    {
        PTOOLSTATUS_TAB_INFO tabInfo;

        tabInfo = ToolStatusInterface->RegisterTabInfo(addedTabPage->Index);
        tabInfo->BannerText = L"Search Hooks";
        tabInfo->ActivateContent = EtpToolStatusActivateContent;
    }
}

HWND NTAPI EtpHookTabCreateFunction(
    _In_ PVOID Context
    )
{
    HWND hwnd;

	ULONG thinRows;

	thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
	hwnd = CreateWindow(
		PH_TREENEW_CLASSNAME,
		NULL,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows,
		0,
		0,
		3,
		3,
		PhMainWndHandle,
		NULL,
		PluginInstance->DllBase,
		NULL
		);

	if (!hwnd)
		return NULL;

	HookNodeList = PhCreateList(100);

	EtInitializeHookTreeList(hwnd);

	HookTreeNewCreated = TRUE;

    return hwnd;
}

VOID NTAPI EtpHookTabSelectionChangedCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
    if (HookTreeNewCreated && (BOOLEAN)Parameter1 && WinVerOK && Isdiffhooks())
    {
        refill();
        SetFocus(HookTreeNewHandle);
    }
}

VOID EtWriteDiskList(
	_Inout_ PPH_FILE_STREAM FileStream,
	_In_ ULONG Mode
	)
{
	PPH_LIST lines;
	ULONG i;

	lines = PhGetGenericTreeNewLines(HookTreeNewHandle, Mode);

	for (i = 0; i < lines->Count; i++)
	{
		PPH_STRING line;

		line = lines->Items[i];
		PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
		PhDereferenceObject(line);
		PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
	}

	PhDereferenceObject(lines);
}

VOID NTAPI EtpHookTabSaveContentCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
	PPH_FILE_STREAM fileStream = Parameter1;
	ULONG mode = PtrToUlong(Parameter2);

	EtWriteDiskList(fileStream, mode);
}

VOID NTAPI EtpHookTabFontChangedCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
    if (HookTreeNewHandle)
        SendMessage(HookTreeNewHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
}

PWE_HOOK_NODE WeAddHookNode(
	_Inout_ PPH_TREENEW_CONTEXT Context
	)
{
	PWE_HOOK_NODE hookNode;

	hookNode = PhAllocate(sizeof(WE_HOOK_NODE));
	memset(hookNode, 0, sizeof(WE_HOOK_NODE));
	PhInitializeTreeNewNode(&hookNode->Node);

	memset(hookNode->TextCache, 0, sizeof(PH_STRINGREF) * WEWNTLC_MAXIMUM);
	hookNode->Node.TextCache = hookNode->TextCache;
	hookNode->Node.TextCacheSize = WEWNTLC_MAXIMUM;

	PhAddItemList(HookNodeList, hookNode);

	TreeNew_NodesStructured(Context->Handle);

	return hookNode;
}

VOID WepFillHookInfo(
	_In_ PWE_HOOK_NODE Node,
	const phook hook
	)
{
	Node->ahook.entry = hook->entry;
	Node->ahook.entry_index = hook->entry_index;
	Node->ahook.ignore = hook->ignore;
	Node->ahook.object = hook->object;
	Node->ahook.origin = hook->origin;
	Node->ahook.owner = hook->owner;
	Node->ahook.target = hook->target;
}

VOID WepAddChildHookNode(
	_In_ PPH_TREENEW_CONTEXT Context,
	const phook hook
	)
{
	PWE_HOOK_NODE childNode;

	childNode = WeAddHookNode(Context);
	WepFillHookInfo(childNode, hook);
	childNode->Node.Expanded = FALSE;
	PhAddItemList(Context->FlatList, childNode);
	
	if (FilterSupport.NodeList)
		childNode->Node.Visible = PhApplyTreeNewFiltersToNode(&FilterSupport, &childNode->Node);
}

BOOL GetProcessCreationTime(DWORD dwProcessId, LPFILETIME CreationTime)
{
    BOOL bResult = FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
    if (hProcess)
    {
        FILETIME Ignore;
        bResult = GetProcessTimes(hProcess, CreationTime, &Ignore, &Ignore, &Ignore);
        CloseHandle(hProcess);
    }
    return bResult;
}

VOID WepAddHooks(
	_In_ PPH_TREENEW_CONTEXT Context
	)
{
	if (!Context)
		return;

	unsigned int i = 0;
	struct desktop_hook_item *item = NULL;

	gethooks();

	struct desktop_hook_list *const list = glbcurrent->desktop_hooks;

	for (item = list->head; item; item = item->next)
	{
		for (i = 0; i < item->hook_count; ++i)
		{
			if (!item->hook[i].ignore)
			{
				WepAddChildHookNode(Context, &item->hook[i]);
			}
		}
	}
}

VOID EtInitializeHookTreeList(
    _In_ HWND hwnd
    )
{
	HookTreeNewHandle = hwnd;
	PhSetControlTheme(HookTreeNewHandle, L"explorer");
	SendMessage(TreeNew_GetTooltips(HookTreeNewHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);

	TreeNew_SetCallback(hwnd, EtpHookTreeNewCallback, NULL);

	TreeNew_SetRedraw(hwnd, FALSE);

	// Default columns
	
	PhAddTreeNewColumn(hwnd, ETHKTNC_TYPE, TRUE, L"Type", 130, PH_ALIGN_LEFT, 0, 0);
	PhAddTreeNewColumn(hwnd, ETHKTNC_PROCESS, TRUE, L"Process", 130, PH_ALIGN_LEFT, 1, DT_PATH_ELLIPSIS);
	PhAddTreeNewColumn(hwnd, ETHKTNC_PATH, TRUE, L"Path", 700, PH_ALIGN_LEFT, 2, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx(hwnd, ETHKTNC_STARTTIME, TRUE, L"Start time", 100, PH_ALIGN_LEFT, 3, 0, TRUE);
    PhAddTreeNewColumn(hwnd, ETHKTNC_RELATIVESTARTTIME, FALSE, L"Relative start time", 180, PH_ALIGN_LEFT, 4, 0);
	PhAddTreeNewColumn(hwnd, ETHKTNC_PID, TRUE, L"PID", 100, PH_ALIGN_RIGHT, 5, DT_RIGHT);
	PhAddTreeNewColumn(hwnd, ETHKTNC_FLAGS, TRUE, L"Flags", 800, PH_ALIGN_LEFT, 6, 0);

	TreeNew_SetRedraw(hwnd, TRUE);

	PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, HookNodeList);

	if (ToolStatusInterface)
	{
		PhRegisterCallback(ToolStatusInterface->SearchChangedEvent, EtpSearchChangedHandler, NULL, &SearchChangedRegistration);
		PhAddTreeNewFilter(&FilterSupport, EtpSearchHookListFilterCallback, NULL);
	}

	PPH_TREENEW_CONTEXT context;

	context = (PPH_TREENEW_CONTEXT)GetWindowLongPtr(hwnd, 0);

	WepAddHooks(context);

	EtLoadSettingsHookTreeList();
}

VOID EtLoadSettingsHookTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_HOOK_TREE_LIST_COLUMNS);
    PhCmLoadSettings(HookTreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_HOOK_TREE_LIST_SORT);
    TreeNew_SetSort(HookTreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID EtSaveSettingsHookTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    if (!HookTreeNewCreated)
        return;

    settings = PhCmSaveSettings(HookTreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_HOOK_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(HookTreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
	PhSetIntegerPairSetting(SETTING_NAME_HOOK_TREE_LIST_SORT, sortSettings);
}

VOID EtRemoveHookNode(
    _In_ PWE_HOOK_NODE HookNode
    )
{
    ULONG index;

    if ((index = PhFindItemList(HookNodeList, HookNode)) != -1)
        PhRemoveItemList(HookNodeList, index);

    if (HookNode->flagstext) PhDereferenceObject(HookNode->flagstext);
    if (HookNode->pidpath) PhDereferenceObject(HookNode->pidpath);
    if (HookNode->pidtext) PhDereferenceObject(HookNode->pidtext);
    if (HookNode->StartTimeText) PhDereferenceObject(HookNode->StartTimeText);
    if (HookNode->RelativeStartTimeText) PhDereferenceObject(HookNode->RelativeStartTimeText);

    PhFree(HookNode);
}

void getfullpath(TCHAR *filename, HANDLE pid)
{
	HANDLE processHandle = NULL;

	processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (processHandle != NULL) {
		GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH);
		CloseHandle(processHandle);
	}
}

#define SORT_FUNCTION(Column) EtpHookTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl EtpHookTreeNewCompare##Column( \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PWE_HOOK_NODE node1 = *(PWE_HOOK_NODE *)_elem1; \
    PWE_HOOK_NODE node2 = *(PWE_HOOK_NODE *)_elem2; \
    phook hookItem1 = &node1->ahook; \
    phook hookItem2 = &node2->ahook; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
	{ \
		PPH_STRING p1; \
		PPH_STRING p2; \
		if (hookItem1->origin && hookItem2->origin) { \
		p1 = PhFormatString(L"%s", hookItem1->origin->spi->ImageName.Buffer); \
		p2 = PhFormatString(L"%s", hookItem2->origin->spi->ImageName.Buffer); \
		sortResult = PhCompareString(p1, p2, TRUE); \
		PhDereferenceObject(p1); \
		PhDereferenceObject(p2); } \
		if (sortResult == 0) \
		{ \
			const unsigned index1 = (unsigned)(hookItem1->object.iHook + 1); \
			const unsigned index2 = (unsigned)(hookItem2->object.iHook + 1); \
			if (index1 < w_hooknames_count && index2 < w_hooknames_count) \
			{ \
				PPH_STRING p3; \
				PPH_STRING p4; \
				p3 = PhFormatString(L"%s", w_hooknames[index1]); \
				p4 = PhFormatString(L"%s", w_hooknames[index2]); \
				sortResult = PhCompareString(p3, p4, TRUE); \
				PhDereferenceObject(p3); \
				PhDereferenceObject(p4); \
			} \
		} \
	} \
    return PhModifySort(sortResult, HookTreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Type)
{
	const unsigned index1 = (unsigned)(hookItem1->object.iHook + 1);
	const unsigned index2 = (unsigned)(hookItem2->object.iHook + 1);

	if (index1 < w_hooknames_count && index2 < w_hooknames_count)
	{
		PPH_STRING s1;
		PPH_STRING s2;

		s1 = PhFormatString(L"%s", w_hooknames[index1]);
		s2 = PhFormatString(L"%s", w_hooknames[index2]);

		sortResult = PhCompareString(s1, s2, TRUE);

		PhDereferenceObject(s1);
		PhDereferenceObject(s2);
	}
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Process)
{
	PPH_STRING s1;
	PPH_STRING s2;

	if (hookItem1->origin && hookItem2->origin)
	{
		s1 = PhFormatString(L"%s", hookItem1->origin->spi->ImageName.Buffer);
		s2 = PhFormatString(L"%s", hookItem2->origin->spi->ImageName.Buffer);

		sortResult = PhCompareString(s1, s2, TRUE);

		PhDereferenceObject(s1);
		PhDereferenceObject(s2);
	}
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Path)
{
	PPH_STRING s1;
	PPH_STRING s2;

	if (hookItem1->origin && hookItem2->origin)
	{
		TCHAR filename1[MAX_PATH];
		TCHAR filename2[MAX_PATH];

		getfullpath(filename1, hookItem1->origin->spi->UniqueProcessId);
		getfullpath(filename2, hookItem2->origin->spi->UniqueProcessId);

		s1 = PhFormatString(L"%s", filename1);
		s2 = PhFormatString(L"%s", filename2);

		sortResult = PhCompareString(s1, s2, TRUE);

		PhDereferenceObject(s1);
		PhDereferenceObject(s2);
	}
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PID)
{
	if (hookItem1->origin && hookItem2->origin)
	{
		sortResult = uint64cmp(hookItem1->origin->spi->UniqueProcessId, hookItem2->origin->spi->UniqueProcessId);
	}
}
END_SORT_FUNCTION

int starttimecmp(HANDLE p1, HANDLE p2)
{
    LARGE_INTEGER CreationTime1;
    LARGE_INTEGER CreationTime2;
    if (GetProcessCreationTime(p1, &CreationTime1) &&
        GetProcessCreationTime(p2, &CreationTime2))
    {
        return int64cmp(CreationTime1.QuadPart, CreationTime2.QuadPart);
    }

    return 0;
}

BEGIN_SORT_FUNCTION(StartTime)
{
    if (hookItem1->origin && hookItem2->origin)
    {
        sortResult = starttimecmp(hookItem1->origin->spi->UniqueProcessId, hookItem2->origin->spi->UniqueProcessId);
    }
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RelativeStartTime)
{
    if (hookItem1->origin && hookItem2->origin)
    {
        sortResult = -starttimecmp(hookItem1->origin->spi->UniqueProcessId, hookItem2->origin->spi->UniqueProcessId);
    }
}
END_SORT_FUNCTION

void getflags(WCHAR *buf, DWORD flags)
{

	wcscpy(buf, L"");

	if (!flags)
		return;

	if (flags & HF_GLOBAL)
		wcscat(buf, L"HF_GLOBAL ");

	if (flags & HF_ANSI)
		wcscat(buf, L"HF_ANSI ");

	if (flags & HF_NEEDHC_SKIP)
		wcscat(buf, L"HF_NEEDHC_SKIP ");

	if (flags & HF_HUNG)
		wcscat(buf, L"HF_HUNG ");

	if (flags & HF_HOOKFAULTED)
		wcscat(buf, L"HF_HOOKFAULTED ");

	if (flags & HF_NOPLAYBACKDELAY)
		wcscat(buf, L"HF_NOPLAYBACKDELAY ");

	if (flags & HF_WX86KNOWINDOWLL)
		wcscat(buf, L"HF_WX86KNOWINDOWLL ");

	if (flags & HF_DESTROYED)
		wcscat(buf, L"HF_DESTROYED ");

	if (flags & ~(DWORD)HF_VALID)
	{
		wchar_t h[50];
		swprintf(h, 50, L"<0x%08lX> ", (DWORD)(flags & ~(DWORD)HF_VALID));
		wcscat(buf, h);
	}

}

BEGIN_SORT_FUNCTION(FLAGS)
{
	WCHAR buf1[255];
	WCHAR buf2[255];

	getflags(buf1, hookItem1->object.flags);
	getflags(buf2, hookItem2->object.flags);

	PPH_STRING s1;
	PPH_STRING s2;

	s1 = PhFormatString(L"%s", buf1);
	s2 = PhFormatString(L"%s", buf2);

	sortResult = PhCompareString(s1, s2, TRUE);

	PhDereferenceObject(s1);
	PhDereferenceObject(s2);

}
END_SORT_FUNCTION

BOOLEAN NTAPI EtpHookTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    //PET_HOOK_NODE node;
	PWE_HOOK_NODE node;
	const struct gui *gui;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(PID),
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(FLAGS),
					SORT_FUNCTION(Path),
                    SORT_FUNCTION(StartTime),
                    SORT_FUNCTION(RelativeStartTime)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (HookTreeNewSortColumn < ETHKTNC_MAXIMUM)
                    sortFunction = sortFunctions[HookTreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort(HookNodeList->Items, HookNodeList->Count, sizeof(PVOID), sortFunction);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)HookNodeList->Items;
                getChildren->NumberOfChildren = HookNodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
			phook hookItem;

			node = (PWE_HOOK_NODE)getCellText->Node;

			hookItem = &node->ahook;

            switch (getCellText->Id)
            {
			case ETHKTNC_TYPE:
			{

				const unsigned index = (unsigned)(hookItem->object.iHook + 1);

				if (index < w_hooknames_count)
					PhInitializeStringRef(&getCellText->Text, w_hooknames[index]);
				else
					PhInitializeStringRef(&getCellText->Text, L"Undefined");

			}
            break;

			case ETHKTNC_PID:
			{
				gui = hookItem->origin;

				if (!gui)
					return FALSE;

				PH_FORMAT format;
				
				PhInitFormatD(&format, gui->spi->UniqueProcessId);
				PhMoveReference(&node->pidtext, PhFormat(&format, 1, 0));
				getCellText->Text = node->pidtext->sr;				
			}
            break;

			case ETHKTNC_PROCESS:
			{
				gui = hookItem->origin;

				if (gui)
					PhInitializeStringRef(&getCellText->Text, gui->spi->ImageName.Buffer);
				else
					return FALSE;
			}
			break;

			case ETHKTNC_PATH:
			{
				TCHAR filename[MAX_PATH];

				if (!hookItem->origin)
					return FALSE;

				getfullpath(filename, hookItem->origin->spi->UniqueProcessId);

				PH_FORMAT format;

				PhInitFormatS(&format, filename);
				PhMoveReference(&node->pidpath, PhFormat(&format, 1, 0));
				getCellText->Text = node->pidpath->sr;

			}
            break;
			
			case ETHKTNC_FLAGS:
			{
				WCHAR buf[255];

				getflags(buf, hookItem->object.flags);

				PH_FORMAT format;

				PhInitFormatS(&format, buf);
				PhMoveReference(&node->flagstext, PhFormat(&format, 1, 0));
				getCellText->Text = node->flagstext->sr;
			}
            break;

            case ETHKTNC_STARTTIME:
            {
                gui = hookItem->origin;

                if (!gui)
                    return FALSE;

                LARGE_INTEGER CreationTime;
                if (GetProcessCreationTime(gui->spi->UniqueProcessId, &CreationTime))
                {
                    SYSTEMTIME systemTime;

                    PhLargeIntegerToLocalSystemTime(&systemTime, &CreationTime);
                    PhMoveReference(&node->StartTimeText, PhFormatDateTime(&systemTime));
                    getCellText->Text = node->StartTimeText->sr;
                }
            }
            break;

            case ETHKTNC_RELATIVESTARTTIME:
            {
                gui = hookItem->origin;

                if (!gui)
                    return FALSE;

                LARGE_INTEGER CreationTime;
                if (GetProcessCreationTime(gui->spi->UniqueProcessId, &CreationTime))
                {
                    LARGE_INTEGER currentTime;
                    PPH_STRING startTimeString;

                    PhQuerySystemTime(&currentTime);
                    startTimeString = PhFormatTimeSpanRelative(currentTime.QuadPart - CreationTime.QuadPart);
                    PhMoveReference(&node->RelativeStartTimeText, PhConcatStrings2(startTimeString->Buffer, L" ago"));
                    PhDereferenceObject(startTimeString);
                    getCellText->Text = node->RelativeStartTimeText->sr;
                } 
            }
            break;

            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        return TRUE;
    case TreeNewGetCellTooltip:
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &HookTreeNewSortColumn, &HookTreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    EtHandleHookCommand(ID_HOOK_COPY);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(HookTreeNewHandle, 0, -1);
                break;
            case VK_RETURN:
                EtHandleHookCommand(ID_HOOK_OPENFILELOCATION);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            EtHandleHookCommand(ID_HOOK_OPENFILELOCATION);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            EtShowHookContextMenu(mouseEvent->Location);
        }
        return TRUE;
    case TreeNewDestroying:
        {
            EtSaveSettingsHookTreeList();
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING EtpGetHookItemProcessName(
    _In_ PET_HOOK_ITEM HookItem
    )
{
    PH_FORMAT format[4];

    if (!HookItem->ProcessId)
        return PhCreateString(L"No Process");

    PhInitFormatS(&format[1], L" (");
    PhInitFormatU(&format[2], HandleToUlong(HookItem->ProcessId));
    PhInitFormatC(&format[3], ')');

    if (HookItem->ProcessName)
        PhInitFormatSR(&format[0], HookItem->ProcessName->sr);
    else
        PhInitFormatS(&format[0], L"Unknown Process");

    return PhFormat(format, 4, 96);
}

phook EtGetSelectedHookItem(
    VOID
    )
{
    phook hookItem = NULL;
    ULONG i;

    for (i = 0; i < HookNodeList->Count; i++)
    {
        PWE_HOOK_NODE node = HookNodeList->Items[i];

        if (node->Node.Selected)
        {
            hookItem = &node->ahook;
            break;
        }
    }

    return hookItem;
}

VOID EtGetSelectedHookItems(
    _Out_ phook **HookItems,
    _Out_ PULONG NumberOfHookItems
    )
{
	
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < HookNodeList->Count; i++)
    {
        PWE_HOOK_NODE node = HookNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, &node->ahook);
        }
    }

    *HookItems = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfHookItems = list->Count;

    PhDereferenceObject(list);
}

VOID EtDeselectAllHookNodes(
    VOID
    )
{
    TreeNew_DeselectRange(HookTreeNewHandle, 0, -1);
}

VOID EtSelectAndEnsureVisibleHookNode(
    _In_ PET_HOOK_NODE HookNode
    )
{
    EtDeselectAllHookNodes();

    if (!HookNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(HookTreeNewHandle, &HookNode->Node);
    TreeNew_SetMarkNode(HookTreeNewHandle, &HookNode->Node);
    TreeNew_SelectRange(HookTreeNewHandle, HookNode->Node.Index, HookNode->Node.Index);
    TreeNew_EnsureVisible(HookTreeNewHandle, &HookNode->Node);
}

VOID EtCopyHookList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(HookTreeNewHandle, 0);
    PhSetClipboardString(HookTreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID EtWriteHookList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(HookTreeNewHandle, Mode);

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}

VOID EtHandleHookCommand(
    _In_ ULONG Id
    )
{
	TCHAR filename[MAX_PATH];
	phook hookItem;

    switch (Id)
    {
    case ID_HOOK_GOTOPROCESS:
        {
            hookItem = EtGetSelectedHookItem();
            PPH_PROCESS_NODE processNode;

			if (hookItem && hookItem->origin)
            {
				if (hookItem->origin->spi->UniqueProcessId)
                {
                    // Check if this is really the process that we want, or if it's just a case of PID re-use.
					if (processNode = PhFindProcessNode(hookItem->origin->spi->UniqueProcessId))
                    {
                        ProcessHacker_SelectTabPage(PhMainWndHandle, 0);
                        PhSelectAndEnsureVisibleProcessNode(processNode);
                    }
                }
            }
        }
        break;

	case ID_HOOK_UNHOOK:
		{
			if (PhShowConfirmMessage(
				PhMainWndHandle,
				L"unhook",
				L"highlighted",
				NULL,
				FALSE
				))
			{
				phook *hookItems;
				ULONG numberOfHookItems;

				EtGetSelectedHookItems(&hookItems, &numberOfHookItems);

				for (int i = 0; i < numberOfHookItems; i++)
				{
					BOOL res = UnhookWindowsHookEx((HHOOK)hookItems[i]->object.head.h);

					if (!res)
					{
						wchar_t buf[256];
						FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);

						PhShowMessage(HookTreeNewHandle, MB_ICONERROR | MB_OK, L"An error occurred: %s", buf);
					}
				}

				PhFree(hookItems);
			}
		}
		break;

	case ID_HOOK_OPENFILELOCATION:
        {
			hookItem = EtGetSelectedHookItem();

			if (hookItem && hookItem->origin)
            {
				getfullpath(filename, hookItem->origin->spi->UniqueProcessId);
				PhShellExploreFile(PhMainWndHandle, filename);  
            }
        }
        break;
    case ID_HOOK_COPY:
        {
            EtCopyHookList();
        }
        break;
    case ID_HOOK_PROPERTIES:
        {
            hookItem = EtGetSelectedHookItem();

			if (hookItem && hookItem->origin)
            {
				getfullpath(filename, hookItem->origin->spi->UniqueProcessId);
                PhShellProperties(PhMainWndHandle, filename);
            }
        }
        break;
    }
}

VOID EtpInitializeHookMenu(
    _In_ PPH_EMENU Menu,
    _In_ phook *HookItems,
    _In_ ULONG NumberOfHookItems
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfHookItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
	else if (NumberOfHookItems > 1 || !HookItems[0]->origin)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_HOOK_COPY, TRUE);
		PhEnableEMenuItem(Menu, ID_HOOK_UNHOOK, TRUE);
    }
}

VOID EtShowHookContextMenu(
    _In_ POINT Location
    )
{
    phook *hookItems;
    ULONG numberOfHookItems;

    EtGetSelectedHookItems(&hookItems, &numberOfHookItems);

    if (numberOfHookItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_HOOKS), 0);
        PhSetFlagsEMenuItem(menu, ID_HOOK_OPENFILELOCATION, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        EtpInitializeHookMenu(menu, hookItems, numberOfHookItems);

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            Location.x,
            Location.y
            );

        if (item)
        {
            EtHandleHookCommand(item->Id);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(hookItems);
}

static VOID NTAPI EtpSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhApplyTreeNewFilters(&FilterSupport);
}

static BOOLEAN NTAPI EtpSearchHookListFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PWE_HOOK_NODE hookNode = (PWE_HOOK_NODE)Node;
    PTOOLSTATUS_WORD_MATCH wordMatch = ToolStatusInterface->WordMatch;
    struct hook *hookItem = NULL;
    PH_STRINGREF sr;

    hookItem = &hookNode->ahook;

    if (PhIsNullOrEmptyString(ToolStatusInterface->GetSearchboxText()))
        return TRUE;

    if (!hookItem || !hookItem->origin)
        return FALSE;

    PhInitializeStringRef(&sr, hookItem->origin->spi->ImageName.Buffer);

    if (wordMatch(&sr))
        return TRUE;

    return FALSE;
}

VOID NTAPI EtpToolStatusActivateContent(
    _In_ BOOLEAN Select
    )
{
    SetFocus(HookTreeNewHandle);

    if (Select)
    {
        if (TreeNew_GetFlatNodeCount(HookTreeNewHandle) > 0)
            EtSelectAndEnsureVisibleHookNode((PET_HOOK_NODE)TreeNew_GetFlatNode(HookTreeNewHandle, 0));
    }
}

INT_PTR CALLBACK EtpHookTabErrorDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {

        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {

            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

void clearallrows()
{
	ULONG i;

	while (HookNodeList->Count > 0)
	{
		for (i = 0; i < HookNodeList->Count; i++)
		{
			PWE_HOOK_NODE node = HookNodeList->Items[i];
			EtRemoveHookNode(node);
		}
	}

}

void refill()
{
    PPH_TREENEW_CONTEXT context;

    context = (PPH_TREENEW_CONTEXT)GetWindowLongPtr(HookTreeNewHandle, 0);

    TreeNew_SetRedraw(HookTreeNewHandle, FALSE);

    clearallrows();

    WepAddHooks(context);

    TreeNew_NodesStructured(HookTreeNewHandle);

    TreeNew_SetRedraw(HookTreeNewHandle, TRUE);
}

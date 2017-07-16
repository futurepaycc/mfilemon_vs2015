/*
MFILEMON - print to file with automatic filename assignment
Copyright (C) 2007-2015 Monti Lorenzo

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "stdafx.h"
#include <shlobj.h>
#include "monitorUI.h"
#include "..\common\monutils.h"
#include "..\common\config.h"
#include "..\common\defs.h"
#include "..\common\autoclean.h"
#include "..\common\version.h"
#include "resource.h"

HINSTANCE g_hInstance = NULL;
static BOOL bPasswordChanged = FALSE;
//2009-08-04 the introduction of "search fields" requires a more complex
//check on the "pattern"
//static LPWSTR szInvalidCharacters = L"/:*?\"<>|";

static LPWSTR szHelpText =
#if (!defined(MFILEMONLANG) || MFILEMONLANG == 0x0409)
L"\
Field format: %[width][.start]type\n\
  width = 0..9 ('i' fields) or 0..99 (other fields).\n\
    Default is 4 for 'i' and 0 for others.\n\
    Strings are right padded with spaces, numbers are left padded with zeroes.\n\
    Negative width swaps padding.\n\
  start = first used number (only valid for 'i' field; default = 1)\n\
  type = field type code. One of:\n\
    i:  auto-increment integer (only valid for filename pattern)\n\
    f:  filename (only valid for user command)\n\
    p:  path (only valid for user command)\n\
    y:  year (2 digits)\n\
    Y:  year (4 digits)\n\
    m:  month number\n\
    M:  month name\n\
    d:  day number\n\
    D:  day name\n\
    h:  hour (12 hours)\n\
    H:  hour (24 hours)\n\
    n:  minutes\n\
    s:  seconds\n\
    t:  print job title\n\
	T:  temp directory path\n\
    j:  print job id\n\
    u:  user name (who started print job)\n\
    c:  computer name (from which came print job)\n\
    r:  printer name\n\
    b:  output bin\n\
To use the '%' character in a filename or user command, insert sequence '%%'.\n\
For filename pattern, special \"search fields\" can be specified in this manner:\n\
|literal|searchstring|\n\
searchstring can contain wildcards.\n\
Mfilemon will search for the \"searchstring\" while choosing the filename,\n\
but then will use \"literal\" in the actual filename.\n\
Examples:\n\
file%i.pdf -> file0001.pdf, file0002.pdf, ...\n\
export%Y-%m-%d-%6i.ps -> export2007-04-20-000001.ps, export2007-04-20-000002.ps, ...\n\
file.%u.%6.0i.prn -> file.Administrator.000000.prn, file.Administrator.000001.prn, ...\n\
file%i-page|%d|*|.jpg -> will examine any file in the form file%i-page*.jpg\n\
                         and then use the name file%i-page%d.jpg. Note that\n\
                         %i will be substituted with the first available integer,\n\
                         while %d will be used literally.";
static LPWSTR szLogLevelNone = L"None";
static LPWSTR szLogLevelErrors = L"Errors";
static LPWSTR szLogLevelWarnings = L"Warnings";
static LPWSTR szLogLevelAll = L"All";
#else if (MFILEMONLANG == 0x0410)
L"\
Formato dei campi: %[width][.start]type\n\
  width = 0..9 (campi 'i') o 0..99 (altri campi).\n\
    Il default ?4 per 'i' e 0 per gli altri.\n\
    Le stringhe sono riempite a destra con spazi, i numeri a sinistra con zeri.\n\
    Una width negativa inverte i riempimenti.\n\
  start = primo numero usato (valido solo per campo 'i'; default = 1)\n\
  type = codice del tipo di campo. Uno tra i seguenti:\n\
    i:  intero ad auto incremento (valido solo per formato nome file)\n\
    f:  nome file (valido solo per comando utente)\n\
    p:  percorso (valido solo per comando utente)\n\
    y:  anno (2 cifre)\n\
    Y:  anno (4 cifre)\n\
    m:  numero del mese\n\
    M:  nome del mese\n\
    d:  numero del giorno\n\
    D:  nome del giorno\n\
    h:  ora (12 ore)\n\
    H:  ora (24 ore)\n\
    n:  minuti\n\
    s:  secondi\n\
    t:  titolo del job stampa\n\
	T:  percorso directory temp\n\
    j:  id del job di stampa\n\
    u:  nome utente (che ha lanciato il job di stampa)\n\
    c:  nome computer (da cui ?partito il job di stampa)\n\
    r:  nome stampante\n\
    b:  vassoio d'uscita\n\
Per usare il carattere '%' in un nome file o comando utente, inserire la sequenza '%%'.\n\
Per i nomi file, speciali \"campi di ricerca\" possono essere specificati come segue:\n\
|stringaletterale|stringaricerca|\n\
stringaricerca pu?contenere caratteri jolly.\n\
Mfilemon cercher?\"stringaricerca\" mentre sceglie il nome del file,\n\
ma poi user?\"stringaletterale\" nel nome del file definitivo.\n\
Esempi:\n\
file%i.pdf -> file0001.pdf, file0002.pdf, ...\n\
export%Y-%m-%d-%6i.ps -> export2007-04-20-000001.ps, export2007-04-20-000002.ps, ...\n\
file.%u.%6.0i.prn -> file.Administrator.000000.prn, file.Administrator.000001.prn, ...\n\
file%i-page|%d|*|.jpg -> esaminer?i file nella forma file%i-page*.jpg, per poi usare\n\
                         il nome file%i-page%d.jpg. NB %i verr?sostituito con il primo\n\
                         intero libero, mentre %d verr?usato letteralmente.";
static LPWSTR szLogLevelNone = L"Nessuno";
static LPWSTR szLogLevelErrors = L"Errori";
static LPWSTR szLogLevelWarnings = L"Avvisi";
static LPWSTR szLogLevelAll = L"Tutto";
#endif

//-------------------------------------------------------------------------------------
void UpdateCaption(HWND hDlg)
{
	WCHAR szCaption[256];
	WCHAR szOldCaption[256];

	GetWindowTextW(hDlg, szOldCaption, LENGTHOF(szCaption));
	swprintf_s(szCaption, LENGTHOF(szCaption), L"MFILEMON %s - %s", szVersionShort, szOldCaption);
	SetWindowTextW(hDlg, szCaption);
}

//-------------------------------------------------------------------------------------
void TrimControlText(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax)
{
	GetDlgItemTextW(hDlg, nIDDlgItem, lpString, cchMax);
	Trim(lpString);
	SetDlgItemTextW(hDlg, nIDDlgItem, lpString);
}

//-------------------------------------------------------------------------------------
BOOL CheckPattern(LPCWSTR szPattern)
{
	int nPipes = 0;
	while (*szPattern)
	{
		switch (*szPattern)
		{
		case L'/':
		case L':':
		case L'"':
		case L'<':
		case L'>':
			//these are always invalid
			return FALSE;
			break;
		case L'|':
			//a pipe encountered... we are entering or leaving a search field
			nPipes++;
			nPipes %= 3;
			break;
		case L'*':
		case L'?':
			//wildcards are only allowed inside the "search string" of a "search field"
			if (nPipes != 2)
				return FALSE;
			break;
		}
		szPattern++;
	}
	return nPipes == 0; //all search fields must be correctly enclosed in | | |
}

//----------------------------ѡ���ļ���-------------------------------------------
void DoBrowse(HWND hDlg, int nIDDlgItem)
{
	WCHAR pszDisplayName[MAX_PATH + 1];
	BROWSEINFO bi;
	bi.hwndOwner		= hDlg;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= pszDisplayName;
	bi.lpszTitle		= szMsgBrowseFolderTitle;
	bi.ulFlags			= BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS;
	bi.lpfn				= NULL;
	bi.lParam			= 0;
	bi.iImage			= 0;
	LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if (pidl)
	{
		WCHAR szPath[MAX_PATH + 1];
		SHGetPathFromIDListW(pidl, szPath);
		IMalloc* pMalloc;
		SHGetMalloc(&pMalloc);
		pMalloc->Free(pidl);
		pMalloc->Release();
		SetDlgItemTextW(hDlg, nIDDlgItem, szPath);
	}
}

//------------------------�����˿� �Ի��� ���ں���-----------------------------------------------
BOOL CALLBACK AddPortUIDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	static LPPORTCONFIG ppc = NULL;

	switch (uMessage)
	{
	case WM_INITDIALOG:
		UpdateCaption(hDlg);

		ppc = (LPPORTCONFIG)lParam;
		//Nome porta
		SetFocus(GetDlgItem(hDlg, ID_EDTPORTNAME));
		return TRUE;

	case WM_COMMAND:
		_ASSERTE(ppc != NULL);
		switch (LOWORD(wParam))
		{
		case IDOK:
			TrimControlText(hDlg, ID_EDTPORTNAME, ppc->szPortName, LENGTHOF(ppc->szPortName));
			//check nome porta
			if (*ppc->szPortName == L'\0' || wcspbrk(ppc->szPortName, L"\\"))
			{
				MessageBoxW(hDlg, szMsgInvalidPortName, szAppTitle, MB_OK);
				SetFocus(GetDlgItem(hDlg, ID_EDTPORTNAME));
				return TRUE;
			}
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

//---------------------���ö˿� �Ի��� ���ں���---------------------------------------------
BOOL CALLBACK MonitorUIDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd;
	static LPPORTCONFIG ppc = NULL;

	switch (uMessage)
	{

	case WM_INITDIALOG://�Ի����ʼ������Ϣ�����
		UpdateCaption(hDlg);

		ppc = (LPPORTCONFIG)lParam;		//�ӷ���˶�ȡ���Ĵ���������Ϣ
		//Nome porta
		SetDlgItemTextW(hDlg, ID_EDTPORTNAME, ppc->szPortName);
		//Output path
		hWnd = GetDlgItem(hDlg, ID_EDTOUTPUTPATH);
		if (hWnd)
		{
			SetWindowTextW(hWnd, ppc->szOutputPath);
			SetFocus(hWnd);
		}
		//File pattern
		SetDlgItemTextW(hDlg, ID_EDTFILEPATTERN, ppc->szFilePattern);
		//Overwrite
		hWnd = GetDlgItem(hDlg, ID_CHKOVERWRITE);
		if (hWnd)
			SendMessageW(hWnd, BM_SETCHECK, ppc->bOverwrite ? BST_CHECKED : BST_UNCHECKED, 0);
		//User command
		SetDlgItemTextW(hDlg, ID_EDTUSERCOMMAND, ppc->szUserCommandPattern);
		//Execute from
		SetDlgItemTextW(hDlg, ID_EDTCMDPATH, ppc->szExecPath);
		//Pipe data
		hWnd = GetDlgItem(hDlg, ID_PIPEDATA);
		if (hWnd)
			SendMessageW(hWnd, BM_SETCHECK, ppc->bPipeData ? BST_CHECKED : BST_UNCHECKED, 0);
		//Wait termination
		hWnd = GetDlgItem(hDlg, ID_CHKWAITTERMINATION);
		if (hWnd)
			SendMessageW(hWnd, BM_SETCHECK, ppc->bWaitTermination ? BST_CHECKED : BST_UNCHECKED, 0);
		//Log Level
		hWnd = GetDlgItem(hDlg, ID_CBLOGLEVEL);
		if (hWnd)
		{
			SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)szLogLevelNone);
			SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)szLogLevelErrors);
			SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)szLogLevelWarnings);
			SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)szLogLevelAll);
			SendMessageW(hWnd, CB_SETCURSEL, ppc->nLogLevel, 0);
		}
		SetDlgItemTextW(hDlg, ID_EDTUSER, ppc->szUser);
		SetDlgItemTextW(hDlg, ID_EDTDOMAIN, ppc->szDomain);
		SetDlgItemTextW(hDlg, ID_EDTPASSWORD, L"xxxxxxxxxxxxxxxxxxxx");
		bPasswordChanged = FALSE;
		return TRUE;

	case WM_COMMAND://���öԻ��� ��������
		_ASSERTE(ppc != NULL);
		switch (LOWORD(wParam))
		{
		case ID_BTNBROWSE:
			DoBrowse(hDlg, ID_EDTOUTPUTPATH);
			return TRUE;
		case ID_BTNBROWSE2:
			DoBrowse(hDlg, ID_EDTCMDPATH);
			return TRUE;
		case ID_BTNHELP:
		case ID_BTNHELP2:
			MessageBoxW(hDlg, szHelpText, szAppTitle, MB_OK);
			return TRUE;
		case ID_EDTPASSWORD:
			if (HIWORD(wParam) == EN_CHANGE)
				bPasswordChanged = TRUE;
			return TRUE;
		case IDOK:
			{
				TrimControlText(hDlg, ID_EDTOUTPUTPATH, ppc->szOutputPath, LENGTHOF(ppc->szOutputPath));
				TrimControlText(hDlg, ID_EDTFILEPATTERN, ppc->szFilePattern, LENGTHOF(ppc->szFilePattern));
				hWnd = GetDlgItem(hDlg, ID_CHKOVERWRITE);
				if (hWnd)
				{
					switch (SendMessageW(hWnd, BM_GETCHECK, 0, 0))
					{
					case BST_CHECKED:
						ppc->bOverwrite = TRUE;
						break;
					case BST_UNCHECKED:
						ppc->bOverwrite = FALSE;
						break;
					default:
						_ASSERTE(FALSE);
						ppc->bOverwrite = FALSE;
						break;
					}
				}
				TrimControlText(hDlg, ID_EDTUSERCOMMAND, ppc->szUserCommandPattern, LENGTHOF(ppc->szUserCommandPattern));
				TrimControlText(hDlg, ID_EDTCMDPATH, ppc->szExecPath, LENGTHOF(ppc->szExecPath));
				//Pipe data
				hWnd = GetDlgItem(hDlg, ID_PIPEDATA);
				if (hWnd)
				{
					switch (SendMessageW(hWnd, BM_GETCHECK, 0, 0))
					{
					case BST_CHECKED:
						ppc->bPipeData = TRUE;
						break;
					case BST_UNCHECKED:
						ppc->bPipeData = FALSE;
						break;
					default:
						_ASSERTE(FALSE);
						ppc->bPipeData = FALSE;
						break;
					}
				}
				//Wait termination
				hWnd = GetDlgItem(hDlg, ID_CHKWAITTERMINATION);
				if (hWnd)
				{
					switch (SendMessageW(hWnd, BM_GETCHECK, 0, 0))
					{
					case BST_CHECKED:
						ppc->bWaitTermination = TRUE;
						break;
					case BST_UNCHECKED:
						ppc->bWaitTermination = FALSE;
						break;
					default:
						_ASSERTE(FALSE);
						ppc->bWaitTermination = FALSE;
						break;
					}
				}
				//Log Level
				hWnd = GetDlgItem(hDlg, ID_CBLOGLEVEL);
				if (hWnd)
					ppc->nLogLevel = (int)SendMessageW(hWnd, CB_GETCURSEL, 0, 0);
				TrimControlText(hDlg, ID_EDTUSER, ppc->szUser, LENGTHOF(ppc->szUser));
				TrimControlText(hDlg, ID_EDTDOMAIN, ppc->szDomain, LENGTHOF(ppc->szDomain));
				if (bPasswordChanged)
					GetDlgItemTextW(hDlg, ID_EDTPASSWORD, ppc->szPassword, LENGTHOF(ppc->szPassword));

				//check percorso di output
				/*
				//2009-05-14 no need to test this anymore, since we're going to
				//create missing directories through RecursiveCreateFolder
				if (!DirectoryExists(ppc->szOutputPath))
				{
#if (!defined(MFILEMONLANG) || MFILEMONLANG == 0x0409)
					MessageBox(hDlg, L"Insert a valid output path.", szAppTitle, MB_OK);
#else if (MFILEMONLANG == 0x0410)
					MessageBox(hDlg, L"Inserire un percorso di output valido.", szAppTitle, MB_OK);
#endif
					SetFocus(GetDlgItem(hDlg, ID_EDTOUTPUTPATH));
					return TRUE;
				}
				*/
				//check nome file
				if (*ppc->szFilePattern == L'\0')
				{
					MessageBoxW(hDlg, szMsgProvideFileName, szAppTitle, MB_OK);
					SetFocus(GetDlgItem(hDlg, ID_EDTFILEPATTERN));
					return TRUE;
				}
/*
				else if (wcspbrk(ppc->szFilePattern, szInvalidCharacters) != NULL)
				{
					WCHAR szBuf[512];
					swprintf_s(szBuf, LENGTHOF(szBuf), szMsgProvideFileName,
						szMsgInvalidFileName, szInvalidCharacters);
					MessageBoxW(hDlg, szBuf, szAppTitle, MB_OK);
					SetFocus(GetDlgItem(hDlg, ID_EDTFILEPATTERN));
					return TRUE;
				}
*/
				else if (!CheckPattern(ppc->szFilePattern))
				{
					WCHAR szBuf[512];
					swprintf_s(szBuf, LENGTHOF(szBuf), L"%s\r\n%s", szMsgProvideFileName,
						szMsgInvalidFileName);
					MessageBoxW(hDlg, szBuf, szAppTitle, MB_OK);
					SetFocus(GetDlgItem(hDlg, ID_EDTFILEPATTERN));
					return TRUE;
				}

				EndDialog(hDlg, IDOK);
				return TRUE;
			}

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

//---------------------------�����˿� �Ի������-------------------------------------------
BOOL WINAPI MfmAddPortUI(PCWSTR pszServer, HWND hWnd, PCWSTR pszMonitorNameIn,
						 PWSTR* ppszPortNameOut)
{
	if (!hWnd || !IsWindow(hWnd))
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	if (pszServer != NULL)
	{
		MessageBoxW(hWnd, szMsgNoAddOnRemoteSvr, szAppTitle, MB_OK);
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	size_t len = 12 + wcslen(pszMonitorNameIn) + 1;
	HANDLE hHeap = GetProcessHeap();
	LPWSTR pszPrinter;
	if ((pszPrinter = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR))) == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	swprintf_s(pszPrinter, len, L",XcvMonitor %ls", pszMonitorNameIn);
	CPrinterHandle printer(pszPrinter, SERVER_ACCESS_ADMINISTER);
	HeapFree(hHeap, 0, pszPrinter);

	if (!printer.Handle())
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	BOOL bRes = FALSE;
	PORTCONFIG pc = {0};
	DWORD cbOutputNeeded, dwStatus;

	for (;;)
	{
		//1����ȡ�û�����
		if (DialogBoxParamW(g_hInstance, MAKEINTRESOURCE(IDD_ADDPORTUI),
			hWnd, (DLGPROC)AddPortUIDlgProc, (LPARAM)&pc) == IDCANCEL)
		{
			SetLastError(ERROR_CANCELLED);
			return FALSE;
		}

		BOOL bPortExists;

		//2����֤�˿����Ƿ����(����monitor�е�MfmXcvDataPort��������Ӧ���ִ������涼�ж���)
		bRes = XcvDataW(printer, L"PortExists", (PBYTE)pc.szPortName, sizeof(pc.szPortName),
			(PBYTE)&bPortExists, sizeof(bPortExists), &cbOutputNeeded, &dwStatus);
		if (!bRes || dwStatus != ERROR_SUCCESS)
		{
			SetLastError(dwStatus);
			return FALSE;
		}

		if (!bPortExists)
			break;

		MessageBoxW(hWnd, szMsgPortExists, szAppTitle, MB_OK);
	}

	//�˿ڲ����ڣ�ִ��monitor�е���Ӷ˿��߼�
	bRes = XcvDataW(printer, L"AddPort", (PBYTE)pc.szPortName, sizeof(pc.szPortName),
		NULL, 0, &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	//�򿪶˿����öԻ��򣬹��û�����������Ϣ
	if (DialogBoxParamW(g_hInstance, MAKEINTRESOURCE(IDD_MONITORUI),
		hWnd, (DLGPROC)MonitorUIDlgProc, (LPARAM)&pc) == IDCANCEL)
	{
		SetLastError(ERROR_CANCELLED);
		return FALSE;
	}

	//�������û�д���ִ����Ӧ������
	bRes = XcvDataW(printer, L"SetConfig", (PBYTE)&pc, sizeof(pc),
		NULL, 0, &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	if (ppszPortNameOut)
	{
		size_t len = (wcslen(pc.szPortName) + 1) * sizeof(WCHAR);
		*ppszPortNameOut = (PWSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, len);
		if (*ppszPortNameOut)
			CopyMemory(*ppszPortNameOut, pc.szPortName, len);
	}

	//tutto OK, usciamo con TRUE
	return TRUE;
}


//----------------------���ö˿� �Ի������----------------------------------------------
BOOL WINAPI MfmConfigurePortUI(PCWSTR pszServer, HWND hWnd, PCWSTR pszPortName)
{
	if (!hWnd || !IsWindow(hWnd))
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	if (pszServer != NULL)
	{
		MessageBoxW(hWnd, szMsgNoConfigOnRemoteSvr, szAppTitle, MB_OK);
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	size_t len = 9 + wcslen(pszPortName) + 1;
	HANDLE hHeap = GetProcessHeap();
	LPWSTR pszPrinter;
	if ((pszPrinter = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR))) == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	swprintf_s(pszPrinter, len, L",XcvPort %ls", pszPortName);
	CPrinterHandle printer(pszPrinter, SERVER_ACCESS_ADMINISTER);
	HeapFree(hHeap, 0, pszPrinter);

	if (!printer.Handle())
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	BOOL bRes = FALSE;
	PORTCONFIG pc = {0};
	DWORD cbOutputNeeded, dwStatus;

	//��ȡԭ��������Ϣ���л��� 
	bRes = XcvDataW(printer, L"GetConfig", NULL, 0,(PBYTE)&pc, sizeof(pc), &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	//�򿪶˿����öԻ��������������
	if (DialogBoxParamW(g_hInstance, MAKEINTRESOURCE(IDD_MONITORUI),hWnd, (DLGPROC)MonitorUIDlgProc, (LPARAM)&pc) == IDCANCEL)
	{
		SetLastError(ERROR_CANCELLED);
		return FALSE;
	}

	//���µ��÷���˽��ж˿�������Ϣ����
	bRes = XcvDataW(printer, L"SetConfig", (PBYTE)&pc, sizeof(pc), NULL, 0, &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	//tutto OK, usciamo con TRUE
	return TRUE;
}

//-------------------------ɾ���˿� �������--------------------------------------------------
BOOL WINAPI MfmDeletePortUI(PCWSTR pszServer, HWND hWnd, PCWSTR pszPortName)
{
	if (!hWnd || !IsWindow(hWnd))
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	if (pszServer != NULL)
	{
		MessageBoxW(hWnd, szMsgNoDropOnRemoteSvr, szAppTitle, MB_OK);
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	size_t len = 9 + wcslen(pszPortName) + 1;
	HANDLE hHeap = GetProcessHeap();
	LPWSTR pszPrinter;
	if ((pszPrinter = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR))) == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	swprintf_s(pszPrinter, len, L",XcvPort %ls", pszPortName);
	CPrinterHandle printer(pszPrinter, SERVER_ACCESS_ADMINISTER);
	HeapFree(hHeap, 0, pszPrinter);

	if (!printer.Handle())
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	BOOL bRes = FALSE;
	DWORD cbOutputNeeded, dwStatus;

	//����ɾ���˿�
	bRes = XcvDataW(printer, L"DeletePort", (PBYTE)pszPortName,
		((DWORD)wcslen(pszPortName) + 1) * sizeof(WCHAR), NULL, 0, &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	//����ɾ���˿ڵĺ�������
	bRes = XcvDataW(printer, L"PortDeleted", NULL, 0,
		NULL, 0, &cbOutputNeeded, &dwStatus);
	if (!bRes || dwStatus != ERROR_SUCCESS)
	{
		SetLastError(dwStatus);
		return FALSE;
	}

	//tutto OK, usciamo con TRUE
	return TRUE;
}

//----------------------------��ʼ����----------------------------------------------
PMONITORUI WINAPI InitializePrintMonitorUI()
{
	static MONITORUI themonui;

	themonui.dwMonitorUISize	= sizeof(themonui);
	themonui.pfnAddPortUI		= MfmAddPortUI;
	themonui.pfnConfigurePortUI	= MfmConfigurePortUI;
	themonui.pfnDeletePortUI	= MfmDeletePortUI;

	return &themonui;
}

//---------------------------DLL��ڿ�---------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			g_hInstance = hinstDLL;
// see here http://msdn.microsoft.com/en-us/library/ms682659%28v=vs.85%29.aspx
// why the following call should not be done
//			DisableThreadLibraryCalls((HMODULE)hinstDLL);
			break;
		}
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

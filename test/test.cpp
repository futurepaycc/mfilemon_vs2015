// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;


/************************************************************************/
/* 报你妹的句柄错误啊                                                      */
/************************************************************************/
void test1(){
	LPTSTR pPrinterName = L"Generic / Text Only";
	char portName[] = "M_PORT";
	HANDLE hPrinter;
	BOOL bPortExists;
	DWORD cbOutputNeeded, dwStatus;


	//这样就能打开打印机了
	cout << OpenPrinter(pPrinterName, &hPrinter, NULL) << endl;


	BOOL result = XcvDataW(hPrinter, L"PortExists", (PBYTE)portName, sizeof(portName),
		(PBYTE)&bPortExists, sizeof(bPortExists), &cbOutputNeeded, &dwStatus);

	if (result){
		cout << "sucess" << endl;
	}
	else{
		cout << "failter" << endl;
		cout << GetLastError() << endl;//输出6，无效的句柄!
	}

	ClosePrinter(hPrinter);
}


/************************************************************************/
/* 这样加出的来的端口不是可配置类型的，和mfileport未发生关系啊                    */
/************************************************************************/
void test2(){
	HANDLE hXVCPrinter = NULL;
	LPWSTR wsPortName = L"m_port3";
	DWORD dwNeeded, dwStatus;
	PRINTER_DEFAULTS PrinterDefaults;

	PrinterDefaults.pDatatype = NULL;
	PrinterDefaults.pDevMode = NULL;
	PrinterDefaults.DesiredAccess = SERVER_ACCESS_ADMINISTER;

	/*
	现象1：用无效的打印机名字加权限能加上一个无法配置的m_port3端口
	现象3：用有效的打印名字加上权限报无效的句柄，同上
	现象2：用有效的打印名字无法加上端口并无权限配置，报无权限(拒绝访问)
	*/
	if (OpenPrinter(L"Generic / Text Only", &hXVCPrinter, &PrinterDefaults))
	{
		if (!XcvData(hXVCPrinter, L"AddPort", (LPBYTE)wsPortName,
			(wcslen(wsPortName) + 1) * 2, NULL, 0, &dwNeeded, &dwStatus) ||
			!(dwStatus == ERROR_SUCCESS || dwStatus == ERROR_ALREADY_EXISTS))
		{
			cout << "error 52: " << GetLastError() << endl;
		}
		else
		{
			cout << L"Port added successfully. - Status = %d - Port" << endl;
		}

		ClosePrinter(hXVCPrinter);
	}
	else
	{
		cout << "error 63: " << GetLastError() << endl;
	}
}







int _tmain(int argc, _TCHAR* argv[])
{
	test2();
	return 0;
}


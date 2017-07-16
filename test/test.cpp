// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;


/************************************************************************/
/* �����õľ������                                                      */
/************************************************************************/
void test1(){
	LPTSTR pPrinterName = L"Generic / Text Only";
	char portName[] = "M_PORT";
	HANDLE hPrinter;
	BOOL bPortExists;
	DWORD cbOutputNeeded, dwStatus;


	//�������ܴ򿪴�ӡ����
	cout << OpenPrinter(pPrinterName, &hPrinter, NULL) << endl;


	BOOL result = XcvDataW(hPrinter, L"PortExists", (PBYTE)portName, sizeof(portName),
		(PBYTE)&bPortExists, sizeof(bPortExists), &cbOutputNeeded, &dwStatus);

	if (result){
		cout << "sucess" << endl;
	}
	else{
		cout << "failter" << endl;
		cout << GetLastError() << endl;//���6����Ч�ľ��!
	}

	ClosePrinter(hPrinter);
}


/************************************************************************/
/* �����ӳ������Ķ˿ڲ��ǿ��������͵ģ���mfileportδ������ϵ��                    */
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
	����1������Ч�Ĵ�ӡ�����ּ�Ȩ���ܼ���һ���޷����õ�m_port3�˿�
	����3������Ч�Ĵ�ӡ���ּ���Ȩ�ޱ���Ч�ľ����ͬ��
	����2������Ч�Ĵ�ӡ�����޷����϶˿ڲ���Ȩ�����ã�����Ȩ��(�ܾ�����)
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


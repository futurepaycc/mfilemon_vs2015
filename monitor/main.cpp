#include "stdafx.h"
#include "portlist.h"
#include <tchar.h>
#include <iostream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[]){
	g_pPortList = new CPortList(szMonitorName, szDescription);
	//CPortList pl(L"mfilemon", L"Multi file port");
	g_pPortList->LoadFromRegistry();
	//pl.SaveToRegistry();
	//pl.~CPortList();
	//pl.EnumMultiFilePorts()


	cout << " output some thing" << endl;


	Sleep(3000);

	delete g_pPortList;
	return 0;
}
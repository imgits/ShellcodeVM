#pragma once
#include <string>
#include <Windows.h>
#include <DbgHelp.h>

using namespace std;

#pragma comment(lib,"DbgHelp.lib")

class WinMiniDump
{
private:
	HANDLE m_hFile;
	HANDLE m_hMapFile;
	char*  m_MapBuf;
	HANDLE m_hProcess;
	DWORD  m_ProcessId;
	MINIDUMP_TYPE m_DumpType;

public:
	WinMiniDump();
	~WinMiniDump();
	bool Create(char* Filename, HANDLE hProcess=NULL, DWORD ProcessId=0, MINIDUMP_TYPE DumpType= MiniDumpWithFullMemory);
	bool Open(char* Filename);
	bool Close();
	bool DumpHeader();
	bool DumpDirectory();
	bool DumpThreadList(PMINIDUMP_DIRECTORY dir);
	bool DumpModuleList(PMINIDUMP_DIRECTORY dir);
	bool DumpMemoryList(PMINIDUMP_DIRECTORY dir);
	bool DumpMemory64List(PMINIDUMP_DIRECTORY dir);
	bool DumpMemoryInfoList(PMINIDUMP_DIRECTORY dir);

private:
	void ShowError(char* Note=NULL, DWORD Error=0);

};


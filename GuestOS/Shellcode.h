#pragma once
#include <Windows.h>
#include "os.h"
#include "paging.h"

#define SHELLCODE_BUF_PHYSICAL_ADDR		0x01000000
#define SHELLCODE_BUF_VIRTUAL_ADDR		0x00100000
#define SHELLCODE_BUF_SIZE				MB(16)

#define SHELLCODE_STACK_VIRTUAL_ADDR   0x01000000//(SHELLCODE_BUF_VIRTUAL_ADDR + SHELLCODE_BUF_SIZE) 

#define SHELLCODE_TEB_PHYSICAL_ADDR		0x02000000	
#define SHELLCODE_TEB_VIRTUAL_ADDR		0x7EFDD000
#define SHELLCODE_TEB_SIZE				PAGE_SIZE

#define DUMMY_PEB_PHYSICAL_ADDR			0x02001000	
#define DUMMY_PEB_VIRTUAL_ADDR			0x7EFDE000
#define DUMMY_PEB_SIZE					PAGE_SIZE

#define KERNEL32_PHYSICAL_ADDR			0x02002000
#define KERNEL32_VIRTUAL_ADDR			0x76D40000
#define KERNEL32_SIZE					0x00110000

struct PEB_LDR_DATA
{
	DWORD dwLength;
	DWORD dwInitialized;
	LPVOID lpSsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	LPVOID lpEntryInProgress;
};

struct PEB
{
	BYTE bInheritedAddressSpace;
	BYTE bReadImageFileExecOptions;
	BYTE bBeingDebugged;
	BYTE bSpareBool;
	LPVOID lpMutant;
	LPVOID lpImageBaseAddress;
	PEB_LDR_DATA* pLdr;
	//...
};

struct UNICODE_STR
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  pBuffer;
};

struct LDR_MODULE
{
	LIST_ENTRY InLoadOrderModuleList;			//按加载模块顺序构成的模块链表
	LIST_ENTRY InMemoryOrderModuleList;			//按内存顺序的模块链表
	LIST_ENTRY InInitializationOrderModuleList; //按初始化顺序的模块链表
	PVOID BaseAddress; //模块的基地址
	PVOID EntryPoint; //模块的入口
	ULONG SizeOfImage; //模块镜像大小
	UNICODE_STR FullDllName; //路径名
	UNICODE_STR BaseDllName; //模块名
	ULONG Flags;
	SHORT LoadCount; //引用计数
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
};

struct DUMMY_PEB_MODULE_LIST
{
	PEB*            ppeb;
	PEB				peb;
	PEB_LDR_DATA	peb_ldr_data;
	LDR_MODULE		kernel32_modules[5];
	//...
};

static void clone_PEB_Module_list(DUMMY_PEB_MODULE_LIST& peb_module_list)
{
	memset(&peb_module_list, 0, sizeof(peb_module_list));
	PEB* pPeb = (PEB*)__readfsdword(0x30);//read (TEB)

	peb_module_list.ppeb = &peb_module_list.peb;
	memcpy(&peb_module_list.peb, pPeb, sizeof(PEB));
	peb_module_list.peb.pLdr = &peb_module_list.peb_ldr_data;

	PEB_LDR_DATA* pPebLdrData = pPeb->pLdr;//read (PEB)
	memcpy(&peb_module_list.peb_ldr_data, pPebLdrData, sizeof(PEB_LDR_DATA));

	LDR_MODULE* module_kernel32 = NULL;
	for (PLIST_ENTRY pLink = pPebLdrData->InMemoryOrderModuleList.Flink; pLink != &pPebLdrData->InMemoryOrderModuleList; pLink = pLink->Flink)
	{
		LDR_MODULE* pModule = (LDR_MODULE*)CONTAINING_RECORD(pLink, LDR_MODULE, InMemoryOrderModuleList);
		// get pointer to current modules name (unicode string)
		WCHAR* module_name_buf = pModule->BaseDllName.pBuffer;
		UINT32 module_name_len = pModule->BaseDllName.Length;
		if (module_name_buf == NULL) break;

		//printf("%08X %ws %s\n", pModule->BaseAddress, module_name_buf, find_module_name((UINT32)pModule));
		if ((module_name_len == sizeof(L"kernel32.dll") - 2) &&
			(_memicmp(module_name_buf, L"kernel32.dll", module_name_len) == 0)
			)
		{
			module_kernel32 = pModule;
			//break;
		}
	}

	peb_module_list.peb_ldr_data.InInitializationOrderModuleList.Flink = &peb_module_list.kernel32_modules[0].InInitializationOrderModuleList;
	peb_module_list.peb_ldr_data.InLoadOrderModuleList.Flink = &peb_module_list.kernel32_modules[0].InLoadOrderModuleList;
	peb_module_list.peb_ldr_data.InMemoryOrderModuleList.Flink = &peb_module_list.kernel32_modules[0].InMemoryOrderModuleList;

	peb_module_list.peb_ldr_data.InInitializationOrderModuleList.Blink = &peb_module_list.kernel32_modules[4].InInitializationOrderModuleList;
	peb_module_list.peb_ldr_data.InLoadOrderModuleList.Blink = &peb_module_list.kernel32_modules[4].InLoadOrderModuleList;
	peb_module_list.peb_ldr_data.InMemoryOrderModuleList.Blink = &peb_module_list.kernel32_modules[4].InMemoryOrderModuleList;

	for (int i = 0; i < 5; i++)
	{
		memcpy(&peb_module_list.kernel32_modules[i], module_kernel32, sizeof(LDR_MODULE));

		if (i == 0)
		{
			peb_module_list.kernel32_modules[i].InInitializationOrderModuleList.Blink = &peb_module_list.peb_ldr_data.InInitializationOrderModuleList;
			peb_module_list.kernel32_modules[i].InLoadOrderModuleList.Blink = &peb_module_list.peb_ldr_data.InLoadOrderModuleList;
			peb_module_list.kernel32_modules[i].InMemoryOrderModuleList.Blink = &peb_module_list.peb_ldr_data.InMemoryOrderModuleList;
		}
		else
		{
			peb_module_list.kernel32_modules[i].InInitializationOrderModuleList.Blink = &peb_module_list.kernel32_modules[i - 1].InInitializationOrderModuleList;
			peb_module_list.kernel32_modules[i].InLoadOrderModuleList.Blink = &peb_module_list.kernel32_modules[i - 1].InLoadOrderModuleList;
			peb_module_list.kernel32_modules[i].InMemoryOrderModuleList.Blink = &peb_module_list.kernel32_modules[i - 1].InMemoryOrderModuleList;
		}

		if (i == 4)
		{
			peb_module_list.kernel32_modules[i].InInitializationOrderModuleList.Flink = &peb_module_list.peb_ldr_data.InInitializationOrderModuleList;
			peb_module_list.kernel32_modules[i].InLoadOrderModuleList.Flink = &peb_module_list.peb_ldr_data.InLoadOrderModuleList;
			peb_module_list.kernel32_modules[i].InMemoryOrderModuleList.Flink = &peb_module_list.peb_ldr_data.InMemoryOrderModuleList;
		}
		else
		{
			peb_module_list.kernel32_modules[i].InInitializationOrderModuleList.Flink = &peb_module_list.kernel32_modules[i + 1].InInitializationOrderModuleList;
			peb_module_list.kernel32_modules[i].InLoadOrderModuleList.Flink = &peb_module_list.kernel32_modules[i + 1].InLoadOrderModuleList;
			peb_module_list.kernel32_modules[i].InMemoryOrderModuleList.Flink = &peb_module_list.kernel32_modules[i + 1].InMemoryOrderModuleList;
		}
	}
}

class Shellcode
{
public:
	UINT32 m_entry_point;
	UINT32 m_code_size;
public:
	Shellcode();
	~Shellcode();
};


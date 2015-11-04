#include "WinMiniDump.h"

#define  DefaultFilename = "MiniDump.dmp";
WinMiniDump::WinMiniDump()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapFile = NULL;
	m_MapBuf=NULL;
	m_hProcess=NULL;
	m_ProcessId=0;
	m_DumpType= MiniDumpWithFullMemory;
}

WinMiniDump::~WinMiniDump()
{

}

bool WinMiniDump::Close()
{
	if (m_MapBuf) UnmapViewOfFile(m_MapBuf);
	if (m_hMapFile) CloseHandle(m_hMapFile);
	if (m_hFile!=INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapFile = NULL;
	m_MapBuf = NULL;
	return true;
}

bool WinMiniDump::Create(char* Filename, HANDLE hProcess, DWORD ProcessId, MINIDUMP_TYPE DumpType)
{
	Close();
	m_hProcess = (hProcess != NULL) ?  hProcess : GetCurrentProcess();
	m_ProcessId= (ProcessId !=0) ? ProcessId : GetCurrentProcessId();
	m_DumpType = DumpType;
	// 创建一个Dump文件
	m_hFile = CreateFile(Filename, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		ShowError("WinMiniDump::Create::CreateFile");
		return false;
	}

	BOOL ret = MiniDumpWriteDump(
		m_hProcess,
		m_ProcessId,
		m_hFile, 
		m_DumpType,
		NULL, NULL, NULL);
	if (!ret)
	{
		ShowError("WinMiniDump::Create::MiniDumpWriteDump");
		return false;
	}
	return true;
}

bool WinMiniDump::Open(char* Filename)
{
	Close();
	// 打开一个Dump文件
	m_hFile = CreateFile(Filename, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		ShowError("WinMiniDump::Open::CreateFile");
		return false;
	}

	DWORD fsize = GetFileSize(m_hFile, NULL);

	m_hMapFile = CreateFileMapping(
		m_hFile,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,
		0,                       // max. object size
		fsize,                    // buffer size
		NULL);                 // name of mapping object

	if (m_hMapFile == NULL)
	{
		ShowError("WinMiniDump::Open::CreateFileMapping");
		return false;
	}

	m_MapBuf = (char*)MapViewOfFile(
		m_hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		fsize);

	if (m_MapBuf == NULL)
	{
		ShowError("WinMiniDump::Open::MapViewOfFile");
		return false;
	}
	return true;
}

void* WinMiniDump::GetStream(MINIDUMP_STREAM_TYPE type)
{
	PMINIDUMP_DIRECTORY StreamDir = NULL;
	PVOID               StreamPointer = 0;
	ULONG               StreamSize = 0;
	BOOL ret = MiniDumpReadDumpStream(
		m_MapBuf,
		type,
		&StreamDir,
		&StreamPointer,
		&StreamSize);
	if (ret && StreamPointer) return StreamPointer;
	return NULL;
}

void* WinMiniDump::RvaToAddress(uint32_t rva)
{
	return (void*)(m_MapBuf + rva);
}

void WinMiniDump::ShowError(char* Note, DWORD Error)
{
	LPVOID lpvMessageBuffer;
	if (Note == NULL) Note = "WinMiniDump Error";
	if (Error == 0) Error = GetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, Error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpvMessageBuffer, 0, NULL);

	printf("%s:%s\n", Note, lpvMessageBuffer);
	// Free the buffer allocated by the system
	LocalFree(lpvMessageBuffer);
}

bool WinMiniDump::DumpHeader()
{
	PMINIDUMP_HEADER header = (PMINIDUMP_HEADER)m_MapBuf;
	if (header == NULL) return false;
	printf("Version:%d.%d\n", header->Version & 0xffff, header->Version >> 16);
	printf("NumberOfStreams:%d\n", header->NumberOfStreams);
	printf("StreamDirectoryRva:%08X\n", header->StreamDirectoryRva);

#define DUMP_HEADER_FLAG(type) if ( (header->Flags) & (DWORD)type) printf("%s ",(char*)(#type)+8);

	printf("Flags:");
	DUMP_HEADER_FLAG(MiniDumpNormal);
	DUMP_HEADER_FLAG(MiniDumpWithDataSegs);
	DUMP_HEADER_FLAG(MiniDumpWithFullMemory);
	DUMP_HEADER_FLAG(MiniDumpWithHandleData);
	DUMP_HEADER_FLAG(MiniDumpFilterMemory);
	DUMP_HEADER_FLAG(MiniDumpScanMemory);
	DUMP_HEADER_FLAG(MiniDumpWithUnloadedModules);
	DUMP_HEADER_FLAG(MiniDumpWithIndirectlyReferencedMemory);
	DUMP_HEADER_FLAG(MiniDumpFilterModulePaths);
	DUMP_HEADER_FLAG(MiniDumpWithProcessThreadData);
	DUMP_HEADER_FLAG(MiniDumpWithPrivateReadWriteMemory);
	DUMP_HEADER_FLAG(MiniDumpWithoutOptionalData);
	DUMP_HEADER_FLAG(MiniDumpWithFullMemoryInfo);
	DUMP_HEADER_FLAG(MiniDumpWithThreadInfo);
	DUMP_HEADER_FLAG(MiniDumpWithCodeSegs);
	DUMP_HEADER_FLAG(MiniDumpWithoutAuxiliaryState);
	DUMP_HEADER_FLAG(MiniDumpWithFullAuxiliaryState);
	DUMP_HEADER_FLAG(MiniDumpWithPrivateWriteCopyMemory);
	DUMP_HEADER_FLAG(MiniDumpIgnoreInaccessibleMemory);
	DUMP_HEADER_FLAG(MiniDumpWithTokenInformation);
	DUMP_HEADER_FLAG(MiniDumpWithModuleHeaders);
	DUMP_HEADER_FLAG(MiniDumpFilterTriage);
	printf("\n");
	return true;
}

bool WinMiniDump::DumpDirectory()
{
	PMINIDUMP_HEADER header = (PMINIDUMP_HEADER)m_MapBuf;
	if (header == NULL) return false;
	PMINIDUMP_DIRECTORY dir = (PMINIDUMP_DIRECTORY)(m_MapBuf + header->StreamDirectoryRva);
	for (int i = 0; i < header->NumberOfStreams; i++,dir++)
	{
#define DUMP_STREAM_TYPE(type) if ( dir->StreamType == type) printf("%s\n",(char*)(#type));
		printf("StreamType:");
		switch (dir->StreamType)
		{
		case UnusedStream:
			DUMP_STREAM_TYPE(UnusedStream);
			break;
		case ReservedStream0:
			DUMP_STREAM_TYPE(ReservedStream0);
			break;
		case ReservedStream1:
			DUMP_STREAM_TYPE(ReservedStream1);
			break;
		case ThreadListStream:
			DUMP_STREAM_TYPE(ThreadListStream);
			DumpThreadList(dir);
			break;
		case ModuleListStream:
			DUMP_STREAM_TYPE(ModuleListStream);
			DumpModuleList(dir);
			break;
		case MemoryListStream:
			DUMP_STREAM_TYPE(MemoryListStream);
			DumpMemoryList(dir);
			break;
		case ExceptionStream:
			DUMP_STREAM_TYPE(ExceptionStream);
			break;
		case SystemInfoStream:
			DUMP_STREAM_TYPE(SystemInfoStream);
			break;
		case ThreadExListStream:
			DUMP_STREAM_TYPE(ThreadExListStream);
			break;
		case Memory64ListStream:
			DUMP_STREAM_TYPE(Memory64ListStream);
			DumpMemory64List(dir);
			break;
		case CommentStreamA:
			DUMP_STREAM_TYPE(CommentStreamA);
			break;
		case CommentStreamW:
			DUMP_STREAM_TYPE(CommentStreamW);
			break;
		case HandleDataStream:
			DUMP_STREAM_TYPE(HandleDataStream);
			break;
		case FunctionTableStream:
			DUMP_STREAM_TYPE(FunctionTableStream);
			break;
		case UnloadedModuleListStream:
			DUMP_STREAM_TYPE(UnloadedModuleListStream);
			break;
		case MiscInfoStream:
			DUMP_STREAM_TYPE(MiscInfoStream);
			break;
		case MemoryInfoListStream:
			DUMP_STREAM_TYPE(MemoryInfoListStream);
			DumpMemoryInfoList(dir);
			break;
		case ThreadInfoListStream:
			DUMP_STREAM_TYPE(ThreadInfoListStream);
			break;
		case HandleOperationListStream:
			DUMP_STREAM_TYPE(HandleOperationListStream);
			break;
		default:
			printf("Unknown(%d)\n", dir->StreamType);
			break;
		}
	}
	return true;
}

bool WinMiniDump::DumpThreadList(PMINIDUMP_DIRECTORY dir)
{
	PMINIDUMP_THREAD_LIST ThreadList = (PMINIDUMP_THREAD_LIST)(m_MapBuf + dir->Location.Rva);
	PMINIDUMP_THREAD thread = ThreadList->Threads;
	for (int i = 0; i < ThreadList->NumberOfThreads; i++,thread++)
	{
		printf("ThreadId:%d\n", thread->ThreadId);
		//ULONG32 SuspendCount;
		//ULONG32 PriorityClass;
		//ULONG32 Priority;
		//ULONG64 Teb;
		//MINIDUMP_MEMORY_DESCRIPTOR Stack;
		//MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
	}
	return true;
}

bool WinMiniDump::DumpModuleList(PMINIDUMP_DIRECTORY dir)
{
	PMINIDUMP_MODULE_LIST ModuleList = (PMINIDUMP_MODULE_LIST)(m_MapBuf + dir->Location.Rva);
	PMINIDUMP_MODULE module = ModuleList->Modules;
	for (int i = 0; i < ModuleList->NumberOfModules; i++, module++)
	{
		PMINIDUMP_STRING pname = (PMINIDUMP_STRING)(m_MapBuf + module->ModuleNameRva);
		printf("Module %08X %08X %ws\n", (UINT32)module->BaseOfImage, module->SizeOfImage, pname->Buffer);
	}
	return true;
}

bool WinMiniDump::DumpMemoryList(PMINIDUMP_DIRECTORY dir)
{
	PMINIDUMP_MEMORY_LIST MemList = (PMINIDUMP_MEMORY_LIST)(m_MapBuf + dir->Location.Rva);
	PMINIDUMP_MEMORY_DESCRIPTOR mem_desc = MemList->MemoryRanges;
	UINT64  mem_size = 0;
	for (int i = 0; i < MemList->NumberOfMemoryRanges; i++, mem_desc++)
	{
		MINIDUMP_LOCATION_DESCRIPTOR* memory = &mem_desc->Memory;
		UINT32 start = mem_desc->StartOfMemoryRange;
		UINT32 size = memory->DataSize;
		UINT32 end   = start + size;
		printf("%4d %08X-%08X %08X\n", i, start, end, size);
		mem_size += size;
	}
	printf("mem_size=%08X\n", mem_size);
	return true;
}

bool WinMiniDump::DumpMemory64List(PMINIDUMP_DIRECTORY dir)
{
	PMINIDUMP_MEMORY64_LIST Mem64List = (PMINIDUMP_MEMORY64_LIST)(m_MapBuf + dir->Location.Rva);
	PMINIDUMP_MEMORY_DESCRIPTOR64 mem_desc = Mem64List->MemoryRanges;
	UINT64  mem_size = 0;
	for (int i = 0; i < Mem64List->NumberOfMemoryRanges; i++, mem_desc++)
	{
		UINT64 start = mem_desc->StartOfMemoryRange;
		UINT64 size = mem_desc->DataSize;
		UINT64 end = start + size;
		printf("%4d %016llX-%016llX %016llX\n", i, start, end, size);
		mem_size += size;
	}
	printf("mem_size=%08X(%dMB)\n", (UINT32)mem_size, (UINT32)(mem_size>>20));
	return true;
}

bool WinMiniDump::DumpMemoryInfoList(PMINIDUMP_DIRECTORY dir)
{
	PMINIDUMP_MEMORY_INFO_LIST mem_list = (PMINIDUMP_MEMORY_INFO_LIST)(m_MapBuf + dir->Location.Rva);
	PMINIDUMP_MEMORY_INFO mem_info = (PMINIDUMP_MEMORY_INFO)(mem_list + 1);
	UINT64  mem_size = 0;
	int count = 0;
	for (int i = 0; i < mem_list->NumberOfEntries; i++, mem_info++)
	{
		if (!(mem_info->State & MEM_COMMIT)) continue;
		printf("%4d %08X-%08X %08X %08X %08X\n", count++,
			(UINT32)mem_info->BaseAddress,
			(UINT32)(mem_info->BaseAddress + mem_info->RegionSize),
			(UINT32)mem_info->RegionSize,
			(UINT32)mem_info->Type,
			(UINT32)mem_info->Protect);
		mem_size += mem_info->RegionSize;
	}
	printf("mem_size=%08X(%dMB)\n", (UINT32)mem_size, (UINT32)(mem_size >> 20));
	return true;
}
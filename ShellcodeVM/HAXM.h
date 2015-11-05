#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <winioctl.h>
#include "hax-interface.h"
#include "hax-windows.h"
#include "HAXM_VM.h"
#include <string>
#include <vector>
#include <list>
#include <map>

using namespace std;

typedef map<int, HAXM_VM*> HAXM_VM_MAP;

class HAXM
{
private:
	HANDLE		m_hDevice;
	HAXM_VM_MAP	m_vm_list;
public:

	HAXM()
	{
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	~HAXM()
	{
		close_device();
	}

	/*
	* return 0 upon success, -1 when the driver is not loaded,
	* other negative value for other failures
	*/
	bool open_device()
	{
		if (m_hDevice != INVALID_HANDLE_VALUE)
		{
			return true;
		}

		m_hDevice = CreateFile("\\\\.\\HAX",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (m_hDevice == INVALID_HANDLE_VALUE)
		{
			printf("Failed to open the HAX device!\n");
			//uint32_t errNum = GetLastError();
			//if (errNum == ERROR_FILE_NOT_FOUND) return -1;
			return false;
		}
		printf("open the HAX device OK\n");
		return true;
	}

	bool close_device()
	{
		if (m_hDevice != INVALID_HANDLE_VALUE)
		{
			HAXM_VM_MAP::iterator it = m_vm_list.begin();
			for (; it != m_vm_list.end(); it++)
			{
				delete it->second;
			}
			m_vm_list.clear();
			if (!CloseHandle(m_hDevice)) return false;
			m_hDevice = INVALID_HANDLE_VALUE;
		}

		return true;
	}

	//bool get_device_version(struct hax_module_version *version)
	bool is_supported()
	{
		// Current version
		static UINT32 HaxCurrentVersion = 0x2;

		// Minimum HAX kernel version
		static UINT32 HaxMinVersion = 0x1;

		if (m_hDevice == INVALID_HANDLE_VALUE)
		{
			printf("Invalid HANDLE for hax device!\n");
			return false;
		}

		int ret;
		DWORD dSize = 0;
		hax_module_version version;
		ret = DeviceIoControl(m_hDevice,
			HAX_IOCTL_VERSION,
			NULL, 0,
			&version, sizeof(version),
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			DWORD err = GetLastError();
			if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
			{
				printf("HAX module is too large.\n");
			}
			printf("Failed to get Hax module version:%d\n", err);
			return false;
		}
		printf("haxm compat_version:%08X cur_version=%08X\n",
			version.compat_version, version.cur_version);

		if ((HaxMinVersion > version.cur_version) || (HaxCurrentVersion < version.compat_version))
			return false;
		return  true;
	}

	//bool  get_capability()
	bool is_available()
	{
		if (m_hDevice == INVALID_HANDLE_VALUE)
		{
			printf("Invalid HANDLE for hax device!\n");
			return false;
		}

		int ret;
		DWORD dSize = 0;
		DWORD err = 0;
		hax_capabilityinfo cap;
		ret = DeviceIoControl(m_hDevice,
			HAX_IOCTL_CAPABILITY,
			NULL, 0,
			&cap, sizeof(cap),
			&dSize,
			(LPOVERLAPPED)NULL);

		if (!ret)
		{
			err = GetLastError();
			if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
			{
				printf("hax capability is too long to hold.\n");
			}
			printf("Failed to get Hax capability:%d\n", err);
			return false;
		}
		if (cap.wstatus & HAX_CAP_STATUS_WORKING)	printf("HAXM is working\n");
		else
		{
			printf("HAXM is not working possibly because VT/NX is disabled\n");
			if (cap.winfo & HAX_CAP_FAILREASON_VT) printf("HAXM is not working because VT is not enabeld\n");
			else if (cap.winfo & HAX_CAP_FAILREASON_NX)  printf("HAXM is not working because NX not enabled\n");
		}
		if (cap.wstatus & HAX_CAP_MEMQUOTA)
		{
			printf("HAXM has hard limit on how many RAM can be used as guest RAM\n");
			printf("mem_quota:%I64d\n", cap.mem_quota);
		}
		else printf("HAXM has no memory limitation\n");
		return true;
	}

	HAXM_VM* create_vm()
	{
		int ret;
		int vmid = 0;
		DWORD dSize = 0;

		ret = DeviceIoControl(
			m_hDevice,
			HAX_IOCTL_CREATE_VM,
			NULL, 0,
			&vmid, sizeof(vmid),
			&dSize,
			(LPOVERLAPPED) NULL);
		if (!ret) 
		{
			printf("error code:%d", GetLastError());
			return 0;
		}
		printf("vm_id=%08X\n", vmid);
		HAXM_VM* vm = new HAXM_VM(vmid);
		if (!vm->open_vm())
		{
			delete vm;
			return NULL;
		}
		m_vm_list[vmid] = vm;
		return vm;
	}

};


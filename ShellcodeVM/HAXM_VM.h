#pragma once
#include "HAXM_CPU.h"
typedef map<int, HAXM_CPU*> HAXM_CPU_MAP;
class HAXM_VM
{
private:
	UINT32 m_vmid;
	HANDLE m_hVM;
	HAXM_CPU_MAP m_cpu_list;
public:
	HAXM_VM(UINT32 vmid)
	{
		m_hVM = INVALID_HANDLE_VALUE;
		m_vmid = vmid;
	}

	~HAXM_VM()
	{
		close_vm();
	}

	bool open_vm()
	{
		char name[256];
		sprintf(name, "\\\\.\\hax_vm%02d", m_vmid);

		m_hVM = CreateFile(
			name,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (m_hVM == INVALID_HANDLE_VALUE)
		{
			printf("Open the vm devcie error:%s, ec:%d\n", name, GetLastError());
			return false;
		}
		return true;
	}

	bool close_vm()
	{
		if (m_hVM != INVALID_HANDLE_VALUE)
		{
			HAXM_CPU_MAP::iterator it = m_cpu_list.begin();
			for (; it != m_cpu_list.end(); it++)
			{
				delete it->second;
			}
			m_cpu_list.clear();
			if (!CloseHandle(m_hVM)) return false;
			m_hVM = INVALID_HANDLE_VALUE;
		}
		return true;
	}

	HAXM_CPU* create_cpu()
	{
		int ret;
		DWORD dSize = 0;
		int vcpuid = m_cpu_list.size();

		ret = DeviceIoControl(m_hVM,
			HAX_VM_IOCTL_VCPU_CREATE,
			&vcpuid, sizeof(vcpuid),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("Failed to create vcpu %x\n", vcpuid);
			return NULL;
		}
		HAXM_CPU * vcpu = new HAXM_CPU(m_vmid, vcpuid);
		if (!vcpu->open())
		{
			delete vcpu;
			return false;
		}
		m_cpu_list[vcpuid] = vcpu;
		return vcpu;
	}

	bool hax_populate_ram(uint64_t va, uint32_t size)
	{
		int ret;
		struct hax_alloc_ram_info info;
		DWORD dSize = 0;

		info.size = size;
		info.va = va;

		ret = DeviceIoControl(m_hVM,
			HAX_VM_IOCTL_ALLOC_RAM,
			&info, sizeof(info),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("Failed to allocate %x memory\n", size);
			return false;
		}

		return true;
	}

	bool hax_set_phys_mem(uint64_t start_addr, int size, uint64_t phys_offset)
	{
		struct hax_set_ram_info info, *pinfo = &info;
		//ram_addr_t flags = phys_offset & ~TARGET_PAGE_MASK;
		DWORD dSize = 0;
		int ret = 0;

		/* We look for the  RAM and ROM only */
		//if (flags >= IO_MEM_UNASSIGNED)	return 0;

		//if ((start_addr & ~TARGET_PAGE_MASK) || (size & ~TARGET_PAGE_MASK))
		//{
		//	dprint(
		//		"set_phys_mem %x %lx requires page aligned addr and size\n",
		//		start_addr, size);
		//	return -1;
		//}

		info.pa_start = start_addr;
		info.size = size;
		info.va = phys_offset;// (uint64_t)qemu_get_ram_ptr(phys_offset);
		info.flags = 0;// (flags & IO_MEM_ROM) ? 1 : 0;

		ret = DeviceIoControl(m_hVM,
			HAX_VM_IOCTL_SET_RAM,
			pinfo, sizeof(*pinfo),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("hax_set_phys_mem failed:%08X\n", GetLastError());
			return false;
		}
		return true;
	}

	bool notify_qemu_version()
	{
		int ret;
		DWORD dSize = 0;
		hax_qemu_version qemu_version;
		qemu_version.cur_version = 0x02;
		qemu_version.least_version = 0x01;

		ret = DeviceIoControl(m_hVM,
			HAX_VM_IOCTL_NOTIFY_QEMU_VERSION,
			&qemu_version, sizeof(qemu_version),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("Failed to notify qemu API version\n");
			return false;
		}
		return true;
	}

};


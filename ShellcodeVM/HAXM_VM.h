#pragma once
#include "HAXM_CPU.h"

#define   PAGE_SIZE							0x1000
#define   PDE_INDEX(virtual_addr)			((virtual_addr)>>22)
#define   PTE_INDEX(virtual_addr)			(((virtual_addr)>>12)&0x3FF)
#define   PAGE_INDEX(addr)					(addr>>12)
#define   PAGE_ADDRESS(page_index)			(((UINT32)page_index)<<12)
#define   PDE_ADDRESS(pde)					((pde)&0xFFFFF000)
#define   PDE_ATTR(pde)						((pde)&0x00000FFF)
#define   PTE_ADDRESS(pte)					((pte)&0xFFFFF000)
#define   PTE_ATTR(pte)						((pte)&0x00000FFF)


#define PAGES_TO_SIZE(pages)	 ((uint32)(pages)<<12)
#define SIZE_TO_PAGES(size)      (((uint32)(size) + PAGE_SIZE -1)>>12)

#define		PT_PRESENT   0x001
#define		PT_WRITABLE  0x002
#define		PT_USER      0x004
#define		PT_ACCESSED  0x020
#define		PT_DIRTY     0x040
#define		PT_4M		 0x080

#define  FRAME_DATABASE_GPA		0x00000000
#define  FRAME_DATABASE_GVA		0x80000000
#define  FRAME_DATABASE_SIZE	0x00100000
#define  PAGE_DIR_BASE			0xC0300000
#define  PAGE_TABLE_BASE		0xC0000000

#define  PAGE_FREE				0
#define  PAGE_USED				1

/*
0x00000000	+-----------------------+ 0x80000000
			|   frame_database(1M)  |
0x00100000	+-----------------------+
			|                       |
			+-----------------------+
			|                       |
			+-----------------------+ 0xC0000000  
			|page_dir/page_table(4M)|
			+-----------------------+ 0xC0400000
			|                       |
			+-----------------------+

*/
typedef map<int, HAXM_CPU*> HAXM_CPU_MAP;

struct  VM_MEM_BLOCK
{
	UINT64 host_virtual_address;
	UINT64 guest_physical_addrress;
	UINT32 size;
};

class HAXM_VM
{
private:
	UINT32 m_vmid;
	HANDLE m_hVM;
	HAXM_CPU_MAP m_cpu_list;
	map<UINT64, VM_MEM_BLOCK>m_mem_blocks;
	UINT64 m_page_dir_physical_addr;
	UINT64 m_page_dir_virtual_addr;
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
			return NULL;
		}
		m_cpu_list[vcpuid] = vcpu;
		return vcpu;
	}

	bool hax_populate_ram(uint64_t host_virtual_addr, uint32_t size)
	{
		int ret;
		struct hax_alloc_ram_info info;
		DWORD dSize = 0;

		info.size = size;
		info.va = host_virtual_addr;

		map<UINT64, VM_MEM_BLOCK>::iterator it = m_mem_blocks.find(host_virtual_addr);
		if (it != m_mem_blocks.end())
		{
			printf("Memory block 0x%08X--0x%08X has populated!\n", (UINT32)host_virtual_addr, (UINT32)host_virtual_addr + size);
			return false;
		}

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
		
		VM_MEM_BLOCK block = { host_virtual_addr, 0, size };
		m_mem_blocks[host_virtual_addr] = block;

		return true;
	}

	bool hax_set_phys_mem(uint64_t guest_physical_addr, int size, uint64_t host_virtual_addr)
	{
		struct hax_set_ram_info info, *pinfo = &info;
		DWORD dSize = 0;
		int ret = 0;

		map<UINT64, VM_MEM_BLOCK>::iterator it = m_mem_blocks.find(host_virtual_addr);
		if (it == m_mem_blocks.end())
		{
			printf("Memory block 0x%08X--0x%08X has not populated!\n", (UINT32)host_virtual_addr, (UINT32)host_virtual_addr + size);
			return false;
		}
		if (it->second.guest_physical_addrress != 0)
		{
			printf("Memory block 0x%08X--0x%08X has set!\n", (UINT32)host_virtual_addr, (UINT32)host_virtual_addr + size);
			return false;
		}

		info.pa_start = guest_physical_addr;
		info.size = size;
		info.va = host_virtual_addr;
		info.flags = 0;

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
		it->second.guest_physical_addrress = guest_physical_addr;
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

	void   set_page_dir(UINT64 page_dir_physical_addr, UINT64 page_dir_virtual_addr)
	{
		m_page_dir_physical_addr = page_dir_physical_addr;
		m_page_dir_virtual_addr = page_dir_virtual_addr;

		//页目录自映射
		UINT32 *page_dir_hva = (UINT32 *)gpa_to_hva(page_dir_physical_addr);
		page_dir_hva[PDE_INDEX(m_page_dir_virtual_addr)] = (uint32)m_page_dir_physical_addr | PT_PRESENT | PT_WRITABLE;

	}

	bool map_pages(UINT64 guest_physical_addr, UINT64 guest_virtual_addr, int pages)
	{
		UINT32 *page_dir_hva = (UINT32 *)gpa_to_hva(m_page_dir_physical_addr);
		for (int i = 0; i < pages; i++)
		{
			int pde_index = PDE_INDEX(guest_virtual_addr);
			UINT32 pde = page_dir_hva[pde_index];
			UINT32 pt_gpa = PDE_ADDRESS(pde);
			UINT32* pt_hva = NULL;
			if (pt_gpa != 0)
			{
				pt_hva = (UINT32*)gpa_to_hva(pt_gpa);
			}
			else
			{
				pt_gpa = alloc_physical_pages(1);
				if (pt_gpa == 0) return false;
				pt_hva = (UINT32*)gpa_to_hva(pt_gpa);
				memset((void*)pt_hva, 0, PAGE_SIZE);
				page_dir_hva[pde_index] = pt_gpa | PT_PRESENT | PT_WRITABLE;
			}
			int pte_index = PTE_INDEX(guest_virtual_addr);
			pt_hva[pte_index] = guest_physical_addr | PT_PRESENT | PT_WRITABLE;
			guest_physical_addr += PAGE_SIZE;
			guest_virtual_addr += PAGE_SIZE;
		}
	}

	bool unmap_pages(UINT64 guest_virtual_addr, int pages)
	{
		UINT32 *page_dir_hva = (UINT32 *)gpa_to_hva(m_page_dir_physical_addr);
		for (int i = 0; i < pages; i++)
		{
			int pde_index = PDE_INDEX(guest_virtual_addr);
			UINT32 pde = page_dir_hva[pde_index];
			UINT32 pt_gpa = PDE_ADDRESS(pde);
			if (pt_gpa == 0)
			{
				return false;
			}
			UINT32*  pt_hva = (UINT32*)gpa_to_hva(pt_gpa);
			int pte_index = PTE_INDEX(guest_virtual_addr);
			UINT32 pte = pt_hva[pte_index];
			UINT32 pte_gpa = PTE_ADDRESS(pte);
			free_physical_pages(pte_gpa, 1);
			pt_hva[pte_index] = 0;
			guest_virtual_addr += PAGE_SIZE;
		}
	}

	UINT64 alloc_physical_pages(UINT64 guest_physical_addr, int pages)
	{
		BYTE* frame_db_hva = (BYTE *)gpa_to_hva(FRAME_DATABASE_GPA);
		int   page_index = PAGE_INDEX(guest_physical_addr);
		for (int i = 0; i < pages; i++)
		{
			if (frame_db_hva[page_index + i] != PAGE_FREE) return 0;//其中某物理页面已被分配
		}
		memset(frame_db_hva + page_index, PAGE_USED, pages);
		return guest_physical_addr;
	}

	bool free_physical_pages(UINT64 guest_physical_addr, int pages)
	{
		BYTE* frame_db_hva = (BYTE *)gpa_to_hva(FRAME_DATABASE_GPA);
		int   page_index = PAGE_INDEX(guest_physical_addr);
		//memset(frame_db_hva + page_index, PAGE_FREE, pages);
		for (int i = 0; i < pages; i++, page_index++)
		{
			frame_db_hva[page_index] = PAGE_FREE;
		}
		return true;
	}

	UINT64 alloc_physical_pages(int pages)
	{
		BYTE* frame_db_hva = (BYTE *)gpa_to_hva(FRAME_DATABASE_GPA);
		for (int i = 0; i < FRAME_DATABASE_SIZE; i++)
		{
			if (frame_db_hva[i] == PAGE_FREE)
			{
				int j = 1;
				for (; j < pages; j++)
				{
					if (frame_db_hva[i + j] != PAGE_FREE) break;
				}
				if (j == pages)
				{
					memset(frame_db_hva + i, PAGE_USED, pages);
					return PAGE_ADDRESS(i);
				}
				i = j;//跳过中间空白页
			}
		}
		return 0;
	}

	UINT64 gpa_to_hva(UINT64 guest_physical_addr)
	{
		map<UINT64, VM_MEM_BLOCK>::iterator it = m_mem_blocks.begin();
		for (; it != m_mem_blocks.end(); it++)
		{
			UINT64 start_addr = it->second.guest_physical_addrress;
			UINT64 end_addr = it->second.guest_physical_addrress + it->second.size;
			if (guest_physical_addr >= start_addr && guest_physical_addr < end_addr)
			{
				UINT64 offset = guest_physical_addr - it->second.guest_physical_addrress;
				UINT64 hva = it->second.host_virtual_address + offset;
				return hva;
			}
		}
		return 0;
	}

	UINT64 hva_to_gpa(UINT64 host_virtual_addr)
	{
		map<UINT64, VM_MEM_BLOCK>::iterator it = m_mem_blocks.begin();
		for (; it != m_mem_blocks.end(); it++)
		{
			UINT64 start_addr = it->second.host_virtual_address;
			UINT64 end_addr = it->second.host_virtual_address + it->second.size;
			if (host_virtual_addr >= start_addr && host_virtual_addr < end_addr)
			{
				UINT64 offset = host_virtual_addr - it->second.host_virtual_address;
				UINT64 gpa = it->second.guest_physical_addrress + offset;
				return gpa;
			}
		}
		return 0;
	}

};


#pragma once
#include <stdint.h>

#define   PAGE_SIZE						0x1000
#define   PAGE_SIZE_BITS					12
#define   PD_INDEX(virtual_addr)			((virtual_addr>>22))
#define   PT_INDEX(virtual_addr)			((virtual_addr>>12)&0x3FF)

#define   KERNEL_BASE						0x80000000
#define   PAGE_DIR_BASE					m_page_dir //host va
#define   PAGE_TABLE_BASE					m_page_table_base //host va

#define     GET_PDE(addr)  ((uint32_t*)PAGE_DIR_BASE)[(uint32_t)(addr)>>22]
#define     GET_PTE(addr)  ((uint32_t*)PAGE_TABLE_BASE)[(uint32_t)(addr)>>12]
#define     GET_PAGE_TABLE(addr)  (PAGE_TABLE_BASE + ((uint32_t)(addr)>>22)* PAGE_SIZE)

#define     SET_PDE(addr, val)  ((uint32_t*)PAGE_DIR_BASE)[(uint32_t)(addr)>>22] = val;
#define     SET_PTE(addr, val)  ((uint32_t*)PAGE_TABLE_BASE)[(uint32_t)(addr)>>12]=val;

#define     PAGE_ALGINED(addr)		((((uint32_t)addr) & 0x00000FFF) ==0)  	
#define     CHECK_PAGE_ALGINED(addr)  if ((((uint32_t)addr) & 0x00000FFF) !=0) panic("CHECK_PAGE_ALGINED(%08X)",addr); 	

#define		PT_PRESENT   0x001

#define		PT_WRITABLE  0x002
#define		PT_READONLY  0x000

#define		PT_USER      0x004
#define		PT_KERNEL    0x000

#define		PT_ACCESSED  0x020
#define		PT_DIRTY     0x040

#define KB(x)                    ((uint64_t)x<<10)
#define MB(x)                    ((uint64_t)x<<20)
#define GB(x)                    ((uint64_t)x<<30)

#define USER_SPACE(addr)       (((uint32_t)addr) < KERNEL_BASE)
#define KERNEL_SPACE(addr)     (((uint32_t)addr) >= KERNEL_BASE)

#define PAGES_TO_SIZE(pages)	 ((uint64_t)(pages)<<12)
#define SIZE_TO_PAGES(size)      (((uint64_t)(size) + PAGE_SIZE -1)>>12)
#define GPA_TO_HVA(gpa)        (m_host_mem + gpa)

#define     CHECK_PAGE_ALGINED(addr)  if ((((uint64_t)addr) & 0x00000FFF) !=0) printf("CHECK_PAGE_ALGINED(%08X)",addr); 	

class page_frame_database;
class pager
{
private:
	uint64_t m_page_dir;
	uint64_t m_host_mem;
	uint64_t m_page_table_base;
	page_frame_database* m_page_frame_db;
public:
	pager();
	~pager();
public:
	bool     Init(page_frame_database* page_frame_db, uint64_t host_mem, uint64_t page_table_base);
	uint64_t new_page_dir();
	uint64_t new_page_table(uint64_t virtual_address);
	uint64_t map_pages(uint64_t physical_address, uint64_t virtual_address, uint64_t size, uint64_t protect = (PT_PRESENT | PT_WRITABLE));
	void unmap_pages(uint64_t virtual_address, uint64_t size);
};


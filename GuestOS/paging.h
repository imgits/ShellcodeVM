#pragma once
#include <stdint.h>

#define   PAGE_SIZE							0x1000
#define   PAGE_SIZE_BITS					12
#define   PDE_INDEX(virtual_addr)			((virtual_addr>>22))
#define   PTE_INDEX(virtual_addr)			((virtual_addr>>12)&0x3FF)

#define   KERNEL_BASE						0x80000000
#define   PAGE_DIR_BASE						0xC0300000
#define   PAGE_TABLE_BASE					0xC0000000
#define   PAGE_FRAME_BASE					0xC0400000

#define     GET_PDE(addr)  ((uint32_t*)PAGE_DIR_BASE)[(uint32_t)(addr)>>22]
#define     GET_PTE(addr)  ((uint32_t*)PAGE_TABLE_BASE)[(uint32_t)(addr)>>12]
#define     GET_PAGE_TABLE(addr)  (PAGE_TABLE_BASE + ((uint32_t)(addr)>>22)* PAGE_SIZE)

#define     SET_PDE(addr, val)  ((uint32_t*)PAGE_DIR_BASE)[(uint32_t)(addr)>>22] = val;
#define     SET_PTE(addr, val)  ((uint32_t*)PAGE_TABLE_BASE)[(uint32_t)(addr)>>12]=val;

#define     PAGE_ALGINED(addr)		((((uint32_t)addr) & 0x00000FFF) ==0)  	
#define     CHECK_PAGE_ALGINED(addr)  //if ((((uint32_t)addr) & 0x00000FFF) !=0) panic("CHECK_PAGE_ALGINED(%08X)",addr); 	

#define		PT_PRESENT   0x001

#define		PT_WRITABLE  0x002
#define		PT_READONLY  0x000

#define		PT_USER      0x004
#define		PT_KERNEL    0x000

#define		PT_ACCESSED  0x020
#define		PT_DIRTY     0x040

#define   PAGE_FREE					0x00
#define   PAGE_USED					0x01
#define   PAGE_RESERVED				0xff


#define KB(x)                    ((uint32_t)x<<10)
#define MB(x)                    ((uint32_t)x<<20)
#define GB(x)                    ((uint32_t)x<<30)

#define USER_SPACE(addr)       (((uint32_t)addr) < KERNEL_BASE)
#define KERNEL_SPACE(addr)     (((uint32_t)addr) >= KERNEL_BASE)

#define PAGES_TO_SIZE(pages)	 ((uint32_t)(pages)<<12)
#define SIZE_TO_PAGES(size)      (((uint32_t)(size) + PAGE_SIZE -1)>>12)
#define PAGE_ALGIN_SIZE(size)	((uint32_t)(size + 4095)&0xFFFFF000)

class PAGER
{
private:
	uint32_t		m_page_dir;
	uint32_t		m_page_table_base;

	uint32_t		m_page_frame_min;
	uint32_t		m_page_frame_max;
	uint32_t		m_next_free_page_frame;
	uint8_t*		m_page_frame_database;
	bool			m_database_usable;

public:
	PAGER();
	~PAGER();
public:
	bool		Init(uint32_t page_frame_min, uint32_t page_frame_max);

	void		map_pages(uint32_t physical_address, uint32_t virtual_address, uint32_t size, uint32_t protect = (PT_PRESENT | PT_WRITABLE));
	void		unmap_pages(uint32_t virtual_address, uint32_t size);

	bool        create_database();
	uint32_t	alloc_physical_page();
	uint32_t	alloc_physical_pages(uint32_t pages);
	void		free_physical_page(uint32_t page);
	void		free_physical_pages(uint32_t  start_page, uint32_t pages);

private:
	void     startup_page_mode();
	uint32_t new_page_dir();
	uint32_t new_page_table(uint32_t virtual_address);
	bool     identity_paging_lowest_4M();
};


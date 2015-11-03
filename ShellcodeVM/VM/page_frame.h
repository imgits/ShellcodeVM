#pragma once
#include <stdint.h>

#define   PAGE_FRAME_FREE					0
#define   PAGE_FRAME_USED					1

class pager;
class page_frame_database
{
private:
	pager*   m_pager;
	uint64_t m_mem_size;
	uint64_t m_page_frame_min;
	uint64_t m_page_frame_max;
	uint64_t m_next_free_page_frame;
	uint8_t*    m_page_frame_map;
public:
	page_frame_database();
	~page_frame_database();
	bool Init(pager*  pager, uint64_t mem_size);
	uint64_t	alloc_physical_page();
	uint64_t	alloc_physical_pages(uint64_t pages);
	void		free_physical_page(uint64_t page);
	void		free_physical_pages(uint64_t  start_page, uint64_t pages);
};


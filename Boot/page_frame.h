#pragma once
#include <stdint.h>

#define   PAGE_FRAME_FREE					0
#define   PAGE_FRAME_USED					1
#define   PAGE_FRAME_BASE					0xC0400000

class PAGER;
class PAGE_FRAME_DB
{
private:
	PAGER*   m_pager;
	uint32_t m_page_frame_min;
	uint32_t m_page_frame_max;
	uint32_t m_next_free_page_frame;
	uint8_t* m_page_frame_database;
	bool     m_database_usable;
public:
	PAGE_FRAME_DB();
	~PAGE_FRAME_DB();
	bool		Init(PAGER*  pager, uint32_t page_frame_min, uint32_t page_frame_max);
	bool        create_database();
	uint32_t	alloc_physical_page();
	uint32_t	alloc_physical_pages(uint32_t pages);
	void		free_physical_page(uint32_t page);
	void		free_physical_pages(uint32_t  start_page, uint32_t pages);
};


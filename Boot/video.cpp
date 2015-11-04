#include "VIDEO.h"

VIDEO::VIDEO()
{
	m_video_buf=0;
	m_video_buf_size = 0;
	m_screen_width	 = 25;
	m_screen_height  = 80;
	m_cursor_x = 0;
	m_cursor_y = 0;
}

VIDEO::~VIDEO()
{
}

bool VIDEO::Init(uint32_t video_buf, uint32_t buf_size, uint32_t width, uint32_t height)
{
	m_video_buf = video_buf;
	m_video_buf_size = buf_size;
	m_screen_width = width <= MAX_SCREEN_WIDTH ? width-1 : MAX_SCREEN_WIDTH;
	m_screen_height = height<= MAX_SCREEN_HEIGHT? height-1: MAX_SCREEN_HEIGHT;
	m_cursor_x = 0;
	m_cursor_y = 0;
	return true;
}

void VIDEO::putc(char ch)
{
	if (ch == '\r')
	{
		m_cursor_x = 0;
		return;
	}
	if (ch == '\n')
	{
		if (m_cursor_y < m_screen_height) m_cursor_y++;
		else scroll(1);
		return;
	}
	if (m_screen_width < m_screen_width)

}

void VIDEO::puts(char* str)
{

}

void VIDEO::clear()
{

}

void VIDEO::scroll(int lines)
{

}

void VIDEO::gotoxy(int x, int y)
{

}

int  VIDEO::getx()
{

}

int  VIDEO::gety()
{

}


#include "VIDEO.h"
#include <string.h>

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
	if (m_cursor_x >= m_screen_width) 
	{
		m_cursor_x = 0;
		if (m_cursor_y < m_screen_height) m_cursor_y++;
		else scroll(1);
	}
	char* addr = (char*)(m_video_buf + m_cursor_y*LINE_BUF_WIDTH + m_cursor_x++);
	*addr++ = ch;
	*addr = 0;
}

void VIDEO::puts(char* str)
{
	int len = strlen(str);
	for (int i = 0; i < len; i++) putc(str[i]);
}


void VIDEO::clear()
{
	char* line_buf = (char*)m_video_buf;
	for (int i = 0; i <= m_screen_height;i++)
	{
		line_buf[0] = 0;
		line_buf += LINE_BUF_WIDTH;
	}
}

void VIDEO::scroll(int lines)
{
	char* line_buf = (char*)m_video_buf;
	memcpy(line_buf, line_buf + lines*LINE_BUF_WIDTH, (m_screen_height - lines)*LINE_BUF_WIDTH);
}

void VIDEO::gotoxy(int x, int y)
{
	m_cursor_x = x; 
	m_cursor_y = y;
}

int  VIDEO::getx()
{
	return m_cursor_x;
}

int  VIDEO::gety()
{
	return m_cursor_y;
}


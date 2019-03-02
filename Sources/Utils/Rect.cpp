#include "Rect.hpp"

using namespace LWGC;

Rect::Rect(void) : _size(glm::vec2(0, 0)), _offset(glm::vec2(0, 0))
{
}

Rect::Rect(int w, int h, int x, int y) : _size(glm::vec2(w, h)), _offset(glm::vec2(x, y))
{
}

Rect &	Rect::operator=(const Rect & rhs)
{
	if (this != &rhs)
	{
		_size = rhs._size;
		_offset = rhs._offset;
	}
}

float		Rect::GetMaxX(void) const { return _offset.x + _size.x; }
float		Rect::GetMaxY(void) const { return _offset.y + _size.y; }
float		Rect::GetMinX(void) const { return _offset.x; }
float		Rect::GetMinY(void) const { return _offset.y; }

glm::vec2	Rect::GetSize() { return _size; }
glm::vec2	Rect::GetOffset() { return _offset; }

bool	Rect::CanFit(const Rect & rect, int w, int h)
{
	if (_offset.x + rect._size.x <= w && _offset.y + rect._size.y <= h)
		return true;
	return false;
}
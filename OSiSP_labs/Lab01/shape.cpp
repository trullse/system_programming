#include <windows.h>

class Shape {
public:
	enum ShapeType { RECTANGLE, CIRCLE, };

	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	ShapeType shapeType = RECTANGLE;
	COLORREF color = RGB(192, 0, 0);

	Shape() {}

	Shape(ShapeType _shapeType)
	{
		shapeType = _shapeType;
	}

	void checkCoord()
	{
		if (this->x1 > this->x2)
		{
			int buff = this->x1;
			this->x1 = this->x2;
			this->x2 = buff;
		}
		if (this->y1 > this->y2)
		{
			int buff = this->y1;
			this->y1 = this->y2;
			this->y2 = buff;
		}
	}

	void moveFigure(int dx, int dy)
	{
		x1 += dx;
		x2 += dx;
		y1 += dy;
		y2 += dy;
	}
};
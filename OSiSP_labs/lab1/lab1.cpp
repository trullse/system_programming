#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <tchar.h>
#include <vector>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")	// updated styles

RECT rc = { 0 };
int WindowPosX = 0;
int WindowPosY = 0;

int lastShapeId = 0;

class Shape {
public:
	int shapeId;
	int x1, y1, x2, y2;
	int shapeType;	// 0 - rectangle, 1 - circle
	COLORREF color;

	Shape() {}

	Shape(int _shapeType)
	{
		shapeId = ++lastShapeId;
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

int choosenShapeType = 0;
std::vector<Shape*> shapes;
Shape* currentShape = nullptr;
bool isDrawing = false; // while LMB clicked
bool isEditing = false;
// Editing staff
int selectedShapeIndex = -1;
bool isCornerSelected = false;
bool isLtCornerSelected = false;
int deltaWay = 20;

void DrawShape(HDC hdc, int shapeType, int x1, int y1, int x2, int y2)
{
	if (shapeType == 0)
		Rectangle(hdc, x1, y1, x2, y2);
	else
		Ellipse(hdc, x1, y1, x2, y2);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW; // Window redraw after sizing

	RegisterClass(&wc);

	// Create the window.

	HWND hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"Lab 1",                       // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
	{
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);

	// Run the message loop.

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		shapes.clear();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		//TCHAR szText[256] = { 0 };
		//_stprintf_s(szText, TEXT("Window Pos: %i, %i"), WindowPosX, WindowPosY);

		//DrawText(hdc, szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		for (int i = 0; i < shapes.size(); i++)
		{
			Shape shape = *shapes[i];
			DrawShape(hdc, shape.shapeType, shape.x1, shape.y1, shape.x2, shape.y2);
		}
		if (currentShape != nullptr)
			DrawShape(hdc, currentShape->shapeType, currentShape->x1, currentShape->y1, currentShape->x2, currentShape->y2);

		EndPaint(hwnd, &ps);
	}
	return 0;

	case WM_LBUTTONDOWN:
		if (!isEditing)
		{
			isDrawing = true;
			currentShape = new Shape(choosenShapeType);
			currentShape->x1 = currentShape->x2 = LOWORD(lParam);
			currentShape->y1 = currentShape->y2 = HIWORD(lParam);
		}
		else if (isEditing && !isCornerSelected)
		{
			if (selectedShapeIndex == -1)
			{
				for (int i = shapes.size() - 1; i >= 0; i--)
				{
					Shape shape = *shapes[i];
					int x = LOWORD(lParam);
					int y = HIWORD(lParam);
					if (shape.x1 <= x && shape.x2 >= x && shape.y1 <= y && shape.y2 >= y)
					{
						selectedShapeIndex = i;
						break;
					}
				}
				/*if (selectedShapeIndex != -1)
				{
					shapes[selectedShapeIndex].color = 0x000000FF;
					InvalidateRect(hwnd, 0, TRUE);
				}*/
			}
			else
			{
				Shape shape = *shapes[selectedShapeIndex];
				int x1 = shape.x1;
				int y1 = shape.y1;
				int x2 = shape.x2;
				int y2 = shape.y2;
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				double lt_corner_way = sqrt(pow((x1 - x), 2) + pow((y1 - y), 2));
				double rd_corner_way = sqrt(pow((x2 - x), 2) + pow((y2 - y), 2));
				isCornerSelected = true;
				if (lt_corner_way < rd_corner_way)
					isLtCornerSelected = true;
				else
					isLtCornerSelected = false;
			}
		}
		else // corner selected
		{
			if (isLtCornerSelected)
			{
				shapes[selectedShapeIndex]->x1 = LOWORD(lParam);
				shapes[selectedShapeIndex]->y1 = HIWORD(lParam);
			}
			else
			{
				shapes[selectedShapeIndex]->x2 = LOWORD(lParam);
				shapes[selectedShapeIndex]->y2 = HIWORD(lParam);
			}
			shapes[selectedShapeIndex]->checkCoord();
			isCornerSelected = false;
			selectedShapeIndex = -1;
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;

	case WM_MOUSEMOVE:
		if (isDrawing)
		{
			currentShape->x2 = LOWORD(lParam);
			currentShape->y2 = HIWORD(lParam);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;

	case WM_LBUTTONUP:
		if (isDrawing)
		{
			isDrawing = false;
			currentShape->checkCoord();
			shapes.push_back(currentShape);
		}

	case WM_SIZE:
		rc.right = LOWORD(lParam);
		rc.bottom = HIWORD(lParam);
		break;

	case WM_MOVE:
		WindowPosX = (int)(short)LOWORD(lParam);   // horizontal position 
		WindowPosY = (int)(short)HIWORD(lParam);   // vertical position
		InvalidateRect(hwnd, 0, TRUE);				// update window after moving
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostMessage(hwnd, WM_DESTROY, 0, 0);
		else if (wParam == VK_SPACE)
		{
			if (choosenShapeType == 0)
				choosenShapeType = 1;
			else
				choosenShapeType = 0;
		}
		else if (wParam == VK_SHIFT)
		{
			if (!isEditing)
			{
				if (isDrawing)
				{
					isDrawing = false;
					shapes.push_back(currentShape);
				}
				isEditing = true;
			}
			else
			{
				isEditing = false;
				selectedShapeIndex = -1;
				isCornerSelected = false;
			}
		}
		else if (wParam == VK_LEFT)
		{
			if (isEditing && selectedShapeIndex != -1)
				shapes[selectedShapeIndex]->moveFigure(-deltaWay, 0);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_RIGHT)
		{
			if (isEditing && selectedShapeIndex != -1)
				shapes[selectedShapeIndex]->moveFigure(deltaWay, 0);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_UP)
		{
			if (isEditing && selectedShapeIndex != -1)
				shapes[selectedShapeIndex]->moveFigure(0, -deltaWay);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_DOWN)
		{
			if (isEditing && selectedShapeIndex != -1)
				shapes[selectedShapeIndex]->moveFigure(0, deltaWay);
			InvalidateRect(hwnd, 0, TRUE);
		}
		break;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
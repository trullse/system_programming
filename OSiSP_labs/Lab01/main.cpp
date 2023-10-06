#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <tchar.h>
#include <vector>
#include "resource.h"
#include "shape.cpp"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")	// updated styles

RECT rc = { 0 };
int WindowPosX = 0;
int WindowPosY = 0;

Shape::ShapeType choosenShapeType = Shape::RECTANGLE;
std::vector<Shape*> shapes;
Shape* currentShape = nullptr;
bool isDrawing = false; // while LMB clicked
bool isEditing = false;
// Editing staff
int selectedShapeIndex = -1;
bool isCornerSelected = false;
bool isLtCornerSelected = false;
int deltaWay = 20;
HBRUSH brush = CreateSolidBrush(RGB(192, 0, 0));
HBRUSH original = CreateSolidBrush(RGB(192, 50, 0));

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { };

	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW; // Window redraw after sizing
	wc.hIcon = hIcon;

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

// Draw help functions

void DrawShape(HDC hdc, HBRUSH hbrush, int shapeType, int x1, int y1, int x2, int y2)
{
	SelectObject(hdc, hbrush);
	if (shapeType == Shape::RECTANGLE)
	{
		Rectangle(hdc, x1, y1, x2, y2);
	}
	else if (shapeType == Shape::CIRCLE)
	{
		Ellipse(hdc, x1, y1, x2, y2);
	}
}

void StartDraw(LPARAM lParam)
{
	isDrawing = true;
	currentShape = new Shape(choosenShapeType);
	currentShape->x1 = currentShape->x2 = LOWORD(lParam);
	currentShape->y1 = currentShape->y2 = HIWORD(lParam);
}

void SelectShape(LPARAM lParam)
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
}

void SelectCorner(LPARAM lParam)
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

void ChangeCorner(LPARAM lParam, HWND hwnd)
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

void ShapeDrawingCoord(LPARAM lParam, HWND hwnd)
{
	currentShape->x2 = LOWORD(lParam);
	currentShape->y2 = HIWORD(lParam);
	InvalidateRect(hwnd, NULL, TRUE);
}

void EndDrawing()
{
	isDrawing = false;
	currentShape->checkCoord();
	shapes.push_back(currentShape);
}

// Procedure

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

		for (int i = 0; i < shapes.size(); i++)
		{
			Shape shape = *shapes[i];
			DrawShape(hdc, brush, shape.shapeType, shape.x1, shape.y1, shape.x2, shape.y2);
		}
		if (currentShape != nullptr)
			DrawShape(hdc, brush, currentShape->shapeType, currentShape->x1, currentShape->y1, currentShape->x2, currentShape->y2);

		EndPaint(hwnd, &ps);
	}
	return 0;

	case WM_LBUTTONDOWN:
		if (!isEditing)
		{
			StartDraw(lParam);
		}
		else if (isEditing && !isCornerSelected)
		{
			if (selectedShapeIndex == -1)
			{
				SelectShape(lParam);
			}
			else
			{
				SelectCorner(lParam);
			}
		}
		else // corner selected
		{
			ChangeCorner(lParam, hwnd);
		}
		break;

	case WM_MOUSEMOVE:
		if (isDrawing)
		{
			ShapeDrawingCoord(lParam, hwnd);
		}
		break;

	case WM_LBUTTONUP:
		if (isDrawing)
		{
			EndDrawing();
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
			if (choosenShapeType == Shape::RECTANGLE)
				choosenShapeType = Shape::CIRCLE;
			else
				choosenShapeType = Shape::RECTANGLE;
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
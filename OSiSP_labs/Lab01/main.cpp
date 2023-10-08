#ifndef UNICODE
#define UNICODE
#endif 

#define ID_BUTTON 1001
#define ID_LISTBOX 1002
#define ID_LAYER_BUTTON 1003

#include <windows.h>
#include <tchar.h>
#include <vector>
#include "resource.h"
#include "shape.cpp"
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")	// updated styles

RECT rc = { 0 };
int WindowPosX = 0;
int WindowPosY = 0;

HWND hwndChild;
HWND hwndListBox;
HWND AddLayerButton;

HDC hdc;
HDC hdcBuffer;
HBITMAP hBitmap;
HGDIOBJ oldBitmap;
Graphics* graphics;

int toolsWidth = 150;
int margin = 10;
int drawWindowX = toolsWidth;
int drawWindowY = margin;
int drawWindowWidth;
int drawWindowHeight;
int buttonHeight = 30;
int listHeight = 150;

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
COLORREF dColors[16];
COLORREF currentColor = RGB(255, 0, 0);
int layersCount = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK DrawWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// buffer functions

void CreateBuffer(int width, int height) {
	hdcBuffer = CreateCompatibleDC(NULL);
	hBitmap = CreateCompatibleBitmap(hdcBuffer, width, height);
	oldBitmap = SelectObject(hdcBuffer, hBitmap);
}

void DeleteBuffer() {
	SelectObject(hdcBuffer, oldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hdcBuffer);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { };
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW; // Window redraw after sizing
	wc.hIcon = hIcon;

	RegisterClass(&wc);

	WNDCLASS DrawWndClass = { 0 };
	DrawWndClass.lpfnWndProc = DrawWindowProc;
	DrawWndClass.hInstance = hInstance;
	DrawWndClass.lpszClassName = L"DrawWindowClass";

	RegisterClass(&DrawWndClass);


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

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	drawWindowHeight = clientRect.bottom - clientRect.top - 2 * margin;
	drawWindowWidth = clientRect.right - clientRect.left - toolsWidth - margin;

	hwndChild = CreateWindowEx(0, L"DrawWindowClass", L"�������� ����", WS_CHILD | WS_VISIBLE | WS_BORDER,
		drawWindowX, drawWindowY, drawWindowWidth, drawWindowHeight, hwnd, (HMENU)NULL, hInstance, NULL);

	if (hwndChild == NULL) 
	{
		return 0;
	}

	HWND hwndButton = CreateWindow(
		L"BUTTON",             // ��� ������ ��� ������
		L"Choose color",  // ����� �� ������
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // ����� ������
		margin, margin, toolsWidth - 2 * margin, buttonHeight,       // ������� � ������� ������
		hwnd,                  // ������������ ����
		(HMENU)ID_BUTTON,      // ������������� ������
		hInstance,             // ���������� ���������� ����������
		NULL                   // �������������� ���������
	);

	ShowWindow(hwnd, nCmdShow);

	// Run the message loop.

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	GdiplusShutdown(gdiplusToken);

	return 0;
}

// Draw help functions

void DrawShape(Graphics* graphics, Color color, int shapeType, int x1, int y1, int x2, int y2)
{
	SolidBrush brush(color);
	//SolidBrush brush(Color(255, 0, 0, 255));

	int start_x, start_y;
	int width, height;
	if (x1 < x2)
	{
		start_x = x1;
		width = x2 - x1;
	}
	else
	{
		start_x = x2;
		width = x1 - x2;
	}
	if (y1 < y2)
	{
		start_y = y1;
		height = y2 - y1;
	}
	else
	{
		start_y = y2;
		height = y1 - y2;
	}


	if (shapeType == Shape::RECTANGLE)
	{
		RectF rect(start_x, start_y, width, height);
		graphics->FillRectangle(&brush, rect);
	}
	else if (shapeType == Shape::CIRCLE)
	{
		RectF ellipseRect(start_x, start_y, width, height);
		graphics->FillEllipse(&brush, ellipseRect);
	}
}

void StartDraw(LPARAM lParam)
{
	isDrawing = true;
	currentShape = new Shape(choosenShapeType);
	currentShape->x1 = currentShape->x2 = LOWORD(lParam);
	currentShape->y1 = currentShape->y2 = HIWORD(lParam);
	currentShape->color = Color(GetRValue(currentColor), GetGValue(currentColor), GetBValue(currentColor));
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
	currentShape = nullptr;
}

// Color picker

void OpenColorPicker(HWND hwnd) {
	CHOOSECOLOR cc;
	COLORREF acrCustColors[16]; // ������ ��� ���������������� ������

	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwnd; // ������������ ���� ��� ����������� ���� ������ �����
	cc.lpCustColors = (LPDWORD)acrCustColors;
	cc.rgbResult = RGB(255, 0, 0); // ��������� ��������� ����
	cc.Flags = CC_FULLOPEN | CC_RGBINIT; // ��������� ������ ����� ����� � ������������� RGB

	if (ChooseColor(&cc) == TRUE) {
		currentColor = cc.rgbResult;
		if (currentShape != nullptr)
			currentShape->color = Color(GetRValue(currentColor), GetGValue(currentColor), GetBValue(currentColor));
	}
}

// Procedure

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BUTTON) { // ID_BUTTON - ��� ������������� ����� ������
			OpenColorPicker(hwnd);
		}
		if (LOWORD(wParam) == ID_LAYER_BUTTON) {
			wchar_t layerText[20];
			swprintf_s(layerText, sizeof(layerText) / sizeof(layerText[0]), L"Layer %d", ++layersCount);
			SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)layerText);
		}
		break;

	case WM_CREATE:
		hwndListBox = CreateWindow(L"LISTBOX", L"",
			WS_VISIBLE | WS_CHILD | LBS_STANDARD,
			margin, margin * 2 + buttonHeight, toolsWidth - 2 * margin, listHeight, hwnd, (HMENU)ID_LISTBOX, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

		SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)L"Layer 1");
		layersCount++;
		SendMessage(hwndListBox, LB_SETCURSEL, 0, 0);

		AddLayerButton = CreateWindow(
			L"BUTTON",             // ��� ������ ��� ������
			L"Add layer",  // ����� �� ������
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // ����� ������
			margin, margin * 3 + buttonHeight + listHeight, toolsWidth - 2 * margin, buttonHeight,       // ������� � ������� ������
			hwnd,                  // ������������ ����
			(HMENU)ID_LAYER_BUTTON,      // ������������� ������
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),             // ���������� ���������� ����������
			NULL                   // �������������� ���������
		);

		break;

	case WM_DESTROY:
		shapes.clear();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}

	case WM_SIZE:
		rc.right = LOWORD(lParam);
		rc.bottom = HIWORD(lParam);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		drawWindowHeight = clientRect.bottom - clientRect.top - 2 * margin;
		drawWindowWidth = clientRect.right - clientRect.left - toolsWidth - margin;
		SetWindowPos(hwndChild, NULL, drawWindowX, drawWindowY, drawWindowWidth, drawWindowHeight, SWP_NOZORDER);
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

LRESULT CALLBACK DrawWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_CREATE:
		hdc = GetDC(hwnd);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		hdcBuffer = CreateCompatibleDC(hdc);
		hBitmap = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
		SelectObject(hdcBuffer, hBitmap);
		graphics = Graphics::FromHDC(hdcBuffer);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdcBuffer, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		for (int i = 0; i < shapes.size(); i++)
		{
			Shape shape = *shapes[i];
			DrawShape(graphics, shape.color, shape.shapeType, shape.x1, shape.y1, shape.x2, shape.y2);
		}
		if (currentShape != nullptr)
			DrawShape(graphics, currentShape->color, currentShape->shapeType, currentShape->x1, currentShape->y1, currentShape->x2, currentShape->y2);

		BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, hdcBuffer, 0, 0, SRCCOPY);

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
		GetClientRect(hwnd, &clientRect);
		drawWindowHeight = clientRect.bottom - clientRect.top - 2 * margin;
		drawWindowWidth = clientRect.right - clientRect.left - toolsWidth - margin;
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
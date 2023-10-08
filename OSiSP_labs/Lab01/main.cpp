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
//#include "layers.cpp"
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
Shape* currentShape = nullptr;
bool isDrawing = false; // while LMB clicked
bool isEditing = false;
// Editing staff
Shape* selectedShape = nullptr;
bool isCornerSelected = false;
bool isLtCornerSelected = false;
int deltaWay = 20;
COLORREF dColors[16];
COLORREF currentColor = RGB(255, 0, 0);

class Layers
{
private:
	std::vector<std::vector<Shape*>*>* layers;
	int layersCount = 0;
	std::vector<Shape*>* currentLayer = nullptr;
public:
	Layers()
	{
		layers = new std::vector<std::vector<Shape*>*>;
		addLayer();
	}
	int getLayersCount()
	{
		return layersCount;
	}
	std::vector<Shape*>* getCurrentLayer()
	{
		return currentLayer;
	}
	std::vector<Shape*>* addLayer()
	{
		layers->push_back(new std::vector<Shape*>);
		currentLayer = (*layers)[layersCount++];
		return currentLayer;
	}
	std::vector<Shape*>* operator[](int index)
	{
		return (*layers)[index];
	}
	std::vector<Shape*>* setCurrentLayer(int index)
	{
		if (getLayersCount() > index && index >= 0)
			currentLayer = (*layers)[index];
		return currentLayer;
	}
	~Layers()
	{
		for (int i = 0; i < layers->size(); i++)
		{
			for (int j = 0; j < layers[i].size(); j++)
			{
				delete(layers[i][j]);
			}
			layers[i].clear();
		}
		layers->clear();
	}
};

Layers* layers;

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

	hwndChild = CreateWindowEx(0, L"DrawWindowClass", L"Дочернее Окно", WS_CHILD | WS_VISIBLE | WS_BORDER,
		drawWindowX, drawWindowY, drawWindowWidth, drawWindowHeight, hwnd, (HMENU)NULL, hInstance, NULL);

	if (hwndChild == NULL) 
	{
		return 0;
	}

	HWND hwndButton = CreateWindow(
		L"BUTTON",             // Имя класса для кнопки
		L"Choose color",  // Текст на кнопке
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Стили кнопки
		margin, margin, toolsWidth - 2 * margin, buttonHeight,       // Позиция и размеры кнопки
		hwnd,                  // Родительское окно
		(HMENU)ID_BUTTON,      // Идентификатор кнопки
		hInstance,             // Дескриптор экземпляра приложения
		NULL                   // Дополнительные параметры
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
	std::vector<Shape*>* currentLayer = layers->getCurrentLayer();
	for (int i = currentLayer->size() - 1; i >= 0; i--)
	{
		Shape* shape = (*currentLayer)[i];
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (shape->x1 <= x && shape->x2 >= x && shape->y1 <= y && shape->y2 >= y)
		{
			selectedShape = shape;
			break;
		}
	}
}

void SelectCorner(LPARAM lParam)
{
	int x1 = selectedShape->x1;
	int y1 = selectedShape->y1;
	int x2 = selectedShape->x2;
	int y2 = selectedShape->y2;
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
		selectedShape->x1 = LOWORD(lParam);
		selectedShape->y1 = HIWORD(lParam);
	}
	else
	{
		selectedShape->x2 = LOWORD(lParam);
		selectedShape->y2 = HIWORD(lParam);
	}
	selectedShape->checkCoord();
	isCornerSelected = false;
	selectedShape = nullptr;
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
	layers->getCurrentLayer()->push_back(currentShape);
	currentShape = nullptr;
}

// Color picker

void OpenColorPicker(HWND hwnd) {
	CHOOSECOLOR cc;
	COLORREF acrCustColors[16]; // Массив для пользовательских цветов

	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwnd; // Родительское окно для диалогового окна выбора цвета
	cc.lpCustColors = (LPDWORD)acrCustColors;
	cc.rgbResult = RGB(255, 0, 0); // Начальный выбранный цвет
	cc.Flags = CC_FULLOPEN | CC_RGBINIT; // Разрешает полный выбор цвета и инициализацию RGB

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
		if (LOWORD(wParam) == ID_BUTTON) { // ID_BUTTON - это идентификатор вашей кнопки
			OpenColorPicker(hwnd);
			SetFocus(hwndChild);
		}
		if (LOWORD(wParam) == ID_LAYER_BUTTON) {
			if (layers->getLayersCount() < 9)
			{
				layers->addLayer();
				wchar_t layerText[20];
				swprintf_s(layerText, sizeof(layerText) / sizeof(layerText[0]), L"Layer %d", layers->getLayersCount());
				SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)layerText);
			}
			SetFocus(hwndChild);
		}
		if (LOWORD(wParam) == ID_LISTBOX && HIWORD(wParam) == LBN_SELCHANGE) {
			int selectedIndex = SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);
			layers->setCurrentLayer(selectedIndex);
			selectedShape = nullptr;
			isCornerSelected = false;
			SetFocus(hwndChild);
		}
		break;

	case WM_CREATE:
		hwndListBox = CreateWindow(L"LISTBOX", L"",
			WS_VISIBLE | WS_CHILD | LBS_STANDARD,
			margin, margin * 2 + buttonHeight, toolsWidth - 2 * margin, listHeight, hwnd, (HMENU)ID_LISTBOX, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

		layers = new Layers();
		SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)L"Layer 1");

		SendMessage(hwndListBox, LB_SETCURSEL, 0, 0);

		AddLayerButton = CreateWindow(
			L"BUTTON",             // Имя класса для кнопки
			L"Add layer",  // Текст на кнопке
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Стили кнопки
			margin, margin * 3 + buttonHeight + listHeight, toolsWidth - 2 * margin, buttonHeight,       // Позиция и размеры кнопки
			hwnd,                  // Родительское окно
			(HMENU)ID_LAYER_BUTTON,      // Идентификатор кнопки
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),             // Дескриптор экземпляра приложения
			NULL                   // Дополнительные параметры
		);

		break;

	case WM_DESTROY:
		delete(layers);
		delete(currentShape);
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

		for (int i = 0; i < layers->getLayersCount(); i++)
		{
			for (int j = 0; j < (*layers)[i]->size(); j++)
			{
				Shape* shape = (*(*layers)[i])[j];
				DrawShape(graphics, shape->color, shape->shapeType, shape->x1, shape->y1, shape->x2, shape->y2);
			}
			if (layers->getCurrentLayer() == (*layers)[i] && currentShape != nullptr)
			{
				DrawShape(graphics, currentShape->color, currentShape->shapeType, currentShape->x1, currentShape->y1, currentShape->x2, currentShape->y2);
			}
		}

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
			if (selectedShape == nullptr)
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
		SetFocus(hwndChild);
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
					EndDrawing();
				}
				isEditing = true;
			}
			else
			{
				isEditing = false;
				selectedShape = nullptr;
				isCornerSelected = false;
			}
		}
		else if (wParam == VK_LEFT)
		{
			if (isEditing && selectedShape != nullptr)
				selectedShape->moveFigure(-deltaWay, 0);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_RIGHT)
		{
			if (isEditing && selectedShape != nullptr)
				selectedShape->moveFigure(deltaWay, 0);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_UP)
		{
			if (isEditing && selectedShape != nullptr)
				selectedShape->moveFigure(0, -deltaWay);
			InvalidateRect(hwnd, 0, TRUE);
		}
		else if (wParam == VK_DOWN)
		{
			if (isEditing && selectedShape != nullptr)
				selectedShape->moveFigure(0, deltaWay);
			InvalidateRect(hwnd, 0, TRUE);
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
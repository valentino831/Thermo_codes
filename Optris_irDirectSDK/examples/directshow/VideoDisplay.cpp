#include "VideoDisplay.h"
#include <iostream>

bool __displayInstanceMap__[32] = { 0 };
VideoDisplay* __displays__[32];

// Templated display worker function
template <int instance>
DWORD WINAPI displayWorker(LPVOID lpParam)
{
	__displays__[instance]->show();
	__displays__[instance]->messageLoop();
	return 0;
}
typedef DWORD(__stdcall *fpDisplayWorker)(LPVOID lpParam);
static fpDisplayWorker displayWorkerSlot[] =
{
  &displayWorker<0>,
  &displayWorker<1>,
  &displayWorker<2>,
  &displayWorker<3>,
  &displayWorker<4>,
  &displayWorker<5>,
  &displayWorker<6>,
  &displayWorker<7>,
  &displayWorker<8>,
  &displayWorker<9>,
  &displayWorker<10>,
  &displayWorker<11>,
  &displayWorker<12>,
  &displayWorker<13>,
  &displayWorker<14>,
  &displayWorker<15>,
  &displayWorker<16>,
  &displayWorker<17>,
  &displayWorker<18>,
  &displayWorker<19>,
  &displayWorker<20>,
  &displayWorker<21>,
  &displayWorker<22>,
  &displayWorker<23>,
  &displayWorker<24>,
  &displayWorker<25>,
  &displayWorker<26>,
  &displayWorker<27>,
  &displayWorker<28>,
  &displayWorker<29>,
  &displayWorker<30>,
  &displayWorker<31>
};

struct WindowInstance
{
	HWND window;
	VideoDisplay* instance;
	struct WindowInstance *next;
};

WindowInstance *root = 0;

VideoDisplay* GetInstance(HWND hwnd)
{
	WindowInstance* ptr = root;
	while (ptr && ptr->window != hwnd) ptr = ptr->next;
	return ptr ? ptr->instance : 0;
}

VideoDisplay::VideoDisplay(unsigned int width, unsigned int height)
{
	_width = width;
	_height = height;
	// find valid id
	int id = -1;
	for (int i = 0; i < 32; i++)
	{
		if (__displayInstanceMap__[i] == false)
		{
			id = i;
			break;
		}
	}
	if (id == -1)
	{
		std::cout << "Maximum number of instances reached ... aborting" << std::endl;
		abort();
	}
	_instanceID = id;
	__displayInstanceMap__[_instanceID] = true;
	__displays__[_instanceID] = this;
	_window = NULL;
}

VideoDisplay::~VideoDisplay()
{
// free slot for new instances
	if (_valid)
		CloseWindow(_window);
	__displayInstanceMap__[_instanceID] = false;
}


void VideoDisplay::createWindow()
{

	HINSTANCE hinst = GetModuleHandle(NULL);

	//Register Window Class
	WNDCLASSEX windowclass;
	memset(&windowclass, 0, sizeof(WNDCLASSEX));

	windowclass.cbSize = sizeof(WNDCLASSEX);
	windowclass.style = CS_HREDRAW | CS_VREDRAW;
	windowclass.lpfnWndProc = MessageHandler;
	windowclass.hInstance = hinst;
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowclass.lpszMenuName = L"Main Menu";
	windowclass.lpszClassName = L"VideoDisplay";
	windowclass.hIconSm = (HICON)LoadImage(hinst, MAKEINTRESOURCE(5), IMAGE_ICON, GetSystemMetrics(SM_CYSMICON), GetSystemMetrics(SM_CXSMICON), LR_DEFAULTCOLOR);

	RegisterClassEx(&windowclass);

	//Get correct window size with its border and titlebar
	RECT rect;
	rect.left = rect.top = 0;
	rect.right = _width;
	rect.bottom = _height;
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	//Create the Window
	_window = CreateWindow(L"VideoDisplay", L"Optris PI imager example",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, hinst, NULL);

	//Create a compatible backbuffer
	HDC current = GetDC(_window);
	RECT clientrect;

	GetClientRect(_window, &clientrect);
	_backbuffer = CreateCompatibleDC(current);
	_backbitmap = CreateCompatibleBitmap(current, clientrect.right, clientrect.bottom);
	SelectObject(_backbuffer, (HGDIOBJ)_backbitmap);
	ReleaseDC(_window, current);

	//add hwnd to instance reference
	WindowInstance** ptr = &root;
	while (*ptr) ptr = &(*ptr)->next;
	*ptr = (WindowInstance*)calloc(1, sizeof(WindowInstance));
	(*ptr)->window = _window;
	(*ptr)->instance = this;

	_valid = true;
}

void VideoDisplay::show()
{
	if (_window == NULL)
	{
		createWindow();
	}
	ShowWindow(_window, SW_SHOWNORMAL);
	UpdateWindow(_window);
}

HANDLE VideoDisplay::showDetach()
{
	return CreateThread(NULL, 0, displayWorkerSlot[_instanceID], NULL, 0, NULL);
}

void VideoDisplay::run()
{
	bool run = true;
	while (run)
	{
		MSG msg;
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0)
		{
			if (msg.message == WM_QUIT || msg.message == WM_DESTROY || msg.message == WM_CLOSE)
				run = false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void VideoDisplay::drawCapture(int x, int y, int width, int height, int bpp, unsigned char* data)
{
	if (!_valid) return;

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = bpp;
	bmi.bmiHeader.biCompression = BI_RGB;

	SetDIBitsToDevice(_backbuffer, x, y, width, height, 0, 0, 0, (UINT)height, data, &bmi, DIB_RGB_COLORS);

	//invalidate whole window
	RECT invalid;
	GetClientRect(_window, &invalid);
	InvalidateRect(_window, &invalid, false);
}

bool VideoDisplay::dispatchMessages()
{
	MSG msg;
	if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return _valid;
}

bool VideoDisplay::isRunning()
{
	return _valid;
}

void VideoDisplay::messageLoop()
{
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT VideoDisplay::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CLOSE:
	{
		RECT clientrect;

		GetClientRect(hwnd, &clientrect);
		VideoDisplay* cw = GetInstance(hwnd);

		cw->_valid = false;
		PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RECT clientrect;

		GetClientRect(hwnd, &clientrect);
		VideoDisplay* cw = GetInstance(hwnd);

		//scale image with fix aspect ration
		float scaleW = clientrect.right / (float)cw->_width;
		float scaleH = clientrect.bottom / (float)cw->_height;

		if (scaleW < scaleH)
		{
			clientrect.bottom = scaleW * cw->_height;
		}
		else
		{
			clientrect.right = scaleH * cw->_width;
		}

		HDC hdc = BeginPaint(hwnd, &ps);
		StretchBlt(hdc, 0, 0, clientrect.right, clientrect.bottom, cw->_backbuffer, 0, 0, cw->_width, cw->_height, SRCCOPY);
		EndPaint(hwnd, &ps);

		return 0;
	}

	default: return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

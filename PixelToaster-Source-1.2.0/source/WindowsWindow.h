// Windows Window class
// Copyright � Glenn Fiedler
// http://www.pixeltoaster.com

class WindowsWindow
{
public:

	WindowsWindow( WindowsAdapter * adapter, const char title[], int width, int height )
	{
		// setup data

		assert(adapter);

		this->adapter = adapter;
		this->width = width;
		this->height = height;

		// defaults

		window = 0;
		systemMenu = 0;
		active = false;
		listener = 0;
		centered = false;
		zoomLevel = ZOOM_ORIGINAL;
		mode = Windowed;

		// get handle to system arrow cursor

		arrowCursor = LoadCursor(0, IDC_ARROW);

		// clear mouse data

		mouse.x = 0;
		mouse.y = 0;
		mouse.buttons.left = false;
		mouse.buttons.middle = false;
		mouse.buttons.right = false;

		// setup keyboard data

		for (int i=0; i<256; i++) 
		{
			translate[i] = (Key::Code) i;
			down[i] = false;
		}

		translate[219] = Key::OpenBracket;
		translate[221] = Key::CloseBracket;
		translate[220] = Key::BackSlash;
		translate[13]  = Key::Enter;
		translate[187] = Key::Equals;
		translate[189] = Key::Separator;
		translate[186] = Key::SemiColon;
		translate[191] = Key::Slash;
		translate[190] = Key::Period;
		translate[188] = Key::Comma;
		translate[45]  = Key::Insert;
		translate[46]  = Key::Delete;

		// setup window class

		HINSTANCE instance = GetModuleHandle(0);

		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = 0;
		windowClass.lpfnWndProc = &StaticWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = instance;
		windowClass.hIcon = LoadIcon(instance, "DisplayIcon");
		windowClass.hCursor = 0;
		windowClass.hbrBackground = 0;
		windowClass.lpszMenuName = 0;
		windowClass.lpszClassName = title;
		windowClass.hIconSm = 0;

		UnregisterClass(title, instance);

		if (!RegisterClassEx(&windowClass)) 
			return;

		// create window

		window = CreateWindow(title, title, WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, instance, NULL);

		if (!window)
			return;

		SetWindowLongPtr(window, GWL_USERDATA, (LONG)(LONG_PTR) this);

		// setup system menu

		updateSystemMenu();
	}

	~WindowsWindow()
	{
		DestroyWindow(window);
		window = 0;
	}

	// show the window (it is initially hidden)

	void show()
	{
		ShowWindow(window, SW_SHOW);
	}
	
	// hide the window

	void hide()
	{
		ShowWindow(window, SW_HIDE);
	}

	// check if window is visible?

	bool visible() const
	{
		return IsWindowVisible(window)!=0;
	}

	// put window in fullscreen mode

	void fullscreen(int width, int height)
	{
		// 1. hide window
		// 2. hide mouse cursor
		// 3. popup window style
		// 4. move window to cover display entirely
		// 5. show window
		// 6. update system menu

		this->width = width;
		this->height = height;
		
		hide();

		SetCursor(0);

		SetWindowLongPtr(window, GWL_STYLE, WS_POPUPWINDOW);	

		int w = GetSystemMetrics(SM_CXSCREEN);
		int h = GetSystemMetrics(SM_CYSCREEN);

		if (width>w)
			w = width;

		if (height>h)
			h = height;

		SetWindowPos(window, 0, 0, 0, w, h, SWP_NOZORDER);

		show();

		mode = Fullscreen;
		updateSystemMenu();
	}

	// put window in windowed mode

	void windowed(int width, int height)
	{
		// 1. hide window
		// 2. overlapped window style
		// 3. adjust window rect
		// 4. center window
		// 5. show mouse cursor
		// 6. update system menu

		this->width = width;
		this->height = height;
		
		hide();

		SetWindowLongPtr(window, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		RECT rect;
		GetWindowRect(window, &rect);
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);
		SetWindowPos(window, 0, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);

		center();

		SetCursor(arrowCursor);

		mode = Windowed;
		updateSystemMenu();
	}

	// center the window on the desktop
	// note: has no effect if the window is minimized or maximized

	void center()
	{
		if (IsIconic(window) || IsZoomed(window))
			return;

		RECT rect;
		GetWindowRect(window, &rect);
		
		const int width = rect.right - rect.left;
		const int height = rect.bottom - rect.top;
		
		int x = (GetSystemMetrics(SM_CXSCREEN) - width) >> 1;
		int y = (GetSystemMetrics(SM_CYSCREEN) - height) >> 1;
		
		if (x<0)
			x = 0;

		if (y<0)
			y = 0;
	
		SetWindowPos(window, 0, x, y, width, height, SWP_NOZORDER);

		centered = true;

		updateSystemMenu();
	}

	// zoom window

	void zoom(float scale)
	{
		if (IsIconic(window) || IsZoomed(window))
			return;

		// get current window rect and calculate current window center

		RECT rect;
		GetWindowRect(window, &rect);

		const int cx = (rect.left + rect.right) / 2;
		const int cy = (rect.top + rect.bottom) / 2;

		// calculate window rect with origin (0,0)

		rect.left = 0;
		rect.top = 0;
		rect.right = rect.left + (int) (width*scale);
		rect.bottom = rect.top + (int) (height*scale);

		// adjust window rect then make adjust origin back to (0,0)

		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

		if (rect.left<0)
		{
			rect.right -= rect.left;
			rect.left = 0;
		}

		if (rect.top<0)
		{
			rect.bottom -= rect.top;
			rect.top = 0;
		}

		// center zoomed window around previous window center

		const int dx = cx - (rect.right - rect.left) / 2;
		const int dy = cy - (rect.bottom - rect.top) / 2;

		rect.left += dx;
		rect.right += dx;
		rect.top += dy;
		rect.bottom += dy;

		// check that the newly centered window position is origin (0,0) or larger. no negative origin values allowed

		if (rect.left<0)
		{
			rect.right -= rect.left;
			rect.left = 0;
		}

		if (rect.top<0)
		{
			rect.bottom -= rect.top;
			rect.top = 0;
		}

		// finally set the zoomed window position

		SetWindowPos(window, 0, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);
		
		// detect what zoom level we are at and update system menu

		if (scale==1.0f)
			zoomLevel = ZOOM_ORIGINAL;
		else if (scale==2.0f)
			zoomLevel = ZOOM_2X;
		else if (scale==4.0f)
			zoomLevel = ZOOM_4X;
		else if (scale==8.0f)
			zoomLevel = ZOOM_8X;
		else
			zoomLevel = ZOOM_RESIZED;
	}

	// window update pumps the message queue

	void update()
	{
		// hide mouse cursor if fullscreen

		if (mode==Fullscreen && active)
			SetCursor(0);

		// check window

		if (!window)
			return;

		// message pump

		MSG message;

		bool done = false;

		while ( !done )
		{
			if ( PeekMessage( &message, window, 0, 0, PM_REMOVE ) ) 
			{
				TranslateMessage( &message );
				DispatchMessage( &message );
			}
			else
			{
				done = true;
			}

			Sleep(0);
		}
	}

	// get the window handle.
	// null if the window failed to initialize.

	HWND handle() const
	{
		return window;
	}

protected:

	static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LONG_PTR extra = GetWindowLongPtr(hWnd, GWL_USERDATA);

		if (!extra) 
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		WindowsWindow * window = (WindowsWindow*) extra;

		return window->WindowProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			case WM_ACTIVATE:
				active = wParam != WA_INACTIVE;
				if (listener)
					listener->onActivate(active);
				break;

			case WM_PAINT:
				adapter->paint();
				break;

			case WM_SIZING:
				adapter->paint();
				zoomLevel = ZOOM_RESIZED;
			case WM_SIZE:
				updateSystemMenu();
				break;

			case WM_QUIT:
			case WM_CLOSE:
				if (listener)
					listener->onClose();
				else
					adapter->exit();
				break;

			case WM_SETCURSOR:
				if (LOWORD(lParam)==HTCLIENT)
				{
					if (mode==Fullscreen)
						SetCursor(0);
					else
						SetCursor(arrowCursor);
				}
				else
					return DefWindowProc(hWnd, uMsg, wParam, lParam);
				break;

			case WM_SYSCOMMAND:
			{
				switch (wParam)
				{
					case MENU_CENTER:
						center();
						updateSystemMenu();
						break;

					case MENU_ZOOM_ORIGINAL:
						zoom(1.0f);
						updateSystemMenu();
						break;

					case MENU_ZOOM_2X:
						zoom(2.0f);
						updateSystemMenu();
						break;

					case MENU_ZOOM_4X:
						zoom(4.0f);
						updateSystemMenu();
						break;

					case MENU_ZOOM_8X:
						zoom(8.0f);
						updateSystemMenu();
						break;

					case MENU_WINDOWED:
						adapter->windowed();
						break;

					case MENU_FULLSCREEN:
						adapter->fullscreen();
						break;

					default:
						return DefWindowProc(hWnd, uMsg, wParam, lParam);
				}
			}
			break;

			case WM_MOVING:
				if (centered)
				{
					centered = false;
					updateSystemMenu();
				}
				break;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				unsigned char key = (unsigned char)wParam;
			
				if (key==VK_RETURN && GetAsyncKeyState(VK_MENU))
				{
					adapter->toggle();
					break;
				}

				if (!down[key])
				{
					if (listener)
						listener->onKeyDown((Key::Code)translate[key]);
					down[key] = true;
				}

				if (listener)
					listener->onKeyPressed((Key::Code)translate[key]);
			}
			break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				unsigned char key = (unsigned char)wParam;
			
				if (listener)
					listener->onKeyUp((Key::Code)translate[key]);
				else if (key==27)
					adapter->exit();

				down[key] = false;
			}
			break;

			case WM_LBUTTONDOWN:
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					SetCapture(hWnd);
				mouse.buttons.left = true;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonDown(mouse);
				break;

			case WM_MBUTTONDOWN:
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					SetCapture(hWnd);
				mouse.buttons.middle = true;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonDown(mouse);
				break;

			case WM_RBUTTONDOWN:
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					SetCapture(hWnd);
				mouse.buttons.right = true;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonDown(mouse);
				break;

			case WM_LBUTTONUP:
				mouse.buttons.left = false;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonUp(mouse);
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					ReleaseCapture();
				break;

			case WM_MBUTTONUP:
				mouse.buttons.middle = false;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonUp(mouse);
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					ReleaseCapture();
				break;

			case WM_RBUTTONUP:
				mouse.buttons.right = false;
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseButtonUp(mouse);
				if (!(mouse.buttons.left | mouse.buttons.right | mouse.buttons.middle))
					ReleaseCapture();
				break;

			case WM_MOUSEMOVE:
				mouse.x = (float) GET_X_LPARAM(lParam); 
				mouse.y = (float) GET_Y_LPARAM(lParam);
				if (listener)
					listener->onMouseMove(mouse);
				break;

			default:
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}

	// system menu item ids

	enum SystemMenuItems
	{
		MENU_SEPARATOR_A = 1,
		MENU_ZOOM_ORIGINAL,
		MENU_ZOOM_2X,
		MENU_ZOOM_4X,
		MENU_ZOOM_8X,
		MENU_SEPARATOR_B,
		MENU_WINDOWED,
		MENU_FULLSCREEN,
		MENU_SEPARATOR_C,
		MENU_CENTER
	};

	// current zoom level for window

	enum ZoomLevel
	{
		ZOOM_RESIZED,
		ZOOM_ORIGINAL,
		ZOOM_2X,
		ZOOM_4X,
		ZOOM_8X
	};

	// update addition system menu items

	void updateSystemMenu()
	{
		systemMenu = GetSystemMenu(window, FALSE);

		// remove additional items

		RemoveMenu(systemMenu, MENU_SEPARATOR_A, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_ZOOM_ORIGINAL, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_ZOOM_2X, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_ZOOM_4X, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_ZOOM_8X, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_SEPARATOR_B, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_FULLSCREEN, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_WINDOWED, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_SEPARATOR_C, MF_BYCOMMAND);
		RemoveMenu(systemMenu, MENU_CENTER, MF_BYCOMMAND);

		// rebuild menu

		bool windowed = mode==Windowed && !IsIconic(window) && !IsMaximized(window);

		if (windowed)
		{
			AppendMenu(systemMenu, MF_SEPARATOR, MENU_SEPARATOR_A, "");
			AppendMenu(systemMenu, MF_STRING, MENU_ZOOM_ORIGINAL, "Original");

			const int desktopWidth = GetSystemMetrics(SM_CXSCREEN);
			const int desktopHeight = GetSystemMetrics(SM_CYSCREEN);

			if (width*2<desktopWidth && height*2<desktopHeight)
				AppendMenu(systemMenu, MF_STRING, MENU_ZOOM_2X, "2x Zoom");

			if (width*4<desktopWidth && height*4<desktopHeight)
				AppendMenu(systemMenu, MF_STRING, MENU_ZOOM_4X, "4x Zoom");
			
			if (width*8<desktopWidth && height*8<desktopHeight)
				AppendMenu(systemMenu, MF_STRING, MENU_ZOOM_8X, "8x Zoom");
		}

		AppendMenu(systemMenu, MF_SEPARATOR, MENU_SEPARATOR_B, "");

		if (!windowed)
			AppendMenu(systemMenu, MF_STRING, MENU_WINDOWED, "Windowed");
		else
			AppendMenu(systemMenu, MF_STRING, MENU_FULLSCREEN, "Fullscreen");

		if (!centered && windowed)
		{
			AppendMenu(systemMenu, MF_SEPARATOR, MENU_SEPARATOR_C, "");
			AppendMenu(systemMenu, MF_STRING, MENU_CENTER, "Center");
		}
	}

private:

	HWND window;					// window handle
	HMENU systemMenu;				// system menu handle
	int width;						// natural window width
	int height;						// natural window height
	bool active;					// true if window is currently active

	enum Mode
	{
		Fullscreen,
		Windowed
	};

	Mode mode;						// current window mode (fullscreen or windowed)

	Mouse mouse;					// current mouse input data

	Key translate[256];				// key translation table (win32 scancode -> Key::Code)
	bool down[256];					// key down table (true means key is down)

	HCURSOR arrowCursor;			// handle to system arrow cursor (does not need to be freed)

	bool centered;					// true if window is centered
	ZoomLevel zoomLevel;			// current zoom level

	WindowsAdapter * adapter;		// the adapter interface (must not be null)

public:

	Listener * listener;			// the listener interface (may be null)
};

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <Windows.h>
#include "gl.h"

#define WGL_CONTEXT_PROFILE_MASK_ARB				0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB			0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002
#define ERROR_INVALID_PROFILE_ARB					0x2096

#define WGL_CONTEXT_DEBUG_BIT_ARB					0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB		0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB				0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB				0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB					0x2093
#define WGL_CONTEXT_FLAGS_ARB						0x2094
#define ERROR_INVALID_VERSION_ARB					0x2095

typedef HGLRC WINAPI wglCreateContextAttribsARB_t (HDC hDC, HGLRC hShareContext, const int *attribList);
static wglCreateContextAttribsARB_t *wglCreateContextAttribsARB;



LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			if (lparam != 0)
			{
				PostMessage((HWND)lparam, message, wparam, lparam);
			}
		} break;

		/*case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:*/
		case WM_DESTROY:
		case WM_QUIT:
		case WM_CLOSE:
		{
			PostMessage(NULL, message, wparam, lparam);
		} break;

		default:
		{
			return DefWindowProc(window, message, wparam, lparam);
		} break;
	}

	return (0);
}

typedef enum
{
	APP_MOUSE_STATE_BUTTON_LEFT		= 0x01,
	APP_MOUSE_STATE_BUTTON_RIGHT	= 0x02,
	APP_MOUSE_STATE_BUTTON_MIDDLE	= 0x04,
} APP_MOUSE_STATE;

typedef struct
{
	HWND		hwnd;
	HDC			hdc;
	HGLRC		hglrc;
	UINT 		open;
	POINT		size;

	POINT		cursor;
	uint8_t		mouse_state;
	int32_t		wheel_delta;
} app_t;

app_t create_app(const char *title, int32_t width, int32_t height)
{
	HWND hwnd;
	int32_t window_width = width;
	int32_t window_height = height;
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		WNDCLASSEX window_class = { 0 };
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		window_class.lpfnWndProc = (WNDPROC)WindowProc;
		window_class.hInstance = hInstance;
		window_class.hIcon = NULL;
		window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
		window_class.lpszClassName = "PROFILER_WINDOW_CLASS";
		assert(RegisterClassEx(&window_class) != 0);

		RECT desktop_rect = { 0 };
		GetClientRect(GetDesktopWindow(), &desktop_rect);
		RECT window_rect = {
			((desktop_rect.right - desktop_rect.left) / 2) - (window_width / 2),
			((desktop_rect.bottom - desktop_rect.top) / 2) - (window_height / 2),
			((desktop_rect.right - desktop_rect.left) / 2) + (window_width / 2),
			((desktop_rect.bottom - desktop_rect.top) / 2) + (window_height / 2)
		};

		DWORD style = WS_OVERLAPPEDWINDOW;
		AdjustWindowRect(&window_rect, style, FALSE);
		hwnd = CreateWindowEx(WS_EX_APPWINDOW, "PROFILER_WINDOW_CLASS", title, style,
			window_rect.left, window_rect.top,
			window_rect.right - window_rect.left, window_rect.bottom - window_rect.top,
			NULL, NULL, hInstance, NULL);

		ShowWindow(hwnd, SW_SHOW);
	}


	HDC device_context = GetDC (hwnd);//GetDC ((HWND)window);
	HGLRC render_context = NULL;
	// Opengl context
	{
		PIXELFORMATDESCRIPTOR pfd = {
			sizeof (PIXELFORMATDESCRIPTOR),
			1, // Version
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,	// Flags
			PFD_TYPE_RGBA,	// Pixel type
			32,				// Color bits
			0, 0, 0, 0, 0, 0, // red, red shift, green, green shift, blue, blue shift,
			0, 0, // alpha, alpha shift
			0,	 // accum bits
			0, 0, 0, 0, // accum: red, green, blue, alpha
			24,	// Depth bits
			8,	// Stencil bits
			0,	// Aux buffers
			PFD_MAIN_PLANE,	// Layer type
			0,	// Reserved
			0, 0, 0	// Layer mask, visible mask, damage mask
		};



		int32_t pixel_format = ChoosePixelFormat (device_context, &pfd);
		assert (pixel_format != 0);
		assert (SetPixelFormat (device_context, pixel_format, &pfd) == TRUE);

		HGLRC tmp_context = wglCreateContext (device_context);
		wglMakeCurrent (device_context, tmp_context);
		render_context = tmp_context;

		HMODULE opengl_handle = LoadLibrary ("opengl32.dll");
		if (opengl_handle != NULL)
		{
			#define GLE_FUNC(type, name, ...) \
				name = (name##_t *) wglGetProcAddress (#name);\
				if (name == NULL) { name = (name##_t *) GetProcAddress (opengl_handle, #name); if (name == NULL) { printf ("failed to load %s\n", #name); }}

				GL_UNITILITY_LIST
				GL_STATE_MANAGEMENT_LIST
				GL_RENDERING_LIST
				GL_SHADERS_LIST
				GL_BUFFER_OBJECTS_LIST
				GL_TEXTURE_LIST
				GL_FRAMEBUFFER_LIST
			#undef GLE_FUNC
		}

		else
		{
			// Failed to load opengl functions
			printf ("failed to load opengl32.dll");
		}

		int32_t major = 0;
		int32_t minor = 0;
		glGetIntegerv (GL_MAJOR_VERSION, &major);
		glGetIntegerv (GL_MINOR_VERSION, &minor);

		int32_t attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, major,
			WGL_CONTEXT_MINOR_VERSION_ARB, minor,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		wglCreateContextAttribsARB = (wglCreateContextAttribsARB_t *)wglGetProcAddress("wglCreateContextAttribsARB");
		if (wglCreateContextAttribsARB != NULL)
		{
			render_context = wglCreateContextAttribsARB(device_context, 0, attribs);
		}

		wglMakeCurrent (NULL, NULL);
		wglDeleteContext (tmp_context);

		assert (render_context != NULL);
		wglMakeCurrent (device_context, render_context);
	}

	app_t app = { 0 };
	app.hwnd = hwnd;
	app.open = 1;
	app.size = (POINT){ width, height };
	app.hdc = device_context;
	app.hglrc = render_context;
	app.cursor = (POINT){0};
	app.mouse_state = 0;
	app.wheel_delta = 0;

	return (app);
}


void app_swap_buffers (app_t *app)
{
	SwapBuffers (app->hdc);
}

void update_app(app_t *app)
{
	app->wheel_delta = 0;
	int32_t result = 0;
	MSG msg = { 0 };
	while (result = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!(msg.message >= WM_MOUSEFIRST && msg.message >= WM_MOUSELAST))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		switch (msg.message)
		{
			case WM_COMMAND:
			{
				
			} break;

			case WM_QUIT:
			case WM_CLOSE:
			{
				app->open = false;
			} break;

			case WM_LBUTTONDOWN:
			{
				app->mouse_state |= APP_MOUSE_STATE_BUTTON_LEFT;
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;

			case WM_RBUTTONDOWN:
			{
				app->mouse_state |= APP_MOUSE_STATE_BUTTON_RIGHT;
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;

			case WM_LBUTTONUP:
			{
				app->mouse_state &= ~APP_MOUSE_STATE_BUTTON_LEFT;
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;

			case WM_RBUTTONUP:
			{
				app->mouse_state &= ~APP_MOUSE_STATE_BUTTON_RIGHT;
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;

			case WM_MOUSEMOVE:
			{
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;

			case WM_MOUSEWHEEL:
			{
				app->wheel_delta = GET_WHEEL_DELTA_WPARAM (msg.wParam);
				if (app->wheel_delta < 0)
				{
					app->wheel_delta = -1;
				}

				else if (app->wheel_delta > 0)
				{
					app->wheel_delta = 1;
				}
				app->cursor = (POINT) {msg.lParam & 0xFFFF, (msg.lParam >> 16) & 0xFFFF};
			} break;
		}

		{
			RECT rect;
			GetClientRect(app->hwnd, &rect);
			POINT size = { rect.right - rect.left, rect.bottom - rect.top };

			if (size.x != app->size.x || size.y != app->size.y)
			{
				app->size = size;
			}
		}
	}
}


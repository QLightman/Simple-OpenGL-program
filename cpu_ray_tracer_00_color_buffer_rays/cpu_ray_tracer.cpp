#include "cpu_ray_tracer.h"

// ----------------------------------------------------------------------------------------------------------------------------

CString ErrorLog;

// ----------------------------------------------------------------------------------------------------------------------------

CCamera::CCamera()
{
	X = vec3(1.0, 0.0, 0.0);
	Y = vec3(0.0, 1.0, 0.0);
	Z = vec3(0.0, 0.0, 1.0);

	Reference = vec3(0.0, 0.0, 0.0);
	Position = vec3(0.0, 0.0, 5.0);
}

CCamera::~CCamera()
{
}

void CCamera::CalculateRayMatrix()
{
	Vin[0] = X.x; Vin[4] = Y.x; Vin[8] = Z.x;
	Vin[1] = X.y; Vin[5] = Y.y; Vin[9] = Z.y;
	Vin[2] = X.z; Vin[6] = Y.z; Vin[10] = Z.z;

	RayMatrix = Vin * Pin * BiasMatrixInverse * VPin;
}

void CCamera::Look(const vec3 &Position, const vec3 &Reference, bool RotateAroundReference)
{
	this->Reference = Reference;
	this->Position = Position;

	Z = normalize(Position - Reference);
	X = normalize(cross(vec3(0.0f, 1.0f, 0.0f), Z));
	Y = cross(Z, X);

	if(!RotateAroundReference)
	{
		this->Reference = this->Position;
		this->Position += Z * 0.05f;
	}

	CalculateRayMatrix();
}

bool CCamera::OnKeyDown(UINT nChar)
{
	float Distance = 0.125f;

	if(GetKeyState(VK_CONTROL) & 0x80) Distance *= 0.5f;
	if(GetKeyState(VK_SHIFT) & 0x80) Distance *= 2.0f;

	vec3 Up(0.0f, 1.0f, 0.0f);
	vec3 Right = X;
	vec3 Forward = cross(Up, Right);

	Up *= Distance;
	Right *= Distance;
	Forward *= Distance;

	vec3 Movement;

	if(nChar == 'W') Movement += Forward;
	if(nChar == 'S') Movement -= Forward;
	if(nChar == 'A') Movement -= Right;
	if(nChar == 'D') Movement += Right;
	if(nChar == 'R') Movement += Up;
	if(nChar == 'F') Movement -= Up;

	Reference += Movement;
	Position += Movement;

	return Movement.x != 0.0f || Movement.y != 0.0f || Movement.z != 0.0f;
}

void CCamera::OnMouseMove(int dx, int dy)
{
	float sensitivity = 0.25f;

	float hangle = (float)dx * sensitivity;
	float vangle = (float)dy * sensitivity;

	Position -= Reference;

	Y = rotate(Y, vangle, X);
	Z = rotate(Z, vangle, X);

	if(Y.y < 0.0f)
	{
		Z = vec3(0.0f, Z.y > 0.0f ? 1.0f : -1.0f, 0.0f);
		Y = cross(Z, X);
	}

	X = rotate(X, hangle, vec3(0.0f, 1.0f, 0.0f));
	Y = rotate(Y, hangle, vec3(0.0f, 1.0f, 0.0f));
	Z = rotate(Z, hangle, vec3(0.0f, 1.0f, 0.0f));

	Position = Reference + Z * length(Position);

	CalculateRayMatrix();
}

void CCamera::OnMouseWheel(short zDelta)
{
	Position -= Reference;

	if(zDelta < 0 && length(Position) < 500.0f)
	{
		Position += Position * 0.1f;
	}

	if(zDelta > 0 && length(Position) > 0.05f)
	{
		Position -= Position * 0.1f;
	}

	Position += Reference;
}

// ----------------------------------------------------------------------------------------------------------------------------

CCamera Camera;

// ----------------------------------------------------------------------------------------------------------------------------

CRayTracer::CRayTracer()
{
	ColorBuffer = NULL;
}

CRayTracer::~CRayTracer()
{
}

bool CRayTracer::Init()
{
	return true;
}

void CRayTracer::RayTrace(int x, int y)
{
	if(ColorBuffer != NULL)
	{
		vec3 Ray = normalize(*(vec3*)&(Camera.RayMatrix * vec4((float)x + 0.5f, (float)y + 0.5f, 0.0f, 1.0f)));

		BYTE *colorbuffer = (LineWidth * y + x) * 3 + ColorBuffer;

		colorbuffer[2] = Ray.r <= 0.0f ? 0 : Ray.r >= 1.0 ? 255 : (BYTE)(Ray.r * 255);
		colorbuffer[1] = Ray.g <= 0.0f ? 0 : Ray.g >= 1.0 ? 255 : (BYTE)(Ray.g * 255);
		colorbuffer[0] = Ray.b <= 0.0f ? 0 : Ray.b >= 1.0 ? 255 : (BYTE)(Ray.b * 255);
	}
}

void CRayTracer::Resize(int Width, int Height)
{
	this->Width = Width;
	this->Height = Height;

	if(ColorBuffer != NULL)
	{
		delete [] ColorBuffer;
		ColorBuffer = NULL;
	}

	if(Width > 0 && Height > 0)
	{
		LineWidth = Width;

		int WidthMod4 = Width % 4;

		if(WidthMod4 > 0)
		{
			LineWidth += 4 - WidthMod4;
		}

		ColorBuffer = new BYTE[LineWidth * Height * 3];

		memset(&ColorBufferInfo, 0, sizeof(BITMAPINFOHEADER));
		ColorBufferInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		ColorBufferInfo.bmiHeader.biPlanes = 1;
		ColorBufferInfo.bmiHeader.biBitCount = 24;
		ColorBufferInfo.bmiHeader.biCompression = BI_RGB;
		ColorBufferInfo.bmiHeader.biWidth = LineWidth;
		ColorBufferInfo.bmiHeader.biHeight = Height;

		Camera.VPin[0] = 1.0f / (float)Width;
		Camera.VPin[5] = 1.0f / (float)Height;

		float tany = tan(45.0f / 360.0f * (float)M_PI), aspect = (float)Width / (float)Height;

		Camera.Pin[0] = tany * aspect;
		Camera.Pin[5] = tany;
		Camera.Pin[10] = 0.0f;
		Camera.Pin[14] = -1.0f;

		Camera.CalculateRayMatrix();
	}
}

void CRayTracer::Destroy()
{
	if(ColorBuffer != NULL)
	{
		delete [] ColorBuffer;
		ColorBuffer = NULL;
	}
}

void CRayTracer::ClearColorBuffer()
{
	if(ColorBuffer != NULL)
	{
		memset(ColorBuffer, 0, LineWidth * Height * 3);
	}
}

void CRayTracer::SwapBuffers(HDC hDC)
{
	if(ColorBuffer != NULL)
	{
		StretchDIBits(hDC, 0, 0, Width, Height, 0, 0, Width, Height, ColorBuffer, &ColorBufferInfo, DIB_RGB_COLORS, SRCCOPY);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------

CRayTracer RayTracer;

// ----------------------------------------------------------------------------------------------------------------------------

CWnd::CWnd()
{
}

CWnd::~CWnd()
{
}

bool CWnd::Create(HINSTANCE hInstance, char *WindowName, int Width, int Height)
{
	WNDCLASSEX WndClassEx;

	memset(&WndClassEx, 0, sizeof(WNDCLASSEX));

	WndClassEx.cbSize = sizeof(WNDCLASSEX);
	WndClassEx.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WndClassEx.lpfnWndProc = WndProc;
	WndClassEx.hInstance = hInstance;
	WndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClassEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.lpszClassName = "Win32CPURayTracerWindow";

	if(RegisterClassEx(&WndClassEx) == 0)
	{
		ErrorLog.Set("RegisterClassEx failed!");
		return false;
	}

	this->WindowName = WindowName;

	this->Width = Width;
	this->Height = Height;

	DWORD Style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if((hWnd = CreateWindowEx(WS_EX_APPWINDOW, WndClassEx.lpszClassName, WindowName, Style, 0, 0, Width, Height, NULL, NULL, hInstance, NULL)) == NULL)
	{
		ErrorLog.Set("CreateWindowEx failed!");
		return false;
	}

	return RayTracer.Init();
}

void CWnd::RePaint()
{
	x = y = 0;
	InvalidateRect(hWnd, NULL, FALSE);
}

void CWnd::Show(bool Maximized)
{
	RECT dRect, wRect, cRect;

	GetWindowRect(GetDesktopWindow(), &dRect);
	GetWindowRect(hWnd, &wRect);
	GetClientRect(hWnd, &cRect);

	wRect.right += Width - cRect.right;
	wRect.bottom += Height - cRect.bottom;

	wRect.right -= wRect.left;
	wRect.bottom -= wRect.top;

	wRect.left = dRect.right / 2 - wRect.right / 2;
	wRect.top = dRect.bottom / 2 - wRect.bottom / 2;

	MoveWindow(hWnd, wRect.left, wRect.top, wRect.right, wRect.bottom, FALSE);

	ShowWindow(hWnd, Maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
}

void CWnd::MsgLoop()
{
	MSG Msg;

	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}

void CWnd::Destroy()
{
	RayTracer.Destroy();

	DestroyWindow(hWnd);
}

void CWnd::OnKeyDown(UINT Key)
{
	if(Camera.OnKeyDown(Key))
	{
		RePaint();
	}
}

void CWnd::OnMouseMove(int X, int Y)
{
	if(GetKeyState(VK_RBUTTON) & 0x80)
	{
		Camera.OnMouseMove(LastX - X, LastY - Y);

		LastX = X;
		LastY = Y;

		RePaint();
	}
}

void CWnd::OnMouseWheel(short zDelta)
{
	Camera.OnMouseWheel(zDelta);

	RePaint();
}

void CWnd::OnPaint()
{
	PAINTSTRUCT ps;

	HDC hDC = BeginPaint(hWnd, &ps);

	static DWORD Start;
	static bool RayTracing;

	if(x == 0 && y == 0)
	{
		RayTracer.ClearColorBuffer();

		Start = GetTickCount();

		RayTracing = true;
	}

	DWORD start = GetTickCount();

	while(GetTickCount() - start < 125 && y < Height)
	{
		int x16 = x + 16, y16 = y + 16;

		for(int yy = y; yy < y16; yy++)
		{
			if(yy < Height)
			{
				for(int xx = x; xx < x16; xx++)
				{
					if(xx < Width)
					{
						RayTracer.RayTrace(xx, yy);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}

		x = x16;

		if(x >= Width)
		{
			x = 0;
			y = y16;
		}
	}

	RayTracer.SwapBuffers(hDC);

	if(RayTracing)
	{
		if(y >= Height)
		{
			RayTracing = false;
		}

		DWORD End = GetTickCount();

		CString text = WindowName;

		text.Append(" - %dx%d", Width, Height);
		text.Append(", Time: %.03f s", (float)(End - Start) * 0.001f);

		SetWindowText(hWnd, text);

		InvalidateRect(hWnd, NULL, FALSE);
	}

	EndPaint(hWnd, &ps);
}

void CWnd::OnRButtonDown(int X, int Y)
{
	LastX = X;
	LastY = Y;
}

void CWnd::OnSize(int Width, int Height)
{
	this->Width = Width;
	this->Height = Height;

	RayTracer.Resize(Width, Height);

	RePaint();
}

// ----------------------------------------------------------------------------------------------------------------------------

CWnd Wnd;

// ----------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uiMsg)
	{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_MOUSEMOVE:
			Wnd.OnMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;

		case 0x020A: // WM_MOUSWHEEL
			Wnd.OnMouseWheel(HIWORD(wParam));
			break;

		case WM_KEYDOWN:
			Wnd.OnKeyDown((UINT)wParam);
			break;

		case WM_PAINT:
			Wnd.OnPaint();
			break;

		case WM_RBUTTONDOWN:
			Wnd.OnRButtonDown(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_SIZE:
			Wnd.OnSize(LOWORD(lParam), HIWORD(lParam));
			break;

		default:
			return DefWindowProc(hWnd, uiMsg, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR sCmdLine, int iShow)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if(Wnd.Create(hInstance, "CPU ray tracer 00 - Color buffer, rays", 800, 600))
	{
		Wnd.Show();
		Wnd.MsgLoop();
	}
	else
	{
		MessageBox(NULL, ErrorLog, "Error", MB_OK | MB_ICONERROR);
	}

	Wnd.Destroy();

	return 0;
}

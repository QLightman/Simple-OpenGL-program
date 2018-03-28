#include <windows.h>

#include "string.h"
#include "glmath.h"

// ----------------------------------------------------------------------------------------------------------------------------

class CCamera
{
public:
	vec3 X, Y, Z, Reference, Position;
	mat4x4 Vin, Pin, VPin, RayMatrix;

public:
	CCamera();
	~CCamera();

	void CalculateRayMatrix();
	void Look(const vec3 &Position, const vec3 &Reference, bool RotateAroundReference = false);
	bool OnKeyDown(UINT nChar);
	void OnMouseMove(int dx, int dy);
	void OnMouseWheel(short zDelta);
};

// ----------------------------------------------------------------------------------------------------------------------------

class CRayTracer
{
private:
	BYTE *ColorBuffer;
	BITMAPINFO ColorBufferInfo;
	int Width, LineWidth, Height;

public:
	CRayTracer();
	~CRayTracer();

	bool Init();
	void RayTrace(int x, int y);
	void Resize(int Width, int Height);
	void Destroy();

	void ClearColorBuffer();
	void SwapBuffers(HDC hDC);
};

// ----------------------------------------------------------------------------------------------------------------------------

class CWnd
{
protected:
	char *WindowName;
	HWND hWnd;
	int Width, Height, x, y, LastX, LastY;

public:
	CWnd();
	~CWnd();

	bool Create(HINSTANCE hInstance, char *WindowName, int Width, int Height);
	void RePaint();
	void Show(bool Maximized = false);
	void MsgLoop();
	void Destroy();

	void OnKeyDown(UINT Key);
	void OnMouseMove(int X, int Y);
	void OnMouseWheel(short zDelta);
	void OnPaint();
	void OnRButtonDown(int X, int Y);
	void OnSize(int Width, int Height);
};

// ----------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR sCmdLine, int iShow);

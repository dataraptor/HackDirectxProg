#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include "hook.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparm, LPARAM lparm);

LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class

// define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define CUSTOMFVF  (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)  // Flexible Vertex Format
typedef struct {
	FLOAT x, y, z, rhw;   // cordinate and rhw:transformed vertex
	DWORD color;          // for D3DFVF_DIFFUSE. Color For Vertex
} CUSTOMVERTEX;


bool DrawMessage(LPD3DXFONT font, unsigned int x, unsigned int y, int alpha, unsigned char r, unsigned char g, unsigned char b, LPCSTR Message)
{	// Create a colour for the text
	D3DCOLOR fontColor = D3DCOLOR_ARGB(alpha, r, g, b);
	RECT rct; //Font
	rct.left=x;
	rct.right=1680;
	rct.top=y;
	rct.bottom=rct.top+200;
	font->DrawTextA(NULL, Message, -1, &rct, 0, fontColor);
	return true;
}


LPDIRECT3DVERTEXBUFFER9 v_buffer;   // stores info about vertices in vram or ram
LPD3DXFONT font;
void InitGraphics(void) {
	CUSTOMVERTEX vertices[] =
    {
        { 320.0f, 50.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 0, 255), },
        { 520.0f, 400.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 255, 0), },
        { 120.0f, 400.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(255, 0, 0), },
    };

	d3ddev->CreateVertexBuffer( 3 * sizeof(CUSTOMVERTEX),
								0, 
								CUSTOMFVF,
								D3DPOOL_MANAGED, // create buffer video memory
								&v_buffer,
								NULL );

	VOID *pvoid;  // the void pointer to v_buffer
	v_buffer->Lock(0, 0, (void**)&pvoid, 0); // tells video hardware not to move v_buffer
	memcpy(pvoid, vertices, sizeof(vertices));  // copy vertices to v_buffer
	v_buffer->Unlock();


	// Create font
	D3DXCreateFont( d3ddev, 17, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &font );
}

void InitD3D(HWND hwnd) {
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3ddp;  // d3d device parameter
	memset(&d3ddp, 0, sizeof(d3ddp));
	
	d3ddp.Windowed = TRUE;  // Fullscreen
	d3ddp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3ddp.hDeviceWindow = hwnd;
	d3ddp.BackBufferFormat = D3DFMT_X8R8G8B8;  // Set the back buffer format to 32-bit
	d3ddp.BackBufferWidth = SCREEN_WIDTH;
	d3ddp.BackBufferHeight = SCREEN_HEIGHT;
	
	// Creates a graphics device interface
	d3d->CreateDevice( D3DADAPTER_DEFAULT,  // Select a graphics device 
					   D3DDEVTYPE_HAL,  // Hardware Abstraction Layer
					   hwnd, 
				       D3DCREATE_SOFTWARE_VERTEXPROCESSING,  // do all 3d calc in software
					   &d3ddp, 
					   &d3ddev );

	// Initialize graphics
	InitGraphics();
}


IDirect3DSurface9 *backbf, *copy;
int i = 0;
int fps = 0;
void RenderFrame(void) {

	// clear the window to deep blue
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(i*1, 40, 100), 0.1f, 0);  i++;
	d3ddev->BeginScene();  // begin 3d scene. Locks Buffer in video memory
	
	// do 3d rendaring on the back buffer
	d3ddev->SetFVF(CUSTOMFVF);  // set vertex format
	d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(CUSTOMVERTEX)); // select vertex buffer
	d3ddev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);   // we want to draw a triangle with the vertices



	DrawMessage(font, 10, 10, 255, 150, 150, 150, "HELLO");
	// Update fps value
	char buff[100];
	sprintf(buff, "%d", fps);
	DrawMessage(font, 10, 30, 255, 15, 150, 150, buff);


	//d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbf);
	//D3DXSaveSurfaceToFile(L"G:\\1\\1.bmp", D3DXIFF_BMP, backbf, NULL, NULL);


	d3ddev->EndScene();  // unlocks buffer in video memory
	d3ddev->Present(NULL, NULL, NULL, NULL);  // display rendered frame



	
}

void CloseD3D(void) {
	v_buffer->Release();  // close and release vertex buffer
	d3ddev->Release();	// close 3d device
	d3d->Release();  // release direct3d
}

float lastframetime = 0;
void CalcFPS(void) {
	float currframetime = (float)timeGetTime();
	float delta = (currframetime - lastframetime) * 0.001f;
	fps = 1/delta;
	lastframetime = currframetime;
}



typedef HRESULT (__stdcall * EndScene_Func)(IDirect3DDevice9 * pDevice);
EndScene_Func EndScene_Game;  //store games EndScene Function Location

Hook EndScene_Hook;

HRESULT __stdcall hk_EndScene(IDirect3DDevice9 * pDevice)
{
	//drawing
	EndScene_Hook.Clear();
	HRESULT res = EndScene_Game(pDevice);
	EndScene_Hook.Restore();

	return res;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	
	HWND hwnd;  //handle for the window
	WNDCLASSEX wc;  //info about window
	memset(&wc, 0, sizeof(wc));  //clear garbage value

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground = (HBRUSH)COLOR_WINDOW;  // Make window transparent
	wc.lpszClassName = L"WindowClass1";
	RegisterClassEx(&wc);

	hwnd = CreateWindowEx(NULL,
                      L"WindowClass1",    // name of the window class
                      L"Our First Windowed Program",   // title of the window
					  CS_HREDRAW | CS_VREDRAW,  // Fullscreen. POPUP:Remove border
                      500,    // x-position of the window
                      60,    // y-position of the window
					  SCREEN_WIDTH,     // width of the window
                      SCREEN_HEIGHT,    // height of the window
                      NULL,   // we have no parent window, NULL
                      NULL,   // we aren't using menus, NULL
                      hInstance,    // application handle
                      NULL);  // used with multiple windows, NULL
	ShowWindow(hwnd, SW_SHOW);
	InitD3D(hwnd);




	// Hooking
	void ** pVTable = *reinterpret_cast<void***>(d3ddev);
	EndScene_Game = (EndScene_Func)pVTable[42];
	
	EndScene_Hook.Init(EndScene_Game, hk_EndScene);




	MSG msg;
    // Enter the infinite message loop
	while(TRUE) {
	    // Check to see if any messages are waiting in the queue
	    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	        // Translate the message and dispatch it to WindowProc()
	        TranslateMessage(&msg);
	        DispatchMessage(&msg);
	    }
	
	    // If the message is WM_QUIT, exit the while loop
	    if(msg.message == WM_QUIT)
	        break;
	
	    // Run game code here
	    // ...
	    // ...

		CalcFPS();

		RenderFrame();
	}
	CloseD3D();

    // return this part of the WM_QUIT message to Windows
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparm, LPARAM lparm) {
	// sort through and find what code to run for the message given
    switch(message) {
        // this message is read when the window is closed
        case WM_DESTROY: {
            // close the application entirely
            PostQuitMessage(0);
            return 0;
        } break;
    }
    // Handle any messages the switch statement didn't
    return DefWindowProc (hwnd, message, wparm, lparm);
}


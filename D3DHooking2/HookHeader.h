#pragma once
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

#define DLogs(x,...) { if(x) {printf("========"); printf(__VA_ARGS__); printf("=========\n"); } else {printf(__VA_ARGS__); printf("\n");} }

using namespace std;
void* dtable[119];
void DrawFilledRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* dev);


namespace d3dhelper {
	static HWND window;
	
	//Get Current Process PID 
	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) { 
		
		DWORD pid; //Current Process's PID
		GetWindowThreadProcessId(handle, &pid);

		//if not equal with Current Thread's PID 
		if (GetCurrentProcessId() != pid) {
			return TRUE;
		}		
		window = handle;
		return FALSE;
	}

	// Get Current Process's window handle
	HWND GetProcessWindow() {
		window = NULL;
		EnumWindows(EnumWindowsCallback, NULL);
		return window;
	}

	bool GetD3D9Device(void** pTable, size_t Size) {
		DLogs(1,"Initialize Dummy Device");
		if (!pTable)
			return false;

		//Create D3D Instance Pointer
		IDirect3D9* pd3d = Direct3DCreate9(D3D_SDK_VERSION);
		if (!pd3d)
			return false;
		IDirect3DDevice9* pdd = NULL;

		//Dummy Device Options 
		D3DPRESENT_PARAMETERS d3dpp = {};	//parameters
		d3dpp.Windowed = false;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = GetProcessWindow(); //get process handle

		//Create Dummy Device
		HRESULT ddc = pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pdd);

		if (ddc != S_OK)
		{
			//if window is fullscreen mode, it may fail -> try again in windowed mode
			d3dpp.Windowed = !d3dpp.Windowed;
			DLogs(1, "Try Again in Window Mode");
			ddc = pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pdd);

			if (ddc != S_OK) { //Fail
				DLogs(1, "Find Window Fail");
				pd3d->Release();
				return false;
			}
		}

		memcpy(pTable, *reinterpret_cast<void***>(pdd), Size);

		pdd->Release();
		pd3d->Release();
		DLogs(1, "Dummy Device Initializing success");
		return true;
	}
}

typedef HRESULT (APIENTRY* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
tEndScene oEndScene = NULL;

namespace hook {
	HRESULT APIENTRY hEndScene(LPDIRECT3DDEVICE9 pDevice) {
		/*DLogs(0, "Hook Successful!");*/
		DrawFilledRect(200, 200, 200, 200, D3DCOLOR_ARGB(255, 255, 0, 0), pDevice);
		return oEndScene(pDevice);
	}
	//minimum length size is 12
	PVOID hookTramp(DWORD64 src, DWORD64 target, DWORD len) {
		DLogs(1, "Make Trampoline");
		if (len < 12) {
			DLogs(0, "too small overwrite length");
			return 0;
		}
		DWORD protect = 0;
		PVOID tmp = 0;
		VirtualProtect((PVOID)src, len, PAGE_EXECUTE_READWRITE, &protect);

		//make trampoline
		// push rax
		// movabs rax, 0xCCCCCCCCCCCCCCCC
		// xchg rax, [rsp]
		// ret
		BYTE stub[] = {
			0x50, 0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x87, 0x04, 0x24, 0xC3
		};

		//Allocate Trampoline memory
		PVOID tramp = VirtualAlloc(0, len + sizeof(stub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		//Set Original Address in Trampoline Function
		tmp = &(stub[3]);
		*((DWORD64*)tmp) = src + len;

		if (tramp) {
			//Copy source's original Codes to trampoline
			memcpy(tramp, (PVOID)src, len);
			memcpy( (PBYTE)tramp + len, stub, sizeof(stub));
		}
		else
			return 0;

		//Overwrite Original Function to Jumped to hook function 
		//mov rax
		*((LPWORD)src) = 0xB848;
		tmp = (LPBYTE)src + 2;

		//Target address
		*((DWORD64*)tmp) = target;
		tmp = (LPBYTE)tmp + 8;

		//jmp rax
		*((LPWORD)tmp) = 0xE0FF;
		tmp = (LPBYTE)tmp + 2;

		//overwrite 0x90 to remaining area
		for (int i = 0; i != len - 12; i++, tmp = (LPBYTE)tmp + 1)
		{
			*((LPBYTE)tmp) = 0x90;
		}

		return tramp;
	}


}

void DrawFilledRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* dev)
{
	D3DRECT BarRect = { x, y, x + w, y + h };
	dev->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, color, 0, 0);
}
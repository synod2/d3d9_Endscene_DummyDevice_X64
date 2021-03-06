// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "HookHeader.h"

DWORD HookMain() {
    FILE* pFile = nullptr;
    if (AllocConsole()) {
        freopen_s(&pFile, "CONIN$", "rb", stdin);
        freopen_s(&pFile, "CONOUT$", "wb", stdout);
        freopen_s(&pFile, "CONOUT$", "wb", stderr);
    }
    else
    {
        return 0;
    }
       
    //dummy device hooking
    d3dhelper::GetD3D9Device(dtable, sizeof(dtable));
    DWORD64 pEndScene = (DWORD64)dtable[42];
    
    cout << "pEndScene : " << setbase(16) << pEndScene << endl;

    if (pEndScene) {
        oEndScene = (tEndScene)hook::hookTramp(pEndScene, (DWORD64)hook::hEndScene, 18);
    }

    if (!oEndScene)
    {
        DLogs(0, "EndScene Hook Fail");
    }

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HookMain, hModule, 0, nullptr));
        //HookMain();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include "main.h"
#include "..\include\jytrace.h"

SVC_CONTEXT sc;

BOOL ServicePowerEvent(VOID)
{
    return 0;
}

// Based on http://msdn.microsoft.com/en-us/library/windows/desktop/ms683241(v=vs.85).aspx,
// we can only use values from 128 - 255
BOOL ServiceControl(DWORD dwControlCode)
{
    sc.dwCtrlCode = dwControlCode;
    SetEvent(sc.hEvtMain[EVENT_CTRL_PROCESS]);
    return 0;
}

BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress)
{
    SERVICE_STATUS ServiceStatus;
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE | SERVICE_ACCEPT_STOP;
    ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = dwProgress;
    ServiceStatus.dwWaitHint = 60000;

    return SetServiceStatus(sc.hStatusHandle, &ServiceStatus);
}

void ServiceCleanup(VOID)
{
    if (sc.hMainThread)
    {
        DWORD dwExitCode;

        GetExitCodeThread(sc.hMainThread, &dwExitCode);

        if (dwExitCode == STILL_ACTIVE)
            SetEvent(sc.hEvtMain[EVENT_CTRL_EXIT]);

        if (WaitForSingleObject(sc.hMainThread, 5000) != WAIT_OBJECT_0)
            TerminateThread(sc.hMainThread, FALSE);

        CloseHandle(sc.hMainThread);
    }

    for (int i = 0; i < EVENT_CTRL_COUNT; i++)
        CloseHandle(sc.hEvtMain[i]);
}

DWORD WINAPI ServiceHandlerEx(DWORD dwControlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch (dwControlCode)
    {
        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            DWORD dwCurrentSessionId = ((WTSSESSION_NOTIFICATION*)lpEventData)->dwSessionId;
            EventWriteHexInfo(M, FL, FN, L"Session ID", dwCurrentSessionId);

            switch (dwEventType)
            {
                case WTS_CONSOLE_CONNECT:
                    EventWriteInfoW(M, FL, FN, L"WTS_CONSOLE_CONNECT");
                    break;
                case WTS_SESSION_UNLOCK:
                    EventWriteInfoW(M, FL, FN, L"WTS_SESSION_UNLOCK");
                    break;
                case WTS_SESSION_LOGON:
                    EventWriteInfoW(M, FL, FN, L"WTS_SESSION_LOGON");                    
                    break;
                case WTS_SESSION_LOGOFF:
                    EventWriteInfoW(M, FL, FN, L"WTS_SESSION_LOGOFF");
                    break;
                case WTS_CONSOLE_DISCONNECT:
                    EventWriteInfoW(M, FL, FN, L"WTS_CONSOLE_DISCONNECT");
                    break;
                case WTS_SESSION_LOCK:
                    EventWriteInfoW(M, FL, FN, L"WTS_SESSION_LOCK");
                    break;
            }

            break;
        }

        case SERVICE_CONTROL_POWEREVENT:
        {
            EventWriteInfoW(M, FL, FN, L"SERVICE_CONTROL_POWEREVENT");

            switch (dwEventType)
            {
                case PBT_APMSUSPEND:
                    EventWriteInfoW(M, FL, FN, L"PBT_APMSUSPEND");
                    break;
                case PBT_APMRESUMECRITICAL:
                    EventWriteInfoW(M, FL, FN, L"PBT_APMRESUMECRITICAL");
                    break;
                case PBT_APMRESUMEAUTOMATIC:
                    EventWriteInfoW(M, FL, FN, L"PBT_APMRESUMEAUTOMATIC");
                    break;
                case PBT_APMRESUMESUSPEND:
                    EventWriteInfoW(M, FL, FN, L"PBT_APMRESUMESUSPEND");
                    break;
            }

            break;
        }

        case SERVICE_CONTROL_STOP:
            if (sc.hMainThread) ServiceCleanup();
            break;
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_PRESHUTDOWN:
        case 0x00000040 /*SERVICE_CONTROL_USERMODEREBOOT*/:
            break;

        default: ServiceControl(dwControlCode);
    }

    return NO_ERROR;
}

DWORD WINAPI ServiceMainCtrlThread(VOID* pParam)
{
    DWORD dwWaitResult;
    BOOL bContinue = TRUE;

    EventWriteInfoW(M, FL, FN, L"Service thread started.");

    // Start the event-based control code dispatch loop.
    while (bContinue)
    {
        dwWaitResult = WaitForMultipleObjects(EVENT_CTRL_COUNT, sc.hEvtMain, FALSE, INFINITE) - WAIT_OBJECT_0;

        if (dwWaitResult == EVENT_CTRL_PROCESS)
            EventWriteHexInfo(M, FL, FN, L"Control code", sc.dwCtrlCode);

        if (dwWaitResult == EVENT_CTRL_EXIT)
            bContinue = FALSE;
    }

    EventWriteInfoW(M, FL, FN, L"Service thread end.");

    return 0;
}

BOOL ServiceCreate(VOID)
{
    SECURITY_ATTRIBUTES secAttr;
    SECURITY_DESCRIPTOR sedDesc;
    // Exit event is named so we can terminate this service from outside.
    LPCTSTR	szEventName[] = { NULL, L"Global\\{BB7FA43F-2BAF-4020-B12A-6D96A040D755}", NULL, };
    InitializeSecurityDescriptor(&sedDesc, SECURITY_DESCRIPTOR_REVISION);

#pragma warning(suppress: 6248)
    SetSecurityDescriptorDacl(&sedDesc, TRUE, NULL, FALSE);
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = FALSE;
    secAttr.lpSecurityDescriptor = &sedDesc;

    for (int i = 0; i < EVENT_CTRL_COUNT; i++)
        sc.hEvtMain[i] = CreateEvent(&secAttr, FALSE, FALSE, szEventName[i]);

    sc.hMainThread = CreateThread(NULL, 0, ServiceMainCtrlThread, NULL, 0, NULL);

    if (!sc.hMainThread)
    {
        for (int i = 0; i < EVENT_CTRL_COUNT; i++)
            CloseHandle(sc.hEvtMain[i]);

        return FALSE;
    }

    return TRUE;
}

VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    EventRegisterJyTrace();

    sc.hStatusHandle = RegisterServiceCtrlHandlerEx(lpszArgv[0], ServiceHandlerEx, NULL);

    if (ServiceCreate())
    {
        NotifySCM(SERVICE_RUNNING, 0, 0);
        WaitForSingleObject(sc.hMainThread, INFINITE);
    }
    else
    {
        NotifySCM(SERVICE_STOP_PENDING, 0, 0);
    }

    ServiceCleanup();

    NotifySCM(SERVICE_STOPPED, 0, 0);

    EventUnregisterJyTrace();

    return;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { TEXT("win32basesvc"), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcher(ServiceTable);

    return 0;
}
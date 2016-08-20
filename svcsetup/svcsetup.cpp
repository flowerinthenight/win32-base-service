/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include "svcsetup.h"
#include <strsafe.h>
#include <PathCch.h>
#include "..\include\jytrace.h"

#pragma comment(lib, "pathcch.lib")

BOOL CmdServiceSetup(BOOL bUninstall)
{
    wchar_t *szName = L"win32basesvc";
    wchar_t *szDesc = L"A simple Win32 service.";
    BOOL bReturn = TRUE;

    if (bUninstall == FALSE)
    {
        SC_HANDLE schSCManager;
        SC_HANDLE schService;
        TCHAR szPath[MAX_PATH];
        wchar_t szPathSafe[MAX_PATH];

        if (!GetModuleFileName(NULL, szPath, MAX_PATH))
        {
            EventWriteLastError(M, FL, FN, L"GetModuleFileName", GetLastError());
            return FALSE;
        }

        PathCchRemoveFileSpec(szPath, MAX_PATH);
        StringCchPrintf(szPathSafe, MAX_PATH, L"\"%s\\win32-base-svc.exe\"", szPath);
        EventWriteWideStrInfo(M, FL, FN, L"PathSafe", szPathSafe);

        // Get a handle to the SCM database. 
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if (!schSCManager)
        {
            EventWriteLastError(M, FL, FN, L"OpenSCManager", GetLastError());
            return FALSE;
        }

        // Create the service
        schService = CreateService(
            schSCManager,               // SCM database
            szName,                     // name of service
            szName,                     // service name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_AUTO_START,         // start type
            SERVICE_ERROR_NORMAL,       // error control type
            szPathSafe,                 // path to service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            NULL,                       // no dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if (!schService)
        {
            CloseServiceHandle(schSCManager);
            EventWriteLastError(M, FL, FN, L"CreateService", GetLastError());
            return FALSE;
        }

        SERVICE_DESCRIPTION sd;
        sd.lpDescription = szDesc;

        if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd))
            EventWriteLastError(M, FL, FN, L"ChangeServiceConfig2", GetLastError());

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }
    else
    {
        SC_HANDLE schSCManager;
        SC_HANDLE schService;

        // Get a handle to the SCM database. 
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if (!schSCManager)
        {
            EventWriteLastError(M, FL, FN, L"OpenSCManager", GetLastError());
            return FALSE;
        }

        // Get a handle to the service.
        schService = OpenService(schSCManager, szName, DELETE);

        if (!schService)
        {
            EventWriteLastError(M, FL, FN, L"OpenService", GetLastError());
            return FALSE;
        }

        // Delete the service.
        if (!DeleteService(schService))
        {
            EventWriteLastError(M, FL, FN, L"DeleteService", GetLastError());
            bReturn = FALSE;
        }

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }

    return bReturn;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    EventRegisterJyTrace();

    TCHAR szCmd[MAX_PATH];

    StringCchPrintf(szCmd, MAX_PATH, TEXT("%s"), lpCmdLine);

    if (!_tcsicmp(szCmd, TEXT("install")))
    {
        EventWriteInfoW(M, FL, FN, L"Service install.");
        CmdServiceSetup(FALSE);
    }
    else if (!_tcsicmp(szCmd, TEXT("uninstall")))
    {
        EventWriteInfoW(M, FL, FN, L"Service uninstall.");
        CmdServiceSetup(TRUE);
    }
    else
    {
        EventWriteInfoW(M, FL, FN, L"Option not supported.");
    }

    EventUnregisterJyTrace();

    return 0;
}
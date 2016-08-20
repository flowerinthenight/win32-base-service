/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>

enum {
    EVENT_CTRL_PROCESS,
    EVENT_CTRL_EXIT,
    EVENT_CTRL_COUNT,
};

typedef struct _SVC_CONTEXT {
    SERVICE_STATUS_HANDLE hStatusHandle;
    HANDLE hMainThread;
    HANDLE hEvtMain[EVENT_CTRL_COUNT];
    DWORD dwCtrlCode;
} SVC_CONTEXT, *PSVC_CONTEXT;
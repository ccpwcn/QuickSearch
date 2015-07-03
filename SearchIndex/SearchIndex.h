#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#define SEARCH_INDEX_API	extern "C" __declspec(dllexport)

DWORD FAR WINAPI WorkThreadProc(LPVOID lpParam);

SEARCH_INDEX_API DWORD StartBuildIndex();

SEARCH_INDEX_API DWORD StopBuildIndex();
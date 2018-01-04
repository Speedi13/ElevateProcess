#pragma once

void GetDriverPath(wchar_t* pOut, DWORD dwOutSize, wchar_t* DriverName);

DWORD GetProcessIdFromName(char * ProcessName);

bool DoesFileExist( wchar_t* path );
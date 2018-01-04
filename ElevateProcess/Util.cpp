#include <Windows.h>
#include <TlHelp32.h>
#include "Util.h"

bool DoesFileExist(wchar_t* path)
{
	HANDLE hFile = CreateFileW(path,NULL, NULL, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	CloseHandle(hFile);
	return true;
}

void GetDriverPath(wchar_t* pOut, DWORD dwOutSize, wchar_t* DriverName)
{
	DWORD dwLen =
		GetModuleFileNameW(NULL, pOut, dwOutSize);
	for (size_t i = dwLen; i > 0; i--)
	{
		if (pOut[i] == '\\')
		{
			for (size_t j = 0; ; j++)
			{
				pOut[i + 1 + j] = DriverName[j];
				if (DriverName[j] == NULL)
					break;
			}
			break;
		}
	}
	return;
}

bool StrcmpToLower(char* string1, char* string2)
{
	for (size_t i = 0; ; i++)
	{
		if (tolower(string1[i]) != tolower(string2[i])) //make sure boths chars are the same
			return false;
		if (string1[i] == NULL) //check if they reached the end
			break;
	}
	return true;
}

DWORD GetProcessIdFromName(char * ProcessName)
{
	DWORD dwProcessId = NULL;

	HANDLE hHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hHandle == INVALID_HANDLE_VALUE || hHandle == NULL)
		return dwProcessId;

	PROCESSENTRY32 ProcessEntry = { 0 };
	RtlZeroMemory(&ProcessEntry, sizeof(PROCESSENTRY32));
	ProcessEntry.dwSize = sizeof(ProcessEntry);

	BOOL IsValidProcess = Process32First(hHandle, &ProcessEntry);
	do
	{
		if (StrcmpToLower(ProcessEntry.szExeFile, ProcessName) == true)
		{
			dwProcessId = ProcessEntry.th32ProcessID;
			break;
		}
	} while (IsValidProcess = Process32Next(hHandle, &ProcessEntry), 
		     IsValidProcess == TRUE);

	CloseHandle(hHandle);
	return dwProcessId;
}
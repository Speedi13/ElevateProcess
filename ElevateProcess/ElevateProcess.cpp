#include <windows.h>
#include <stdio.h>
#include <ntstatus.h>
#include "DriverMgr.h"
#include "Util.h"

//https://msdn.microsoft.com/en-us/library/6sehtctf.aspx
//  
// _WIN32_WINNT version constants  
//  
#define _WIN32_WINNT_NT4                    0x0400 // Windows NT 4.0  
#define _WIN32_WINNT_WIN2K                  0x0500 // Windows 2000  
#define _WIN32_WINNT_WINXP                  0x0501 // Windows XP  
#define _WIN32_WINNT_WS03                   0x0502 // Windows Server 2003  
#define _WIN32_WINNT_WIN6                   0x0600 // Windows Vista  
#define _WIN32_WINNT_VISTA                  0x0600 // Windows Vista  
#define _WIN32_WINNT_WS08                   0x0600 // Windows Server 2008  
#define _WIN32_WINNT_LONGHORN               0x0600 // Windows Vista  
#define _WIN32_WINNT_WIN7                   0x0601 // Windows 7  
#define _WIN32_WINNT_WIN8                   0x0602 // Windows 8  
#define _WIN32_WINNT_WINBLUE                0x0603 // Windows 8.1  
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 // Windows 10  
#define _WIN32_WINNT_WIN10                  0x0A00 // Windows 10  

#define SYSTEM_PID 4 //Is always the processid of the system process

//you can get them with WinDbg when Kernel Debugging
DWORD GetTokenOffset()
{
	typedef NTSTATUS(NTAPI *t_RtlGetVersion)( _Inout_	PRTL_OSVERSIONINFOW lpVersionInformation);
	static t_RtlGetVersion RtlGetVersion = NULL;
	if (RtlGetVersion == NULL)
	{ 
		HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
		RtlGetVersion = (t_RtlGetVersion)GetProcAddress(hNtDll, "RtlGetVersion");
	}
	OSVERSIONINFO OsVersionInfo;
	RtlZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));

	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (RtlGetVersion( (PRTL_OSVERSIONINFOW)&OsVersionInfo ) != STATUS_SUCCESS)
		return NULL;

	DWORD64 dwVersion = (OsVersionInfo.dwMajorVersion << 8) | OsVersionInfo.dwMinorVersion;
	switch (dwVersion)
	{
	case _WIN32_WINNT_WIN7:
		return 0x208;
		break;
	case _WIN32_WINNT_WIN8:		// Windows 8.0
	case _WIN32_WINNT_WINBLUE:	// Windows 8.1
		return 0x348;
		break;
	case _WIN32_WINNT_WIN10:// Windows 10  
		return 0x358;
		break;
	default:
		printf("[!] Error: Your windows version is not supported!\n");
		break;
	}
	return NULL;
}


typedef
struct _ElevateData
{
	unsigned __int32 dwSystemPid;
	unsigned __int32 dwElevatePid;
	unsigned __int64 dwTokenOffset;
} ElevateData;

#define IOCTL_ElevateProcess CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)

DWORD KernelElevate(	HANDLE hElevationDriver, 
						DWORD32 dwSystemPid, 
						DWORD32 dwTargetPid)
{
	DWORD dwReturnSize = NULL;
	BOOL bDeviceIoControl = FALSE;
	DWORD dwLastError = NULL;
	ElevateData Data = { 0 };
	Data.dwSystemPid = dwSystemPid;
	Data.dwElevatePid = dwTargetPid;
	Data.dwTokenOffset = GetTokenOffset();
	if (Data.dwTokenOffset == NULL)
		return ERROR_UNSUPPORTED_TYPE;

	bDeviceIoControl =
	DeviceIoControl(hElevationDriver,
					IOCTL_ElevateProcess,
					(void*)&Data, sizeof(ElevateData),
					(void*)NULL, NULL,
					&dwReturnSize,
					NULL);
	dwLastError = GetLastError();
	if (bDeviceIoControl == TRUE)
		return ERROR_SUCCESS;
	
	return dwLastError;
}

int main( int iArgCount, char** Args )
{
	if (iArgCount < 2)
	{
		printf("Elevate Process:\n");
		printf("Argument1: Process Name to Elevate\n\n");
		system("pause");
		return ERROR_SUCCESS;
	}

	char* pTargetProcessName = Args[1];
	
	DWORD dwTargetPid = GetProcessIdFromName(pTargetProcessName);
	if (dwTargetPid == NULL)
	{
		printf("[!] Couldn't find Process \"%s\"\n", pTargetProcessName);
		system("pause");
		return ERROR_SUCCESS;
	}
	DWORD dwResult = ERROR_SUCCESS;

	//check if the driver is loaded already
	HANDLE hElevationDriver = ConnectToDriver(L"\\\\.\\ElevationDriver"); 
	if (hElevationDriver == INVALID_HANDLE_VALUE || hElevationDriver == NULL)
	{//if not try loading it:
		wchar_t* szDriverPath = (wchar_t*)malloc(MAX_PATH * 2 + 2);
		GetDriverPath(szDriverPath, MAX_PATH, L"ElevateProcessDriver.sys");

		if (DoesFileExist(szDriverPath) != true)
		{
			printf("[!] Error: Driver not found, make sure to put the driver ( \"ElevateProcessDriver.sys\" ) in the same folder as this executable\n");
			system("pause");
			return ERROR_NOT_FOUND;
		}

		dwResult = CreateServiceX(szDriverPath, L"ElevateProcessDriver", L"ElevateProcessDriver", SERVICE_DEMAND_START);
		if (dwResult != ERROR_SUCCESS)
		{
			DeleteServiceX(L"ElevateProcessDriver");

			printf("[!] Error while CreatingService: 0x%X\n", dwResult);
			if (dwResult == ERROR_ACCESS_DENIED)
				printf("[!] Error: Make sure to run this program as Administrator\n");
			system("pause");
			return dwResult;
		}

		free(szDriverPath);
		dwResult = StartServiceX(L"ElevateProcessDriver");
		if (dwResult != ERROR_SUCCESS)
		{
			StopServiceX(L"ElevateProcessDriver");
			DeleteServiceX(L"ElevateProcessDriver");

			if (dwResult == ERROR_INVALID_IMAGE_HASH)
			{
				printf("[!] Error: Make sure you have \"Driver Signature Enforcement\" (short: DSE) disabled!\n");
				printf("[!]        You have to disable it every time you start your PC!\n\n");
			}
			else
			{
				printf("[!] Error while StartService: 0x%X\n", dwResult);
			}

			system("pause");
			return dwResult;
		}

		Sleep(1000);
		hElevationDriver = ConnectToDriver(L"\\\\.\\ElevationDriver");
		if (hElevationDriver == INVALID_HANDLE_VALUE || hElevationDriver == NULL)
		{
			printf("Failed to connect to driver!\n");
			system("pause");
			return ERROR_BAD_DEVICE;
		}
	}
	
	dwResult = KernelElevate(hElevationDriver, SYSTEM_PID, dwTargetPid);

	CloseHandle(hElevationDriver);

	StopServiceX(L"ElevateProcessDriver");
	DeleteServiceX(L"ElevateProcessDriver");
	
	if (dwResult != ERROR_SUCCESS)
	{
		printf("[!] Error while talking to the driver: 0x%X\n", dwResult);
		system("pause");
		return dwResult;
	}
	else
	{
		printf("[+] System privileges have been granted to the process %u (\"%s\")\n", dwTargetPid,pTargetProcessName);
	}

	system("pause");
    return ERROR_SUCCESS;
}


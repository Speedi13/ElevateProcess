#include <Windows.h>
#include "DriverMgr.h"

HANDLE ConnectToDriver(wchar_t* Device)
{
	HANDLE hDriverDevice = CreateFileW(Device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	return hDriverDevice;
}

DWORD CreateServiceX( wchar_t* FilePath, wchar_t* ServiceName, wchar_t* DisplayName, DWORD dwStartType )
{
	DWORD dwLastError = NULL;
	SetLastError(ERROR_SUCCESS);
	SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	dwLastError = GetLastError();
	if (hSCManager == NULL || hSCManager == INVALID_HANDLE_VALUE)
		return dwLastError;

	SC_HANDLE hService = CreateServiceW(hSCManager,
										ServiceName, 
										DisplayName,
										SERVICE_ALL_ACCESS,
										SERVICE_KERNEL_DRIVER,
										dwStartType,
										SERVICE_ERROR_NORMAL,
										FilePath,
										NULL, NULL, NULL, NULL, NULL
								   );
	dwLastError = GetLastError();
	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);
	if ( hService == NULL || hService == INVALID_HANDLE_VALUE)
	{
		if (dwLastError == ERROR_SERVICE_EXISTS)
			return ERROR_SUCCESS;
		return dwLastError;
	}
	return ERROR_SUCCESS;
}

DWORD StartServiceX( wchar_t* ServiceName)
{
	DWORD dwLastError = NULL;
	SetLastError(ERROR_SUCCESS);
	SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	dwLastError = GetLastError();
	if (hSCManager == NULL || hSCManager == INVALID_HANDLE_VALUE)
		return dwLastError;

	SC_HANDLE hService = OpenServiceW(hSCManager, ServiceName, SERVICE_ALL_ACCESS);
	dwLastError = GetLastError();
	if ( hService == NULL || hService == INVALID_HANDLE_VALUE)
	{
		CloseServiceHandle(hSCManager);
		return dwLastError;
	}

	BOOL bServiceStarted =
	StartServiceW(hService, NULL, NULL);
	dwLastError = GetLastError();

	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);

	if (bServiceStarted != TRUE)
		return dwLastError;

	return ERROR_SUCCESS;
}

DWORD StopServiceX(wchar_t* ServiceName)
{
	DWORD dwLastError = NULL;
	SetLastError(ERROR_SUCCESS);
	SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	dwLastError = GetLastError();
	if (hSCManager == NULL || hSCManager == INVALID_HANDLE_VALUE)
		return dwLastError;

	SC_HANDLE hService = OpenServiceW(hSCManager, ServiceName, SERVICE_ALL_ACCESS);
	dwLastError = GetLastError();
	if (hService == NULL || hService == INVALID_HANDLE_VALUE)
	{
		CloseServiceHandle(hSCManager);
		return dwLastError;
	}

	SERVICE_STATUS ServiceStatus;
	RtlZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));

	BOOL bServiceStopped =
	ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
	dwLastError = GetLastError();

	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);

	if (bServiceStopped != TRUE)
		return dwLastError;

	return ERROR_SUCCESS;
}

DWORD DeleteServiceX(wchar_t* ServiceName)
{
	DWORD dwLastError = NULL;
	SetLastError(ERROR_SUCCESS);
	SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	dwLastError = GetLastError();
	if (hSCManager == NULL || hSCManager == INVALID_HANDLE_VALUE)
		return dwLastError;

	SC_HANDLE hService = OpenServiceW(hSCManager, ServiceName, SERVICE_ALL_ACCESS);
	dwLastError = GetLastError();
	if (hService == NULL || hService == INVALID_HANDLE_VALUE)
	{
		CloseServiceHandle(hSCManager);
		return dwLastError;
	}

	SERVICE_STATUS ServiceStatus;
	RtlZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));

	BOOL bServiceDeleted =
	DeleteService(hService);
	dwLastError = GetLastError();

	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);

	if (bServiceDeleted != TRUE)
		return dwLastError;

	return ERROR_SUCCESS;
}
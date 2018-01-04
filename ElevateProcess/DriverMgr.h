#pragma once

HANDLE ConnectToDriver(wchar_t* Device);

DWORD CreateServiceX(wchar_t* FilePath, wchar_t* ServiceName, wchar_t* DisplayName, DWORD dwStartType = SERVICE_DEMAND_START);
DWORD StartServiceX(wchar_t* ServiceName);
DWORD StopServiceX(wchar_t* ServiceName);
DWORD DeleteServiceX(wchar_t* ServiceName);
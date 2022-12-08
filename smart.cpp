
#include <winioctl.h>

/***********************************************************************************************/

#include <pshpack1.h>
typedef struct  _DRIVEATTRIBUTEHDR
{
    WORD    wRevision;      // Attribute structure revision
    BYTE    bData[1];       // Data
} DRIVEATTRIBUTEHDR, * LPDRIVEATTRIBUTEHDR;

typedef struct  _DRIVEATTRIBUTE
{
    BYTE    bAttrID;        // Identifies which attribute
    WORD    wStatusFlags;   // see bit definitions below
    BYTE    bAttrValue;     // Current normalized value
    BYTE    bWorstValue;    // How bad has it ever been?
    BYTE    bRawValue[6];   // Un-normalized value
    BYTE    bReserved;      // ...
} DRIVEATTRIBUTE, * PDRIVEATTRIBUTE, * LPDRIVEATTRIBUTE;
#include <poppack.h>

/***********************************************************************************************/

#define NUM_ATTRIBUTE_STRUCTS        30

/***********************************************************************************************/

#define ATTR_TEMPERATURE    194

/***********************************************************************************************/

BOOL GetHDDTemp(DWORD iDrive, __int64* pRawTemp)
{
    if (!pRawTemp)
        return FALSE;

    TCHAR szDrive[MAX_PATH] = { 0 };
    wsprintf(szDrive, TEXT("\\\\.\\PhysicalDrive%i"), iDrive);

    HANDLE hDrive = CreateFile(szDrive, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDrive == INVALID_HANDLE_VALUE)
        return FALSE;

    SENDCMDINPARAMS cmdIn = { 0 };
    cmdIn.cBufferSize = 0;
    cmdIn.irDriveRegs.bCommandReg = SMART_CMD;
    cmdIn.irDriveRegs.bFeaturesReg = ENABLE_SMART;
    cmdIn.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
    cmdIn.irDriveRegs.bCylHighReg = SMART_CYL_HI;
    cmdIn.irDriveRegs.bSectorCountReg = 1;
    cmdIn.irDriveRegs.bSectorNumberReg = 1;
    cmdIn.irDriveRegs.bDriveHeadReg = 0xA0 | (((BYTE)iDrive & 1) << 4);

    DWORD dwReturned = 0;
    BYTE bOut[sizeof(SENDCMDOUTPARAMS) - 1 + READ_ATTRIBUTE_BUFFER_SIZE] = { 0 };
    if (!DeviceIoControl(hDrive, SMART_SEND_DRIVE_COMMAND, &cmdIn, sizeof(cmdIn), bOut, sizeof(bOut), &dwReturned, NULL))
    {
        CloseHandle(hDrive); hDrive = NULL;
        return FALSE;
    }

    cmdIn.cBufferSize = READ_ATTRIBUTE_BUFFER_SIZE;
    cmdIn.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;

    ZeroMemory(bOut, sizeof(bOut));
    if (!DeviceIoControl(hDrive, SMART_RCV_DRIVE_DATA, &cmdIn, sizeof(cmdIn), bOut, sizeof(bOut), &dwReturned, NULL))
    {
        CloseHandle(hDrive); hDrive = NULL;
        return FALSE;
    }

    BOOL fResult = FALSE;
    LPDRIVEATTRIBUTEHDR lpAttrHdr = (LPDRIVEATTRIBUTEHDR)(((LPSENDCMDOUTPARAMS)bOut)->bBuffer);
    LPDRIVEATTRIBUTE lpAttr = (LPDRIVEATTRIBUTE)(lpAttrHdr->bData);
    for (int iAttr = 0; iAttr < NUM_ATTRIBUTE_STRUCTS; iAttr++)
    {
        if (lpAttr[iAttr].bAttrID == ATTR_TEMPERATURE)
        {
            *pRawTemp = 0;
            CopyMemory((void*)pRawTemp, (void*)lpAttr[iAttr].bRawValue, sizeof(lpAttr[iAttr].bRawValue));
            fResult = TRUE;
            break;
        }
    }

    CloseHandle(hDrive); hDrive = NULL;
    return fResult;
}

/************************************************************************************************/
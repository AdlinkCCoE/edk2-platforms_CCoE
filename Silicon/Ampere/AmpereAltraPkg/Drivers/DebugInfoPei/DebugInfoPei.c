/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/HobLib.h>
#include <Library/NVParamLib.h>
#include <Library/SMProLib.h>
#include <Library/AmpereCpuLib.h>
#include <PlatformInfoHob.h>
#include <NVParamDef.h>
#include <Pcie.h>

#define MAX_PRINT_LEN       512

#define GB_SCALE_FACTOR     1073741824
#define MB_SCALE_FACTOR     1048576
#define KB_SCALE_FACTOR     1024
#define MHZ_SCALE_FACTOR    1000000

STATIC VOID
SerialPrint (
  IN  CONST CHAR8  *FormatString,
  ...
  )
{
  CHAR8     Buf[MAX_PRINT_LEN];
  VA_LIST   Marker;
  UINTN     NumberOfPrinted;

  VA_START (Marker, FormatString);
  NumberOfPrinted = AsciiVSPrint (Buf, sizeof (Buf), FormatString, Marker);
  SerialPortWrite((UINT8 *) Buf, NumberOfPrinted);
  VA_END (Marker);
}

/**
  Print any existence NVRAM.
**/
STATIC VOID
PrintNVRAM (
  VOID
  )
{
  NVPARAM   Idx;
  UINT32    Val;
  UINT16    ACLRd = NV_PERM_ALL;
  BOOLEAN   Flag;

  Flag = FALSE;
  for (Idx = NV_PREBOOT_PARAM_START; Idx <= NV_PREBOOT_PARAM_MAX; Idx+=NVPARAM_SIZE) {
    if (!NVParamGet (Idx, ACLRd, &Val)) {
      if (!Flag) {
        SerialPrint ("Pre-boot Configuration Setting:\n");
        Flag = TRUE;
      }
      SerialPrint ("    %04X: 0x%X (%d)\n", (UINT32) Idx, Val, Val);
    }
  }
  Flag = FALSE;
  for (Idx = NV_MANU_PARAM_START; Idx <= NV_MANU_PARAM_MAX; Idx+=NVPARAM_SIZE) {
    if (!NVParamGet (Idx, ACLRd, &Val)) {
      if (!Flag) {
        SerialPrint ("Manufacturer Configuration Setting:\n");
        Flag = TRUE;
      }
      SerialPrint ("    %04X: 0x%X (%d)\n", (UINT32) Idx, Val, Val);
    }
  }
  Flag = FALSE;
  for (Idx = NV_USER_PARAM_START; Idx <= NV_USER_PARAM_MAX; Idx+=NVPARAM_SIZE) {
    if (!NVParamGet (Idx, ACLRd, &Val)) {
      if (!Flag) {
        SerialPrint ("User Configuration Setting:\n");
        Flag = TRUE;
      }
      SerialPrint ("    %04X: 0x%X (%d)\n", (UINT32) Idx, Val, Val);
    }
  }
  Flag = FALSE;
  for (Idx = NV_BOARD_PARAM_START; Idx <= NV_BOARD_PARAM_MAX; Idx+=NVPARAM_SIZE) {
    if (!NVParamGet (Idx, ACLRd, &Val)) {
      if (!Flag) {
        SerialPrint ("Board Configuration Setting:\n");
        Flag = TRUE;
      }
      SerialPrint ("    %04X: 0x%X (%d)\n", (UINT32) Idx, Val, Val);
    }
  }
}

STATIC
UINT32
GetCCIXLinkWidth (
  IN UINTN Socket
  )
{
  UINT64    PcieTcuBase[] = {AC01_PCIE_REGISTER_BASE};
  UINT64    LinkStsReg = ((Socket == 0) ? PcieTcuBase[0] : PcieTcuBase[1]) + 0x10008080;

  return ((MmioRead32 (LinkStsReg) >> 20) & 0x3F);
}

STATIC
CHAR8*
GetCCIXLinkSpeed (
  IN UINTN Socket
  )
{
  UINT64    PcieTcuBase[] = {AC01_PCIE_REGISTER_BASE};
  UINT64    LinkStsReg = ((Socket == 0) ? PcieTcuBase[0] : PcieTcuBase[1]) + 0x10008080;
  UINT64    EsmStatReg = ((Socket == 0) ? PcieTcuBase[0] : PcieTcuBase[1]) + 0x100083a0;

  if (MmioRead32 (EsmStatReg) & 0x80) {
    /* ESM_CALIB_CMPLT is set */
    switch (MmioRead32 (EsmStatReg) & 0x7f) {
    case 1:
      return "2.5 GT/s";
    case 2:
      return "5 GT/s";
    case 3:
      return "8 GT/s";
    case 6:
      return "16 GT/s";
    case 0xa:
      return "20 GT/s";
    case 0xf:
      return "25 GT/s";
    }
  } else {
    switch ((MmioRead32 (LinkStsReg) >> 16) & 0xF) {
    case 1:
      return "2.5 GT/s";
    case 2:
      return "5 GT/s";
    case 3:
      return "8 GT/s";
    case 4:
      return "16 GT/s";
    case 5:
      return "32 GT/s";
    }
  }

  return "Unknown";
}

/**
  Print system info
**/
STATIC VOID
PrintSystemInfo (
  VOID
  )
{
  UINTN               Idx;
  VOID                *Hob;
  PlatformInfoHob_V2  *PlatformHob;
  CONST EFI_GUID      PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;

  Hob = GetFirstGuidHob (&PlatformHobGuid);
  if (!Hob) {
    return;
  }

  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);

  SerialPrint ("SCP FW version    : %a\n", (const CHAR8 *) PlatformHob->SmPmProVer);
  SerialPrint ("SCP FW build date : %a\n", (const CHAR8 *) PlatformHob->SmPmProBuild);

  SerialPrint ("Failsafe status                 : %d\n", PlatformHob->FailSafeStatus);
  SerialPrint ("Reset status                    : %d\n", PlatformHob->ResetStatus);
  SerialPrint ("CPU info\n");
  SerialPrint ("    CPU ID                      : %X\n", ArmReadMidr ());
  SerialPrint ("    CPU Clock                   : %d MHz\n", PlatformHob->CpuClk / MHZ_SCALE_FACTOR);
  SerialPrint ("    Number of active sockets    : %d\n", GetNumberActiveSockets ());
  SerialPrint ("    Number of active cores      : %d\n", GetNumberActiveCores ());
  if (GetNumberActiveSockets () > 1) {
    SerialPrint ("    Inter Socket Connection 0   : Width: x%d / Speed %a\n",
            GetCCIXLinkWidth (0), GetCCIXLinkSpeed (0));
    SerialPrint ("    Inter Socket Connection 1   : Width: x%d / Speed %a\n",
            GetCCIXLinkWidth (1), GetCCIXLinkSpeed (1));
  }
  for (Idx = 0; Idx < GetNumberActiveSockets (); Idx ++) {
    SerialPrint ("    Socket[%d]: Core voltage     : %d\n", Idx, PlatformHob->CoreVoltage[Idx]);
    SerialPrint ("    Socket[%d]: SCU ProductID    : %X\n", Idx, PlatformHob->ScuProductId[Idx]);
    SerialPrint ("    Socket[%d]: Max cores        : %d\n", Idx, PlatformHob->MaxNumOfCore[Idx]);
    SerialPrint ("    Socket[%d]: Warranty         : %d\n", Idx, PlatformHob->Warranty[Idx]);
    SerialPrint ("    Socket[%d]: Subnuma          : %d\n", Idx, PlatformHob->SubNumaMode[Idx]);
    SerialPrint ("    Socket[%d]: RC disable mask  : %X\n", Idx, PlatformHob->RcDisableMask[Idx]);
    SerialPrint ("    Socket[%d]: AVS enabled      : %d\n", Idx, PlatformHob->AvsEnable[Idx]);
    SerialPrint ("    Socket[%d]: AVS voltage      : %d\n", Idx, PlatformHob->AvsVoltageMV[Idx]);
  }

  SerialPrint ("SOC info\n");
  SerialPrint ("    DDR Frequency               : %d MHz\n", PlatformHob->DramInfo.MaxSpeed);
  for (Idx = 0; Idx < GetNumberActiveSockets (); Idx ++) {
    SerialPrint ("    Socket[%d]: Soc voltage      : %d\n", Idx, PlatformHob->SocVoltage[Idx]);
    SerialPrint ("    Socket[%d]: DIMM1 voltage    : %d\n", Idx, PlatformHob->Dimm1Voltage[Idx]);
    SerialPrint ("    Socket[%d]: DIMM2 voltage    : %d\n", Idx, PlatformHob->Dimm2Voltage[Idx]);
  }

  SerialPrint ("    PCP Clock                   : %d MHz\n", PlatformHob->PcpClk / MHZ_SCALE_FACTOR);
  SerialPrint ("    SOC Clock                   : %d MHz\n", PlatformHob->SocClk / MHZ_SCALE_FACTOR);
  SerialPrint ("    SYS Clock                   : %d MHz\n", PlatformHob->SysClk / MHZ_SCALE_FACTOR);
  SerialPrint ("    AHB Clock                   : %d MHz\n", PlatformHob->AhbClk / MHZ_SCALE_FACTOR);
}

/**
  Entry point function for the PEIM

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS    If we installed our PPI

**/
EFI_STATUS
EFIAPI
DebugInfoPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PrintSystemInfo ();
  PrintNVRAM ();

  return EFI_SUCCESS;
}

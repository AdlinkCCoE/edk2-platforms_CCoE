/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

#define SUBNUMA_MODE_MONOLITHIC        0
#define SUBNUMA_MODE_HEMISPHERE        1
#define SUBNUMA_MODE_QUADRANT          2

STATIC VOID
AcpiPatchCmn600 (VOID)
{
  CHAR8     NodePath[MAX_ACPI_NODE_PATH];
  UINTN     Index;

  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.CMN%1X._STA", Index);
    if (GetNumberActiveCPMsPerSocket (Index)) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

STATIC VOID
AcpiPatchDmc620 (VOID)
{
  CHAR8                 NodePath[MAX_ACPI_NODE_PATH];
  UINTN                 Index, Index1;
  EFI_GUID              PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;
  PlatformInfoHob_V2    *PlatformHob;
  UINT32                McuMask;
  VOID                  *Hob;

  Hob = GetFirstGuidHob (&PlatformHobGuid);
  if (!Hob) {
    return;
  }
  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);

  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    McuMask = PlatformHob->DramInfo.McuMask[Index];
    for (Index1 = 0; Index1 < sizeof (McuMask) * 8; Index1++) {
      AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.MC%1X%1X._STA", Index, Index1);
      if (McuMask & (0x1 << Index1)) {
        AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
      } else {
        AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
      }
    }
  }
}

STATIC VOID
AcpiPatchNvdimm (VOID)
{
  CHAR8                 NodePath[MAX_ACPI_NODE_PATH];
  UINTN                 NvdRegionNum, Count;
  EFI_GUID              PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;
  PlatformInfoHob_V2    *PlatformHob;
  VOID                  *Hob;

  Hob = GetFirstGuidHob (&PlatformHobGuid);
  if (!Hob) {
    return;
  }
  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);

  NvdRegionNum = 0;
  for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
    if (PlatformHob->DramInfo.NvdRegion[Count]) {
      NvdRegionNum += 1;
    }
  }

  if (NvdRegionNum == PLATFORM_MAX_NUM_NVDIMM_DEVICE) {
    return;
  }

  /* Disable NVDIMM Root Device */
  if (NvdRegionNum == 0) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.NVDR._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
  }

  /* Disable NVDIMM Device which is not available */
  Count = NvdRegionNum + 1;
  while (Count <= PLATFORM_MAX_NUM_NVDIMM_DEVICE) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.NVDR.NVD%1X._STA", Count);
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    Count++;
  }
}

STATIC VOID
AcpiPatchHwmon (VOID)
{
  CHAR8     NodePath[MAX_ACPI_NODE_PATH];
  UINTN     Index;

  // PCC Hardware Monitor Devices
  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.HM0%1X._STA", Index);
    if (GetNumberActiveCPMsPerSocket (Index)) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }

  // Ampere Altra SoC Hardware Monitor Devices
  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.HM0%1X._STA", Index + 2);
    if (GetNumberActiveCPMsPerSocket (Index)) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

STATIC VOID
AcpiPatchDsu (VOID)
{
  CHAR8                 NodePath[MAX_ACPI_NODE_PATH];
  UINTN                 Index;

  /*
   * The following code may take very long time in emulator. Just leave all Dsu enabled in
   * emulation mode as they only associate with enabled core in OS
   */
#if Emag2Emulator_SUPPORT
  return;
#endif

  for (Index = 0; Index < PLATFORM_CPU_MAX_NUM_CORES; Index += PLATFORM_CPU_NUM_CORES_PER_CPM) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.DU%2X._STA", Index / PLATFORM_CPU_NUM_CORES_PER_CPM);
    if (IsCpuEnabled (Index)) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

STATIC UINT8
PcieGetSubNumaMode (
  VOID
  )
{
  EFI_GUID                    PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;
  PlatformInfoHob_V2          *PlatformHob;
  VOID                        *Hob;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&PlatformHobGuid);
  if (!Hob) {
    return SUBNUMA_MODE_MONOLITHIC;
  }
  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);

  return PlatformHob->SubNumaMode[0];
}

VOID
AcpiPatchPcieNuma (
  VOID
  )
{
  CHAR8     NodePath[MAX_ACPI_NODE_PATH];
  UINTN     Index;
  UINTN     NumaIdx;
  UINTN     NumPciePort;
  UINTN     NumaAssignment[3][16] = {
              { 0, 0, 0, 0, 0, 0, 0, 0,   // Monolitic Node 0 (S0)
                1, 1, 1, 1, 1, 1, 1, 1 }, // Monolitic Node 1 (S1)
              { 0, 1, 0, 1, 0, 0, 1, 1,   // Hemisphere Node 0, 1 (S0)
                2, 3, 2, 3, 2, 2, 3, 3 }, // Hemisphere Node 2, 3 (S1)
              { 0, 2, 1, 3, 1, 1, 3, 3,   // Quadrant Node 0, 1, 2, 3 (S0)
                4, 6, 5, 7, 5, 5, 7, 7 }, // Quadrant Node 4, 5, 6, 7 (S1)
              };

  switch (PcieGetSubNumaMode()) {
  case SUBNUMA_MODE_MONOLITHIC:
  default:
    NumaIdx = 0;
    break;
  case SUBNUMA_MODE_HEMISPHERE:
    NumaIdx = 1;
    break;
  case SUBNUMA_MODE_QUADRANT:
    NumaIdx = 2;
    break;
  }

  if (GetNumberActiveSockets() > 1) {
    NumPciePort = 16; // 16 ports total (8 per socket)
  } else {
    NumPciePort = 8;  // 8 ports total
  }

  for (Index = 0; Index < NumPciePort; Index++) {
    AsciiSPrint (NodePath, sizeof(NodePath), "\\_SB.PCI%X._PXM", Index);
    AcpiDSDTSetNodeStatusValue (NodePath, NumaAssignment[NumaIdx][Index]);
  }
}

EFI_STATUS
AcpiPatchDsdtTable (VOID)
{
  AcpiPatchCmn600 ();
  AcpiPatchDmc620 ();
  AcpiPatchDsu ();
  AcpiPatchHwmon ();
  AcpiPatchNvdimm ();
  AcpiPatchPcieNuma ();

  return EFI_SUCCESS;
}

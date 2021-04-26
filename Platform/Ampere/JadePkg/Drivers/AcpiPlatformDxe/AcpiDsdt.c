/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/PciRootBridgeIo.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>

#include "AcpiNfit.h"
#include "AcpiPlatform.h"

#define PCIE_DEVICE_CONTROL_OFFSET                      0x078
#define PCIE_DEVICE_CONTROL_UNSUPPORT_REQ_REP_EN        0x08
#define PCIE_DEVICE_CONTROL_FATAL_ERR_REPORT_EN         0x04
#define PCIE_DEVICE_CONTROL_NON_FATAL_ERR_REPORT_EN     0x02
#define PCIE_DEVICE_CONTROL_CORR_ERR_REPORT_EN          0x01

#define PCIE_ROOT_ERR_CMD_OFFSET                        0x12C
#define PCIE_ROOT_ERR_CMD_FATAL_ERR_REPORTING_EN        0x4
#define PCIE_ROOT_ERR_CMD_NON_FATAL_ERR_REPORTING_EN    0x2
#define PCIE_ROOT_ERR_CMD_CORR_ERR_REPORTING_EN         0x1

#define PCIE_MAX_DEVICE_PER_ROOT_PORT 8

STATIC VOID
AcpiPatchCmn600 (
  VOID
  )
{
  CHAR8 NodePath[MAX_ACPI_NODE_PATH];
  UINTN Index;

  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.CMN%1X._STA", Index);
    if (GetNumberOfActiveCPMsPerSocket (Index) > 0) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

STATIC VOID
AcpiPatchDmc620 (
  VOID
  )
{
  CHAR8              NodePath[MAX_ACPI_NODE_PATH];
  UINTN              Index, Index1;
  PLATFORM_INFO_HOB  *PlatformHob;
  UINT32             McuMask;
  VOID               *Hob;

  Hob = GetFirstGuidHob (&gPlatformHobGuid);
  if (Hob == NULL) {
    return;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    McuMask = PlatformHob->DramInfo.McuMask[Index];
    for (Index1 = 0; Index1 < sizeof (McuMask) * 8; Index1++) {
      AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.MC%1X%1X._STA", Index, Index1);
      if (McuMask & (0x1 << Index1)) {
        AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
      } else {
        AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
      }
    }
  }
}

STATIC VOID
AcpiPatchNvdimm (
  VOID
  )
{
  CHAR8              NodePath[MAX_ACPI_NODE_PATH];
  UINTN              NvdRegionNumSK0, NvdRegionNumSK1, NvdRegionNum, Count;
  PLATFORM_INFO_HOB  *PlatformHob;
  VOID               *Hob;

  Hob = GetFirstGuidHob (&gPlatformHobGuid);
  if (Hob == NULL) {
    return;
  }
  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  NvdRegionNumSK0 = 0;
  NvdRegionNumSK1 = 0;
  for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
    if (PlatformHob->DramInfo.NvdRegion[Count] > 0) {
      if (PlatformHob->DramInfo.Socket[Count] == 0) {
        NvdRegionNumSK0++;
      } else {
        NvdRegionNumSK1++;
      }
    }
  }
  NvdRegionNum = NvdRegionNumSK0 + NvdRegionNumSK1;

  /* Disable NVDIMM Root Device */
  if (NvdRegionNum == 0) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
  }
  /* Update NVDIMM Device _STA for SK0 */
  if (NvdRegionNumSK0 == 0) {
    /* Disable NVD1/2 */
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD1._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD2._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
  } else if (NvdRegionNumSK0 == 1) {
    if (PlatformHob->DramInfo.NvdimmMode[NVDIMM_SK0] == NVDIMM_NON_HASHED) {
      for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
        if (PlatformHob->DramInfo.NvdRegion[Count] > 0 &&
            PlatformHob->DramInfo.Socket[Count] == 0)
        {
          if (PlatformHob->DramInfo.Base[Count] ==
              PLATFORM_NVDIMM_SK0_NHASHED_REGION0)
          {
            /* Disable NVD2 */
            AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD2._STA");
            AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
          } else if (PlatformHob->DramInfo.Base[Count] ==
                     PLATFORM_NVDIMM_SK0_NHASHED_REGION1)
          {
            /* Disable NVD1 */
            AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD1._STA");
            AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
          }
        }
      }
    }
  }
  /* Update NVDIMM Device _STA for SK1 */
  if (NvdRegionNumSK1 == 0) {
    /* Disable NVD3/4 */
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD3._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD4._STA");
    AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
  } else if (NvdRegionNumSK1 == 1) {
    if (PlatformHob->DramInfo.NvdimmMode[NVDIMM_SK1] == NVDIMM_NON_HASHED) {
      for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
        if (PlatformHob->DramInfo.NvdRegion[Count] > 0 &&
            PlatformHob->DramInfo.Socket[Count] == 1)
        {
          if (PlatformHob->DramInfo.Base[Count] ==
              PLATFORM_NVDIMM_SK1_NHASHED_REGION0)
          {
            /* Disable NVD4 */
            AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD4._STA");
            AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
          } else if (PlatformHob->DramInfo.Base[Count] ==
                     PLATFORM_NVDIMM_SK1_NHASHED_REGION1)
          {
            /* Disable NVD3 */
            AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.NVDR.NVD3._STA");
            AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
          }
        }
      }
    }
  }
}

STATIC VOID
AcpiPatchHwmon (
  VOID
  )
{
  CHAR8 NodePath[MAX_ACPI_NODE_PATH];
  UINT8 Index;

  // PCC Hardware Monitor Devices
  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.HM0%1X._STA", Index);
    if (GetNumberOfActiveCPMsPerSocket (Index) > 0) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }

  // Ampere Altra SoC Hardware Monitor Devices
  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.HM0%1X._STA", Index + 2);
    if (GetNumberOfActiveCPMsPerSocket (Index) > 0) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

STATIC VOID
AcpiPatchDsu (
  VOID
  )
{
  CHAR8 NodePath[MAX_ACPI_NODE_PATH];
  UINTN Index;

  for (Index = 0; Index < PLATFORM_CPU_MAX_NUM_CORES; Index += PLATFORM_CPU_NUM_CORES_PER_CPM) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.DU%2X._STA", Index / PLATFORM_CPU_NUM_CORES_PER_CPM);
    if (IsCpuEnabled (Index)) {
      AcpiDSDTSetNodeStatusValue (NodePath, 0xf);
    } else {
      AcpiDSDTSetNodeStatusValue (NodePath, 0x0);
    }
  }
}

VOID
AcpiPatchPcieNuma (
  VOID
  )
{
  CHAR8 NodePath[MAX_ACPI_NODE_PATH];
  UINTN Index;
  UINTN NumaIdx;
  UINTN NumPciePort;
  UINTN NumaAssignment[3][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0,   // Monolithic Node 0 (S0)
      1, 1, 1, 1, 1, 1, 1, 1 }, // Monolithic Node 1 (S1)
    { 0, 1, 0, 1, 0, 0, 1, 1,   // Hemisphere Node 0, 1 (S0)
      2, 3, 2, 3, 2, 2, 3, 3 }, // Hemisphere Node 2, 3 (S1)
    { 0, 2, 1, 3, 1, 1, 3, 3,   // Quadrant Node 0, 1, 2, 3 (S0)
      4, 6, 5, 7, 5, 5, 7, 7 }, // Quadrant Node 4, 5, 6, 7 (S1)
  };

  switch (CpuGetSubNumaMode ()) {
  case SUBNUMA_MODE_MONOLITHIC:
    NumaIdx = 0;
    break;

  case SUBNUMA_MODE_HEMISPHERE:
    NumaIdx = 1;
    break;

  case SUBNUMA_MODE_QUADRANT:
    NumaIdx = 2;
    break;

  default:
    NumaIdx = 0;
    break;
  }

  if (IsSlaveSocketActive ()) {
    NumPciePort = 16; // 16 ports total (8 per socket)
  } else {
    NumPciePort = 8;  // 8 ports total
  }

  for (Index = 0; Index < NumPciePort; Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.PCI%X._PXM", Index);
    AcpiDSDTSetNodeStatusValue (NodePath, NumaAssignment[NumaIdx][Index]);
  }
}

EFI_STATUS
AcpiPatchPcieAerFwFirst (
  VOID
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS Address;
  EFI_ACPI_SDT_PROTOCOL                       *AcpiTableProtocol;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL             *PciRootBridgeIo;
  EFI_HANDLE                                  *HandleBuffer;
  UINTN                                       HandleCount;
  EFI_ACPI_HANDLE                             TableHandle;
  EFI_ACPI_HANDLE                             ChildHandle;
  EFI_ACPI_DATA_TYPE                          DataType;
  UINTN                                       DataSize;
  CHAR8                                       ObjectPath[8];
  EFI_STATUS                                  Status;
  UINT32                                      AerFwFirstConfigValue;
  UINT32                                      RegData;
  UINT16                                      Device;
  UINT32                                      Index;
  UINT8                                       *Data;

  //
  // Check if PCIe AER Firmware First should be enabled
  //
  Status = NVParamGet (
             NV_SI_RAS_PCIE_AER_FW_FIRST,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &AerFwFirstConfigValue
             );
  if (EFI_ERROR (Status)) {
    Status = NVParamGet (
               NV_SI_RO_BOARD_PCIE_AER_FW_FIRST,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &AerFwFirstConfigValue
               );
    if (EFI_ERROR (Status)) {
      AerFwFirstConfigValue = 0;
    }
  }

  if (AerFwFirstConfigValue == 0) {
    //
    // By default, the PCIe AER FW-First (ACPI Object "AERF") is set to 0
    // in the DSDT table.
    //
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol\n"));
    return Status;
  }

  Status = AcpiOpenDSDT (AcpiTableProtocol, &TableHandle);
  if (EFI_ERROR (Status)) {
    AcpiTableProtocol->Close (TableHandle);
    return Status;
  }

  //
  // Update Name Object "AERF" (PCIe AER Firmware-First) if it is enabled.
  //
  AsciiSPrint (ObjectPath, sizeof (ObjectPath), "\\AERF");
  Status = AcpiTableProtocol->FindPath (TableHandle, ObjectPath, &ChildHandle);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    Status = AcpiTableProtocol->GetOption (
                                  ChildHandle,
                                  0,
                                  &DataType,
                                  (VOID *)&Data,
                                  &DataSize
                                  );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)
        && Data[0] == AML_NAME_OP
        && (Data[5] == AML_ZERO_OP || Data[5] == AML_ONE_OP))
    {
      Data[5] = 1; // Enable PCIe AER Firmware-First
    }
  }

  AcpiTableProtocol->Close (TableHandle);
  AcpiDSDTUpdateChecksum (AcpiTableProtocol);

  //
  // For PCIe AER Firmware First, PCIe capability registers need
  // to be updated to allow Firmware to detect AER errors.
  //

  HandleCount = 0;
  HandleBuffer = NULL;
  PciRootBridgeIo = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop through each root complex
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID **)&PciRootBridgeIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Loop through each root port
    //
    for (Device = 1; Device <= PCIE_MAX_DEVICE_PER_ROOT_PORT; Device++) {
      Address.Bus = 0;
      Address.Device = Device;
      Address.Function = 0;
      Address.Register = 0;

      Address.ExtendedRegister = PCIE_DEVICE_CONTROL_OFFSET;
      PciRootBridgeIo->Pci.Read (PciRootBridgeIo, EfiPciWidthUint32, *((UINT64 *)&Address), 1, &RegData);

      if (RegData == 0xFFFFFFFF) {
        continue;
      }

      RegData |= PCIE_DEVICE_CONTROL_UNSUPPORT_REQ_REP_EN
                 | PCIE_DEVICE_CONTROL_FATAL_ERR_REPORT_EN
                 | PCIE_DEVICE_CONTROL_NON_FATAL_ERR_REPORT_EN
                 | PCIE_DEVICE_CONTROL_CORR_ERR_REPORT_EN;

      PciRootBridgeIo->Pci.Write (PciRootBridgeIo, EfiPciWidthUint32, *((UINT64 *)&Address), 1, &RegData);

      RegData = 0;
      Address.ExtendedRegister = PCIE_ROOT_ERR_CMD_OFFSET;
      PciRootBridgeIo->Pci.Read (PciRootBridgeIo, EfiPciWidthUint32, *((UINT64 *)&Address), 1, &RegData);

      RegData |= PCIE_ROOT_ERR_CMD_FATAL_ERR_REPORTING_EN
                 | PCIE_ROOT_ERR_CMD_NON_FATAL_ERR_REPORTING_EN
                 | PCIE_ROOT_ERR_CMD_CORR_ERR_REPORTING_EN;

      PciRootBridgeIo->Pci.Write (PciRootBridgeIo, EfiPciWidthUint32, *((UINT64 *)&Address), 1, &RegData);
    }
  }

  return Status;
}

EFI_STATUS
AcpiPatchDsdtTable (
  VOID
  )
{
  AcpiPatchCmn600 ();
  AcpiPatchDmc620 ();
  AcpiPatchDsu ();
  AcpiPatchHwmon ();
  AcpiPatchNvdimm ();
  AcpiPatchPcieNuma ();
  AcpiPatchPcieAerFwFirst ();

  return EFI_SUCCESS;
}

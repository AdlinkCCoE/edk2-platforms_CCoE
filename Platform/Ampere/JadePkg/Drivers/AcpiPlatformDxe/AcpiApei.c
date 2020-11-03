/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiApei.h"

#ifdef DEBUG
#define DBG(arg...)  DEBUG((DEBUG_VERBOSE, ## arg))
#else
#define DBG(arg...)
#endif

STATIC VOID
AcpiApeiUninstallTable (UINT32 Signature)
{
  EFI_STATUS                  Status;
  EFI_ACPI_TABLE_PROTOCOL     *AcpiTableProtocol = NULL;
  EFI_ACPI_SDT_PROTOCOL       *AcpiTableSdtProtocol = NULL;
  EFI_ACPI_SDT_HEADER         *Table = NULL;
  EFI_ACPI_TABLE_VERSION      TableVersion;
  UINTN                       TableKey;
  UINTN                       Idx;

  /*
   * Get access to ACPI tables
   */
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID**) &AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    DBG("[%a:%d]: Unable to locate ACPI table protocol\n", __FUNCTION__, __LINE__);
    return;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID**) &AcpiTableSdtProtocol);
  if (EFI_ERROR (Status)) {
    DBG("[%a:%d]: Unable to locate ACPI table support protocol\n", __FUNCTION__, __LINE__);
    return;
  }

  /*
   * Search for ACPI Table Signature
   */
  for (Idx = 0; ; Idx++) {
    Status = AcpiTableSdtProtocol->GetAcpiTable (Idx, &Table, &TableVersion, &TableKey);
    if (EFI_ERROR (Status)) {
      DBG("APEI: Unable to get ACPI table index:%d\n", Idx);
      return;
    } else if (Table->Signature == Signature) {
      break;
    }
  }

  /*
   * Uninstall ACPI Table
   */
  Status = AcpiTableProtocol->UninstallAcpiTable (AcpiTableProtocol, TableKey);
  if (EFI_ERROR (Status)) {
    DBG("APEI: Unable to uninstall table\n");
  }
}

/*
 * Update APEI
 *
 */
EFI_STATUS
AcpiApeiUpdate (VOID)
{
  EFI_STATUS                 Status;
  ACPI_CONFIG_VARSTORE_DATA  AcpiConfigData;
  UINTN                      BufferSize;

  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status = gRT->GetVariable (
             L"AcpiConfigNVData",
             &gAcpiConfigFormSetGuid,
             NULL,
             &BufferSize,
             &AcpiConfigData);
  if (!EFI_ERROR (Status) && !AcpiConfigData.EnableApeiSupport) {
      AcpiApeiUninstallTable (EFI_ACPI_6_3_BOOT_ERROR_RECORD_TABLE_SIGNATURE);
      AcpiApeiUninstallTable (EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE);
      AcpiApeiUninstallTable (EFI_ACPI_6_3_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_SIGNATURE);
      AcpiApeiUninstallTable (EFI_ACPI_6_3_ERROR_INJECTION_TABLE_SIGNATURE);
  }

  return EFI_SUCCESS;
}

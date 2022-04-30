/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/NVParamLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResetSystemLib.h>
#include <Library/AmpereCpuLib.h>

/**
  Entry point function for the PEIM

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS    If we installed our PPI

**/
EFI_STATUS
EFIAPI
FlashPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  CHAR8               BuildUuid[PcdGetSize (PcdPlatformConfigUuid)+sizeof(BOOLEAN)];
  CHAR8               StoredUuid[sizeof (BuildUuid)];
  EFI_STATUS          Status;
  UINTN               FWNvRamStartOffset;
  UINT32              FWNvRamSize;
  UINTN               NvRamAddress;
  UINT32              NvRamSize;
  BOOLEAN             IsAc01;

  IsAc01 = IsAc01Processor ();
  CopyMem ((VOID *)BuildUuid, PcdGetPtr (PcdPlatformConfigUuid), sizeof (BuildUuid));
  BuildUuid[sizeof (BuildUuid)-sizeof(BOOLEAN)]=IsAc01;

  NvRamAddress = PcdGet64 (PcdFlashNvStorageVariableBase64);
  NvRamSize = FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
              FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
              FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

  DEBUG ((
    DEBUG_INFO,
    "%a: Using NV store FV in-memory copy at 0x%lx with size 0x%x\n",
    __FUNCTION__,
    NvRamAddress,
    NvRamSize
    ));

  Status = FlashGetNvRamInfo (&FWNvRamStartOffset, &FWNvRamSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Flash NVRAM info %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (FWNvRamSize < (NvRamSize * 2 + sizeof (BuildUuid))) {
    //
    // NVRAM size provided by FW is not enough
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // We stored BUILD UUID build at the offset NVRAM_SIZE * 2
  //
  Status = FlashReadCommand (
             FWNvRamStartOffset + NvRamSize * 2,
             (UINT8 *)StoredUuid,
             sizeof (StoredUuid)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CompareMem ((VOID *)StoredUuid, (VOID *)BuildUuid, sizeof (BuildUuid)) != 0) {
    DEBUG ((DEBUG_INFO, "BUILD UUID Changed, Update Storage with NVRAM FV\n"));

    Status = FlashEraseCommand (FWNvRamStartOffset, NvRamSize * 2 + sizeof (BuildUuid));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = FlashWriteCommand (
               FWNvRamStartOffset,
               (UINT8 *)NvRamAddress,
               NvRamSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Write new BUILD UUID to the Flash
    //
    Status = FlashWriteCommand (
               FWNvRamStartOffset + NvRamSize * 2,
               (UINT8 *)BuildUuid,
               sizeof (BuildUuid)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = NVParamClrAll ();
    if (!EFI_ERROR (Status)) {
      //
      // Trigger reset to use default NVPARAM
      //
      ResetCold ();
    }
  } else {
    DEBUG ((DEBUG_INFO, "Identical UUID, copy stored NVRAM to RAM\n"));

    Status = FlashReadCommand (
               FWNvRamStartOffset,
               (UINT8 *)NvRamAddress,
               NvRamSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

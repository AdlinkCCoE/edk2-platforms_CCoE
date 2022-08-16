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
//><ADLINK-PX20220627_01>//
#include <Library/AmpereCpuLib.h>
//<<ADLINK-PX20220627_01>//
//><ADLINK-PX20220815_01>//
#include <Library/GpioLib.h>

#define BIOS_RECOVER_GPIO 4
#define ACTIVE_HIGH 1
//<<ADLINK-PX20220815_01>//

//><ADLINK-PX20220627_01>//
#define UUID_SIZE     PcdGetSize (PcdPlatformConfigUuid)+sizeof(BOOLEAN)
//<<ADLINK-PX20220627_01>//

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
  CHAR8               BuildUuid[UUID_SIZE];
  CHAR8               StoredUuid[UUID_SIZE];
//><ADLINK-PX20220627_01>//
  CHAR8               NullUuid[PcdGetSize (PcdPlatformConfigUuid)];
//<<ADLINK-PX20220627_01>//
  EFI_STATUS          Status;
  UINTN               FWNvRamStartOffset;
  UINT32              FWNvRamSize;
  UINTN               NvRamAddress;
  UINT32              NvRamSize;
  UINTN               UefiMiscOffset;
  UINT32              UefiMiscSize;
//><ADLINK-PX20220627_01>//
  BOOLEAN             IsAc01;

  IsAc01 = IsAc01Processor ();
  CopyMem ((VOID *)BuildUuid, PcdGetPtr (PcdPlatformConfigUuid), UUID_SIZE);
  BuildUuid[sizeof (BuildUuid)-sizeof(IsAc01)]=IsAc01;
//<<ADLINK-PX20220627_01>//

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

  Status = FlashGetNvRamInfo (&FWNvRamStartOffset, &FWNvRamSize, &UefiMiscOffset, &UefiMiscSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Flash NVRAM info %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (FWNvRamSize < (NvRamSize * 2)
      || UefiMiscSize < UUID_SIZE) {
    //
    // NVRAM size provided by FW is not enough
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the Platform UUID stored in the very first bytes of the UEFI Misc.
  //
  Status = FlashReadCommand (
             UefiMiscOffset,
             (UINT8 *)StoredUuid,
             UUID_SIZE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

//><ADLINK-PX20220627_01>//
  DEBUG ((DEBUG_INFO, "StoredUuid = "));
  for (int i=0; i< sizeof (StoredUuid)-sizeof(IsAc01); i++) {
    DEBUG ((DEBUG_INFO, "%x ", StoredUuid[i]));
  }
  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "stored IsAc01 = %x\n", StoredUuid[sizeof(StoredUuid)-sizeof(IsAc01)]));
  DEBUG ((DEBUG_INFO, "IsAc01 = %x\n", IsAc01));
//<<ADLINK-PX20220627_01>//

//><ADLINK-PX20220815_01>//
  if ((CompareMem ((VOID *)StoredUuid, (VOID *)BuildUuid, UUID_SIZE) != 0) || 
      (GpioReadBit(BIOS_RECOVER_GPIO)==ACTIVE_HIGH))
  {
    DEBUG ((DEBUG_INFO, "BUILD UUID Changed, Update Storage with NVRAM FV\n"));
    DEBUG ((DEBUG_INFO, "%a%, CPU_BIOS_RECOVER_GPIOAC6 = %x\n", __FUNCTION__,   GpioReadBit(BIOS_RECOVER_GPIO)));
//<<ADLINK-PX20220815_01>//

    Status = FlashEraseCommand (FWNvRamStartOffset, NvRamSize * 2);
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
    Status = FlashEraseCommand (UefiMiscOffset, UUID_SIZE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = FlashWriteCommand (
               UefiMiscOffset,
               (UINT8 *)BuildUuid,
               UUID_SIZE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

//><ADLINK-PX20220627_01>//
    for (int i=0; i<sizeof(NullUuid); i++) {
      NullUuid[i] = 0xff;
    }
    if (CompareMem ((VOID *)StoredUuid, (VOID *)NullUuid, sizeof (NullUuid)) != 0) {
      Status = NVParamClrAll ();
      if (!EFI_ERROR (Status)) {
        //
        // Trigger reset to use default NVPARAM
        //
        ResetCold ();
      }
    }
//<<ADLINK-PX20220627_01>//
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

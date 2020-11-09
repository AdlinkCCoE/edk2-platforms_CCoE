/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_CONFIG_H_
#define _CPU_CONFIG_H_

#include <Uefi.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformManagerHii.h>
#include <Guid/CpuConfigHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/NVParamLib.h>
#include <NVParamDef.h>
#include "NVDataStruc.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8  VfrBin[];

//
// This is the generated String package data for all .UNI files.
//
extern UINT8  AmpereCpuDxeStrings[];

#define CPU_CONFIG_PRIVATE_SIGNATURE SIGNATURE_32 ('C', 'P', 'U', '_')

typedef struct {
  UINTN                            Signature;

  EFI_HANDLE                       DriverHandle;
  EFI_HII_HANDLE                   HiiHandle;
  CPU_VARSTORE_DATA                Configuration;

  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
} CPU_CONFIG_PRIVATE_DATA;

#define CPU_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, CPU_CONFIG_PRIVATE_DATA, ConfigAccess, CPU_CONFIG_PRIVATE_SIGNATURE)

#define SUBNUMA_MODE_MONOLITHIC 0x0
#define SUBNUMA_MODE_HEMISPHERE 0x1
#define SUBNUMA_MODE_QUADRANT   0x2

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

/**
  This function initializes CPU Configuration Menu.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.

**/
EFI_STATUS
EFIAPI
CpuConfigInit (
  VOID
  );

#endif /* _CPU_CONFIG_H_ */

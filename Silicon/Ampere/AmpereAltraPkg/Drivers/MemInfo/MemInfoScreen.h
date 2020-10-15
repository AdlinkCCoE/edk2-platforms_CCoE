 /**
 *
 * Copyright (c) 2020, Ampere Computing LLC
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD License
 *  which accompanies this distribution.  The full text of the license may be found at
 *  http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
 **/

#ifndef _MEM_INFO_SCREEN_H_
#define _MEM_INFO_SCREEN_H_

#include <Uefi.h>

#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiConfigKeyword.h>

#include <Guid/MdeModuleHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/PlatformInfo.h>
#include <Library/AmpereCpuLib.h>

#include "NVDataStruc.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  VfrBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  MemInfoDxeStrings[];

enum DDR_ECC_MODE {
  ECC_DISABLE = 0,
  ECC_SECDED,
  SYMBOL_ECC
};

enum DDR_ERROR_CTRL_MODE_DE {
  ERRCTLR_DE_DISABLE = 0,
  ERRCTLR_DE_ENABLE,
};

enum DDR_ERROR_CTRL_MODE_FI {
  ERRCTLR_FI_DISABLE = 0,
  ERRCTLR_FI_ENABLE,
};

#define MEM_INFO_DDR_SPEED_SEL_OFFSET                     OFFSET_OF (MEM_INFO_VARSTORE_DATA, DDRSpeedSel)
#define MEM_INFO_ECC_MODE_SEL_OFFSET                      OFFSET_OF (MEM_INFO_VARSTORE_DATA, EccMode)
#define MEM_INFO_ERR_CTRL_DE_MODE_SEL_OFFSET              OFFSET_OF (MEM_INFO_VARSTORE_DATA, ErrCtrl_DE)
#define MEM_INFO_ERR_CTRL_FI_MODE_SEL_OFFSET              OFFSET_OF (MEM_INFO_VARSTORE_DATA, ErrCtrl_FI)
#define MEM_INFO_ERR_SLAVE_32BIT_OFFSET                   OFFSET_OF (MEM_INFO_VARSTORE_DATA, Slave32bit)
#define MEM_INFO_DDR_SCRUB_OFFSET                         OFFSET_OF (MEM_INFO_VARSTORE_DATA, ScrubPatrol)
#define MEM_INFO_DDR_DEMAND_SCRUB_OFFSET                  OFFSET_OF (MEM_INFO_VARSTORE_DATA, DemandScrub)
#define MEM_INFO_DDR_WRITE_CRC_OFFSET                     OFFSET_OF (MEM_INFO_VARSTORE_DATA, WriteCrc)

#define MEM_INFO_SCREEN_PRIVATE_DATA_SIGNATURE            SIGNATURE_32 ('M', 'E', 'M', 'i')

#define MEM_INFO_DDR_SPEED_SEL_QUESTION_ID                       0x8001
#define MEM_INFO_FORM_PERFORMANCE_QUESTION_ID                    0x8002
#define MEM_INFO_FORM_PERFORMANCE_ECC_QUESTION_ID                0x8003
#define MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_DE_QUESTION_ID        0x8004
#define MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_FI_QUESTION_ID        0x8005
#define MEM_INFO_DDR_SLAVE_32BIT_QUESTION_ID                     0x8006
#define MEM_INFO_DDR_SCRUB_PATROL_QUESTION_ID                    0x8007
#define MEM_INFO_DDR_DEMAND_SCRUB_QUESTION_ID                    0x8008
#define MEM_INFO_DDR_WRITE_CRC_QUESTION_ID                       0x8009

#define DDR_DEFAULT_SCRUB_PATROL_DURATION 24
#define DDR_DEFAULT_DEMAND_SCRUB          1
#define DDR_DEFAULT_WRITE_CRC             0

typedef struct {
  UINTN                                 Signature;

  EFI_HANDLE                            DriverHandle;
  EFI_HII_HANDLE                        HiiHandle;
  MEM_INFO_VARSTORE_DATA                VarStoreConfig;

  //
  // Consumed protocol
  //
  EFI_HII_DATABASE_PROTOCOL             *HiiDatabase;
  EFI_HII_STRING_PROTOCOL               *HiiString;
  EFI_HII_CONFIG_ROUTING_PROTOCOL       *HiiConfigRouting;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL   *HiiKeywordHandler;
  EFI_FORM_BROWSER2_PROTOCOL            *FormBrowser2;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL        ConfigAccess;
} MEM_INFO_SCREEN_PRIVATE_DATA;

#define MEM_INFO_SCREEN_PRIVATE_FROM_THIS(a)  CR (a, MEM_INFO_SCREEN_PRIVATE_DATA, ConfigAccess, MEM_INFO_SCREEN_PRIVATE_DATA_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

EFI_STATUS
MemInfoScreenInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

EFI_STATUS
MemInfoScreenUnload (
  IN EFI_HANDLE ImageHandle
  );

EFI_STATUS
MemInfoNvparamGet (
  OUT    MEM_INFO_VARSTORE_DATA      *VarStoreConfig
  );

EFI_STATUS
MemInfoNvparamSet (
  IN     MEM_INFO_VARSTORE_DATA      *VarStoreConfig
  );

#endif /* _MEM_INFO_SCREEN_H_ */


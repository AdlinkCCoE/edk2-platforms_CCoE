/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmLib.h>
#include <Library/HobLib.h>
#include <Library/AmpereCpuLib.h>
#include <Guid/SmBios.h>
#include <Protocol/Smbios.h>
#include <PlatformInfoHob.h>

#define CPU_CACHE_LEVEL_MAX 3

#define TYPE4_ADDITIONAL_STRINGS                                  \
  "SOCKET 0\0"                       /* socket type */            \
  "Ampere Computing\0"               /* manufacturer */           \
  "Ampere(TM) Altra(TM) Processor\0" /* processor description */  \
  "NotSet\0"                         /* part number */

#define TYPE7_ADDITIONAL_STRINGS                                  \
  "L1 Cache\0"                       /* L1 Cache  */

//
// Type definition and contents of the default SMBIOS table.
// This table covers only the minimum structures required by
// the SMBIOS specification (section 6.2, version 3.0)
//
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE4 Base;
  CHAR8              Strings[sizeof(TYPE4_ADDITIONAL_STRINGS)];
} ARM_TYPE4;

typedef struct {
  SMBIOS_TABLE_TYPE7 Base;
  CHAR8              Strings[sizeof(TYPE7_ADDITIONAL_STRINGS)];
} ARM_TYPE7;

#pragma pack()
//-------------------------------------
//        SMBIOS Platform Common
//-------------------------------------
enum {
  ADDITIONAL_STR_INDEX_1 = 1,
  ADDITIONAL_STR_INDEX_2,
  ADDITIONAL_STR_INDEX_3,
  ADDITIONAL_STR_INDEX_4,
  ADDITIONAL_STR_INDEX_5,
  ADDITIONAL_STR_INDEX_6,
  ADDITIONAL_STR_INDEX_7,
  ADDITIONAL_STR_INDEX_8,
  ADDITIONAL_STR_INDEX_9,
  ADDITIONAL_STR_INDEX_MAX
};

// Type 4 Processor Socket 0
STATIC ARM_TYPE4 mArmDefaultType4Sk0 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE4),           // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1, //socket type
    3,                      //processor type CPU
    ProcessorFamilyIndicatorFamily2, //processor family, acquire from field2
    ADDITIONAL_STR_INDEX_2, //manufacturer
    {{0,},{0.}},            //processor id
    ADDITIONAL_STR_INDEX_3, //version
    {0,0,0,0,0,1},          //voltage
    0,                      //external clock
    3000,                   //max speed
    3000,                   //current speed
    0x41,                   //status
    ProcessorUpgradeOther,
    0xFFFF,                 //l1 cache handle
    0xFFFF,                 //l2 cache handle
    0xFFFF,                 //l3 cache handle
    0,                      //serial not set
    0,                      //asset not set
    ADDITIONAL_STR_INDEX_4, //part number
    80,                     //core count in socket
    80,                     //enabled core count in socket
    0,                      //threads per socket
    0xEC,                   // processor characteristics
    ProcessorFamilyARMv8,   //ARM core
  },
  TYPE4_ADDITIONAL_STRINGS
};

// Type 4 Processor Socket 1
STATIC ARM_TYPE4 mArmDefaultType4Sk1 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE4),           // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1, //socket type
    3,                      //processor type CPU
    ProcessorFamilyIndicatorFamily2, //processor family, acquire from field2
    ADDITIONAL_STR_INDEX_2, //manufacturer
    {{0,},{0.}},            //processor id
    ADDITIONAL_STR_INDEX_3, //version
    {0,0,0,0,0,1},          //voltage
    0,                      //external clock
    3000,                   //max speed
    3000,                   //current speed
    0x41,                   //status
    ProcessorUpgradeOther,
    0xFFFF,                 //l1 cache handle
    0xFFFF,                 //l2 cache handle
    0xFFFF,                 //l3 cache handle
    0,                      //serial not set
    0,                      //asset not set
    ADDITIONAL_STR_INDEX_4, //part number
    80,                     //core count in socket
    80,                     //enabled core count in socket
    0,                      //threads per socket
    0xEC,                   // processor characteristics
    ProcessorFamilyARMv8,   //ARM core
  },
  TYPE4_ADDITIONAL_STRINGS
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk0L1 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x180,              //L1 enabled, Write Back
    0x8001,             //64k i cache max
    0x8001,             //64k installed
    {0, 0, 0, 0, 0, 1}, //SRAM type
    {0, 0, 0, 0, 0, 1}, //SRAM type
    0,                  //unkown speed
    CacheErrorParity,   //parity checking
    CacheTypeUnified,   //Unified cache
    CacheAssociativity4Way, // 4-way
  },
  "L1 Cache\0"
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk0L2 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x181,              //L2 enabled, Write Back
    0x8010,             //1M cache max
    0x8010,             //1M installed
    {0, 0, 0, 0, 0, 1}, //SRAM type
    {0, 0, 0, 0, 0, 1}, //SRAM type
    0,                  //unkown speed
    CacheErrorSingleBit,//single bit ECC
    CacheTypeUnified,   //Unified cache
    CacheAssociativity8Way, //8-way
  },
  "L2 Cache\0"
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk0L3 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x182,              //L3 enabled, Write Back
    0x8200,             //32M cache max
    0x8200,             //32M installed
    {0, 0, 0, 0, 0, 1}, //SRAM type
    {0, 0, 0, 0, 0, 1}, //SRAM type
    0,                  //unkown speed
    CacheErrorParity,   //parity checking
    CacheTypeUnified,   //Unified cache
    CacheAssociativity32Way, // 32-way
  },
  "L3 Cache\0"
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk1L1 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x180,             //L1 enabled, Write Back
    0x8001,            //64k i cache max
    0x8001,            //64k installed
    {0, 0, 0, 0, 0, 1},//SRAM type
    {0, 0, 0, 0, 0, 1},//SRAM type
    0,                 //unkown speed
    CacheErrorParity,  //parity checking
    CacheTypeUnified,  //Unified cache
    CacheAssociativity4Way, //4-way
  },
  "L1 Cache\0"
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk1L2 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x181,              //L2 enabled, Write Back
    0x8010,             //1M cache max
    0x8010,             //1M installed
    {0, 0, 0, 0, 0, 1}, //SRAM type
    {0, 0, 0, 0, 0, 1}, //SRAM type
    0,                  //unkown speed
    CacheErrorSingleBit,//single bit ECC
    CacheTypeUnified,   //Unified cache
    CacheAssociativity8Way, //8-way
  },
  "L2 Cache\0"
};

// Type 7 Cache Information
STATIC ARM_TYPE7 mArmDefaultType7Sk1L3 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,
    0x182,              //L3 enabled, Write Back
    0x8200,             //32M cache max
    0x8200,             //32M installed
    {0, 0, 0, 0, 0, 1}, //SRAM type
    {0, 0, 0, 0, 0, 1}, //SRAM type
    0,                  //unkown speed
    CacheErrorParity,   //parity checking
    CacheTypeUnified,   //Unified cache
    CacheAssociativity32Way, // 32-way
  },
  "L3 Cache\0"
};

STATIC CONST VOID* DefaultCommonTables[] =
{
  &mArmDefaultType4Sk0,
  &mArmDefaultType4Sk1,
  NULL
};

STATIC CONST VOID* DefaultType7Sk0Tables[] =
{
  &mArmDefaultType7Sk0L1,
  &mArmDefaultType7Sk0L2,
  &mArmDefaultType7Sk0L3,
  NULL
};

STATIC CONST VOID* DefaultType7Sk1Tables[] =
{
  &mArmDefaultType7Sk1L1,
  &mArmDefaultType7Sk1L2,
  &mArmDefaultType7Sk1L3,
  NULL
};

STATIC
UINT32
GetCacheConfig (
  UINTN Level
  )
{
  UINT64 Val;
  BOOLEAN SupportWB;
  BOOLEAN SupportWT;

  Val = Aarch64ReadCCSIDRReg (Level);
  SupportWT = (Val & (1 << 31)) ? TRUE : FALSE;
  SupportWB = (Val & (1 << 30)) ? TRUE : FALSE;
  if (SupportWT && SupportWB) {
    return 2; /* Varies with Memory Address */
  }

  if (SupportWT) {
    return 0;
  }

  return 1; /* WB */
}

STATIC
UINTN
GetStringPackSize (
  CHAR8 *StringPack
  )
{
  UINTN     StrCount;
  CHAR8     *StrStart;

  if (*StringPack == 0 && *(StringPack + 1) == 0 ) {
    return 0;
  }
  // String section ends in double-null (0000h)
  for (StrCount = 0, StrStart = StringPack;
        (*StrStart != 0 || *(StrStart + 1) != 0); StrStart++, StrCount++) {};

  return StrCount + 2; // Included the double NULL
}

// Update String at String number to String Pack
EFI_STATUS
UpdateStringPack (
  CHAR8             *StringPack,
  CHAR8             *String,
  UINTN             StringNumber
  )
{
  CHAR8                     *StrStart;
  UINTN                     StrIndex;
  UINTN                     InputStrLen;
  UINTN                     TargetStrLen;
  UINTN                     BufferSize;
  CHAR8                     *Buffer;

  StrStart = StringPack;
  for (StrIndex = 1; StrIndex < StringNumber; StrStart++) {
    // A string ends in 00h
    if (*StrStart == 0) {
      StrIndex++;
    }
    // String section ends in double-null (0000h)
    if (*StrStart == 0 && *(StrStart + 1) == 0) {
      return EFI_NOT_FOUND;
    }
  }

  if (*StrStart == 0) {
    StrStart++;
  }

  InputStrLen = AsciiStrLen (String);
  TargetStrLen = AsciiStrLen (StrStart);
  BufferSize = GetStringPackSize (StrStart + TargetStrLen + 1);

  // Replace the String if length matched
  // OR this is the last string
  if (InputStrLen == TargetStrLen || !BufferSize) {
    CopyMem (StrStart, String, InputStrLen);
  }
  // Otherwise, buffer is needed to apply new string
  else {
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Buffer, StrStart + TargetStrLen + 1, BufferSize);
    CopyMem (StrStart, String, InputStrLen + 1);
    CopyMem (StrStart + InputStrLen + 1, Buffer, BufferSize);

    FreePool (Buffer);
  }

  return EFI_SUCCESS;
}

#define MHZ_SCALE_FACTOR 1000000
STATIC
VOID
UpdateSmbiosType4 (
  PlatformInfoHob_V2 *PlatformHob
  )
{
  UINTN                       Index;
  CHAR8                       Str[40];
  CHAR8                       *StringPack;
  SMBIOS_TABLE_TYPE4          *Table;

  ASSERT (PlatformHob);

  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    if (Index == 0) {
      Table = &mArmDefaultType4Sk0.Base;
      StringPack = mArmDefaultType4Sk0.Strings;
    }
    else {
      Table = &mArmDefaultType4Sk1.Base;
      StringPack = mArmDefaultType4Sk1.Strings;
    }

    AsciiSPrint (Str, sizeof (Str), "CPU %d", Index);
    UpdateStringPack (StringPack, Str, ADDITIONAL_STR_INDEX_1);

    Table->CoreCount = (UINT16) GetMaximumNumberOfCores ();
    Table->ThreadCount = (UINT16) GetMaximumNumberOfCores ();
    Table->EnabledCoreCount = (UINT16) (GetNumberActiveCoresPerSocket (Index));

    if (Table->EnabledCoreCount) {
      Table->CurrentSpeed = (UINT16) (PlatformHob->CpuClk / MHZ_SCALE_FACTOR);
      Table->ExternalClock = (UINT16) (PlatformHob->PcpClk / MHZ_SCALE_FACTOR);
      Table->MaxSpeed = (UINT16) (PlatformHob->CpuClk / MHZ_SCALE_FACTOR);
      if (PlatformHob->TurboCapability[Index]) {
        Table->MaxSpeed = (UINT16) (PlatformHob->TurboFrequency[Index]);
      }
    }
    else {
      Table->CurrentSpeed = 0;
      Table->ExternalClock = 0;
      Table->MaxSpeed = 0;
      Table->Status = 0;
    }

    *((UINT32*)&Table->ProcessorId) = (UINT32) ArmReadMidr ();
    *((UINT32*)&Table->ProcessorId + 1) = 0;
    *((UINT8*)&Table->Voltage) = 0x80 | PlatformHob->CoreVoltage[Index] / 100;

    /* Type 4 Part number */
    if (Table->EnabledCoreCount) {
      if ((PlatformHob->ScuProductId[Index] & 0xff) == 0x01) {
        AsciiSPrint (Str, sizeof (Str), "Q%02d-%02X",
          PlatformHob->SkuMaxCore[Index], PlatformHob->SkuMaxTurbo[Index]);
      }
      else {
        AsciiSPrint (Str, sizeof (Str), "M%02d%02X",
          PlatformHob->SkuMaxCore[Index], PlatformHob->SkuMaxTurbo[Index]);
      }

    UpdateStringPack (StringPack, Str, ADDITIONAL_STR_INDEX_4);
    }
  }
}

#define CACHE_SIZE(x) (UINT16) (0x8000 | (x) >> 16)
#define CACHE_SIZE_2(x) (0x80000000 | (x) >> 16)

STATIC
VOID
UpdateSmbiosType7 (
  PlatformInfoHob_V2 *PlatformHob
  )
{
  UINTN                       Index;
  SMBIOS_TABLE_TYPE7          *Table;

  ASSERT(PlatformHob);

  for (Index = 0; Index < GetNumberSupportedSockets (); Index++) {
    if (Index == 0) {
      Table = &mArmDefaultType7Sk0L1.Base;
    }
    else {
      Table = &mArmDefaultType7Sk1L1.Base;
    }

    Table->Associativity = (UINT8) CpuGetAssociativity (1);
    Table->CacheConfiguration = (1 << 7 | GetCacheConfig (1) << 8); /* Enabled, Internal, L1 */
    Table->MaximumCacheSize  = CACHE_SIZE (CpuGetCacheSize (1));
    Table->InstalledSize     = CACHE_SIZE (CpuGetCacheSize (1));
    Table->MaximumCacheSize2 = CACHE_SIZE_2 (CpuGetCacheSize (1));
    Table->InstalledSize2    = CACHE_SIZE_2 (CpuGetCacheSize (1));

    if (Index == 0) {
      Table = &mArmDefaultType7Sk0L2.Base;
    }
    else {
      Table = &mArmDefaultType7Sk1L2.Base;
    }

    Table->Associativity = (UINT8) CpuGetAssociativity (2);
    Table->CacheConfiguration = (1 << 7 | GetCacheConfig (2) << 8 | 1); /* Enabled, Internal, L2 */
    Table->MaximumCacheSize  = CACHE_SIZE (CpuGetCacheSize (2));
    Table->InstalledSize     = CACHE_SIZE (CpuGetCacheSize (2));
    Table->MaximumCacheSize2 = CACHE_SIZE_2 (CpuGetCacheSize (2));
    Table->InstalledSize2    = CACHE_SIZE_2 (CpuGetCacheSize (2));

    if (Index == 0) {
      Table = &mArmDefaultType7Sk0L3.Base;
    }
    else {
      Table = &mArmDefaultType7Sk1L3.Base;
    }

    // CpuGetCacheSize() not return L3 size, set it to 32MB
    Table->MaximumCacheSize  = CACHE_SIZE (32 << 20);
    Table->InstalledSize     = CACHE_SIZE (32 << 20);
    Table->MaximumCacheSize2 = CACHE_SIZE_2 (32 << 20);
    Table->InstalledSize2    = CACHE_SIZE_2 (32 << 20);
  }

  if (GetNumberSupportedSockets () == 2 && GetNumberActiveCoresPerSocket (1) == 0) {
    mArmDefaultType7Sk1L1.Base.InstalledSize = 0;
    mArmDefaultType7Sk1L1.Base.InstalledSize2 = 0;
    mArmDefaultType7Sk1L2.Base.InstalledSize = 0;
    mArmDefaultType7Sk1L2.Base.InstalledSize2 = 0;
    mArmDefaultType7Sk1L3.Base.InstalledSize = 0;
    mArmDefaultType7Sk1L3.Base.InstalledSize2 = 0;
  }

}

STATIC
VOID
UpdateSmbiosInfo (
  VOID
  )
{
  VOID                        *Hob;
  PlatformInfoHob_V2          *PlatformHob;
  CONST EFI_GUID              PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&PlatformHobGuid);
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);

  UpdateSmbiosType4 (PlatformHob);
  UpdateSmbiosType7 (PlatformHob);
}

/**
   Install SMBIOS Type 7 table

   @param  Smbios               SMBIOS protocol.
**/
EFI_STATUS
InstallType7Structures (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE   SmbiosHandle;
  SMBIOS_TABLE_TYPE4  *Type4Table;
  CONST VOID          **Tables;
  UINTN               Index;
  UINTN               Level;

  for ( Index = 0; Index < GetNumberSupportedSockets (); Index++ ) {
    if ( Index == 0) {
      Tables = DefaultType7Sk0Tables;
      Type4Table = &mArmDefaultType4Sk0.Base;
    }
    else {
      Tables = DefaultType7Sk1Tables;
      Type4Table = &mArmDefaultType4Sk1.Base;
    }

    for (Level = 0; Level < CPU_CACHE_LEVEL_MAX; Level++ ) {
      SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER*)Tables[Level])->Handle;
      Status = Smbios->Add (
                         Smbios,
                         NULL,
                         &SmbiosHandle,
                         (EFI_SMBIOS_TABLE_HEADER*)Tables[Level]
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "adding SMBIOS type 7 Socket %d L%d cache failed\n", Index, Level + 1));
        //stop adding rather than continuing
        return Status;
      }

      // Save handle to Type 4
      if (Level == 0) { //L1 cache
        Type4Table->L1CacheHandle = SmbiosHandle;
      }
      else if (Level == 1) { //L2 cache
        Type4Table->L2CacheHandle = SmbiosHandle;
      }
      else if (Level == 2) { //L3 cache
        Type4Table->L3CacheHandle = SmbiosHandle;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
   Install a whole table worth of structructures

   @param  Smbios               SMBIOS protocol.
   @param  DefaultTables        A pointer to the default SMBIOS table structure.
**/
EFI_STATUS
InstallStructures (
  IN EFI_SMBIOS_PROTOCOL  *Smbios,
  IN CONST VOID           *DefaultTables[]
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE   SmbiosHandle;
  UINTN               Index;

  for (Index = 0; Index < GetNumberSupportedSockets () && DefaultTables[Index] != NULL; Index++) {
    SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER*)DefaultTables[Index])->Handle;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER*)DefaultTables[Index]
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "adding %d failed\n", Index));

      //stop adding rather than continuing
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
   Install all structures from the DefaultTables structure

   @param  Smbios               SMBIOS protocol

**/
EFI_STATUS
InstallAllStructures (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                            Status        = EFI_SUCCESS;

  UpdateSmbiosInfo ();

  // Install Type 7 structures
  InstallType7Structures (Smbios);

  // Install Tables
  Status = InstallStructures (Smbios, DefaultCommonTables);
  ASSERT_EFI_ERROR (Status);

  return Status;
}


EFI_STATUS
EFIAPI
SmbiosCpuDxeEntry (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_SMBIOS_PROTOCOL   *Smbios;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID**)&Smbios
                );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to locate SMBIOS Protocol"));
    ASSERT(FALSE);
    return Status;
  }

  Status = InstallAllStructures (Smbios);
  DEBUG ((EFI_D_ERROR, "SmbiosCpu install: %x\n", Status));

  return Status;
}

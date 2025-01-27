/** @file
  Header file for NV data structure definition.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TCG2_CONFIG_NV_DATA_H_
#define TCG2_CONFIG_NV_DATA_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/Tcg2ConfigHii.h>
#include <IndustryStandard/TcgPhysicalPresence.h>

#define TCG2_CONFIGURATION_VARSTORE_ID  0x0001
#define TCG2_CONFIGURATION_INFO_VARSTORE_ID     0x0002
#define TCG2_VERSION_VARSTORE_ID        0x0003
#define TCG2_CONFIGURATION_FORM_ID      0x0001

#define KEY_TPM_DEVICE                                 0x2000
#define KEY_TPM2_OPERATION                             0x2001
#define KEY_TPM2_PCR_BANKS_REQUEST_0            0x2003
#define KEY_TPM2_PCR_BANKS_REQUEST_1            0x2004
#define KEY_TPM2_PCR_BANKS_REQUEST_2            0x2005
#define KEY_TPM2_PCR_BANKS_REQUEST_3            0x2006
#define KEY_TPM2_PCR_BANKS_REQUEST_4            0x2007
#define KEY_TCG2_PPI_VERSION                    0x2009
#define KEY_TPM2_ACPI_REVISION                  0x200A

#define TPM_DEVICE_NULL           0
#define TPM_DEVICE_2_0_DTPM       2
#define TPM_DEVICE_MIN            TPM_DEVICE_2_0_DTPM
#define TPM_DEVICE_MAX            TPM_DEVICE_2_0_DTPM
#define TPM_DEVICE_DEFAULT        TPM_DEVICE_2_0_DTPM

#define TPM2_ACPI_REVISION_3       3
#define TPM2_ACPI_REVISION_4       4

#define TCG2_PPI_VERSION_1_2                    0x322E31 // "1.2"
#define TCG2_PPI_VERSION_1_3                    0x332E31 // "1.3"

//
// Nv Data structure referenced by IFR, TPM device user desired
//
typedef struct {
  UINT8 TpmDevice;
} TCG2_CONFIGURATION;

typedef struct {
  UINT64 PpiVersion;
  UINT8  Tpm2AcpiTableRev;
} TCG2_VERSION;

typedef struct {
  BOOLEAN Sha1Supported;
  BOOLEAN Sha256Supported;
  BOOLEAN Sha384Supported;
  BOOLEAN Sha512Supported;
  BOOLEAN Sm3Supported;
} TCG2_CONFIGURATION_INFO;

#define TCG2_STORAGE_NAME           L"TCG2_CONFIGURATION"
#define TCG2_STORAGE_INFO_NAME      L"TCG2_CONFIGURATION_INFO"
#define TCG2_VERSION_NAME           L"TCG2_VERSION"

#endif

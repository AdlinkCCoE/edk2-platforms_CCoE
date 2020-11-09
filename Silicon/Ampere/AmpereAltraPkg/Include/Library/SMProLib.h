/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>
  
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMPROLIB_H_
#define _SMPROLIB_H_

/*
 * Read the doorbell status into data
 * Db           Doorbell number
 * Data         Data to be written from DB IN
 * Param        Data to be written from DB IN0
 * Param1       Data to be written from DB IN1
 * MsgReg       Non-secure doorbell base virtual address
 */
EFI_STATUS EFIAPI SMProDBRd(UINT8 Db, UINT32 *Data, UINT32 *Param, UINT32 *Param1, UINT64 MsgReg);

/*
 * Write the message to the doorbell
 * Db           Doorbell number
 * Data         Data to be written to DB OUT
 * Param        Data to be written to DB OUT0
 * Param1       Data to be written to DB OUT1
 * MsgReg       Non-secure doorbell base virtual address
 */
EFI_STATUS EFIAPI SMProDBWr(UINT8 Db, UINT32 Data, UINT32 Param, UINT32 Param1, UINT64 MsgReg);

/*
 * Send an APEI Check Message to SMpro
 *
 * UAddress     Upper 32-bit of a 64-bit register for the APEI table pointer
 * LAddress     Lower 32-bit of a 64-bit register for the APEI table pointer
 */
EFI_STATUS EFIAPI SMProAPEISetupCheck(UINT32 UAddress, UINT32 LAddress);

/*
 * Send an APEI Clear Message to SMpro
 *
 * UAddress     Upper 32-bit of a 64-bit register for the APEI table pointer
 * LAddress     Lower 32-bit of a 64-bit register for the APEI table pointer
 */
EFI_STATUS EFIAPI SMProAPEISetupClear(UINT32 UAddress, UINT32 LAddress);

/*
 * Enable/Disable APEI with SMpro
 */
EFI_STATUS EFIAPI SMProAPEIEnable(UINT8 Enable);

/*
 * Read register from SMpro
 */
EFI_STATUS EFIAPI SMProRegRd(UINT8 Socket, UINT64 Addr, UINT32 *Value);

/*
 * Write register to SMpro
 */
EFI_STATUS EFIAPI SMProRegWr(UINT8 Socket, UINT64 Addr, UINT32 Value);

#endif /* _SLIMPROLIB_H_*/

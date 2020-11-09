/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <string.h>
#include <Library/HobLib.h>
#include <Library/PlatformInfo.h>
#include <Library/PciePhyLib.h>
#include <Library/PcieBoardLib.h>
#include <Library/SMProLib.h>
#include "PcieCore.h"
#include "Pcie.h"

#define DEV_MASK 0x00F8000
#define BUS_MASK 0xFF00000

STATIC INT32 Ac01PcieCsrOut32 (VOID *Addr, UINT32 Val)
{
  MmioWrite32 ((UINT64)Addr, Val);
  PCIE_CSR_DEBUG ("PCIE CSR WR: 0x%p value: 0x%08X (0x%08X)\n",
              Addr, Val, MmioRead32 ((UINT64)Addr));

  return 0;
}

STATIC INT32 Ac01PcieCsrOut32Serdes (VOID *Addr, UINT32 Val)
{
  MmioWrite32 ((UINT64)Addr, Val);
  PCIE_CSR_DEBUG ("PCIE CSR WR: 0x%p value: 0x%08X (0x%08X)\n",
              Addr, Val, MmioRead32 ((UINT64)Addr));

  return 0;
}

STATIC INT32 Ac01PcieCsrIn32 (VOID *Addr, UINT32 *Val)
{
  *Val = MmioRead32 ((UINT64)Addr);
  PCIE_CSR_DEBUG ("PCIE CSR RD: 0x%p value: 0x%08X\n", Addr, *Val);

  return 0;
}

STATIC INT32 Ac01PcieCsrIn32Serdes (VOID *Addr, UINT32 *Val)
{
  *Val = MmioRead32 ((UINT64)Addr);
  PCIE_CSR_DEBUG ("PCIE CSR RD: 0x%p value: 0x%08X\n", Addr, *Val);

  return 0;
}

void Ac01PcieMmioRd (unsigned long int addr, unsigned int *val)
{
  Ac01PcieCsrIn32Serdes((VOID *) addr, (UINT32 *) val);
}

void Ac01PcieMmioWr (unsigned long int addr, unsigned int val)
{
  Ac01PcieCsrOut32Serdes( (VOID *)addr, (UINT32) val);
}
void Ac01PciePuts_cb(const char *msg)
{
  PCIE_PHY_DEBUG ("%a\n", __func__);
}
void Ac01PciePutint_cb(unsigned int val)
{

  PCIE_PHY_DEBUG ("%a\n", __func__);
}
void Ac01Pcieputhex_cb(unsigned long val)
{
  PCIE_PHY_DEBUG ("%a\n", __func__);
}
int Ac01Pciedebug_print_cb(const char *fmt, ...)
{
  PCIE_PHY_DEBUG ("%a\n", __func__);
  return 0;
}

void Ac01PcieDelay (unsigned int val)
{
  MicroSecondDelay(val);
}

/**
   Write 32-bit value to config space address

   @param Addr          Address within config space
   @param Val           32-bit value to write
**/
INT32 Ac01PcieCfgOut32 (
  IN VOID *Addr,
  IN UINT32 Val
  )
{
  MmioWrite32 ((UINT64)Addr, Val);
  PCIE_DEBUG_CFG ("PCIE CFG WR: 0x%p value: 0x%08X (0x%08X)\n",
                  Addr, Val, MmioRead32 ((UINT64)Addr));

  return 0;
}

/**
   Write 16-bit value to config space address

   @param Addr          Address within config space
   @param Val           16-bit value to write
**/
INT32 Ac01PcieCfgOut16 (
  IN VOID *Addr,
  IN UINT16 Val
  )
{
  UINT64 AlignedAddr = (UINT64) Addr & ~0x3;
  UINT32 Val32  = MmioRead32 (AlignedAddr);

  switch ((UINT64)Addr & 0x3) {
  case 2:
    Val32 &= ~0xFFFF0000;
    Val32 |= (UINT32) Val << 16;
    break;

  case 0:
  default:
    Val32 &= ~0xFFFF;
    Val32 |= Val;
    break;
  }
  MmioWrite32 (AlignedAddr, Val32);
  PCIE_DEBUG_CFG ("PCIE CFG WR16: 0x%p value: 0x%04X (0x%08llX 0x%08X)\n",
                  Addr, Val, AlignedAddr, MmioRead32 ((UINT64)AlignedAddr));

  return 0;
}

/**
   Write 8-bit value to config space address

   @param Addr          Address within config space
   @param Val           8-bit value to write
**/
INT32
Ac01PcieCfgOut8 (
  IN VOID *Addr,
  IN UINT8 Val
  )
{
  UINT64 AlignedAddr = (UINT64) Addr & ~0x3;
  UINT32 Val32  = MmioRead32 (AlignedAddr);

  switch ((UINT64)Addr & 0x3) {
  case 0:
    Val32 &= ~0xFF;
    Val32 |= Val;
    break;

  case 1:
    Val32 &= ~0xFF00;
    Val32 |= (UINT32) Val << 8;
    break;

  case 2:
    Val32 &= ~0xFF0000;
    Val32 |= (UINT32) Val << 16;
    break;

  case 3:
  default:
    Val32 &= ~0xFF000000;
    Val32 |= (UINT32) Val << 24;
    break;
  }
  MmioWrite32 (AlignedAddr, Val32);
  PCIE_DEBUG_CFG ("PCIE CFG WR8: 0x%p value: 0x%04X (0x%08llX 0x%08X)\n",
                  Addr, Val, AlignedAddr, MmioRead32 ((UINT64)AlignedAddr));

  return 0;
}

/**
   Read 32-bit value from config space address

   @param Addr          Address within config space
   @param Val           Point to address for read value
**/
INT32
Ac01PcieCfgIn32 (
  IN VOID *Addr,
  OUT UINT32 *Val
  )
{
  UINT32 RegC, Reg18;
  UINT8  MfHt, Primary = 0, Sec = 0, Sub = 0;

  if ((BUS_NUM(Addr) > 0) && (DEV_NUM(Addr) > 0) && (CFG_REG(Addr) == 0)) {
    *Val = MmioRead32 ((UINT64)Addr);
    PCIE_DEBUG_CFG ("PCIE CFG RD: B%X|D%X 0x%p value: 0x%08X\n", BUS_NUM(Addr), DEV_NUM(Addr), Addr, *Val);

    if (*Val != 0xffffffff) {
      RegC = MmioRead32 ((UINT64)Addr + 0xC);
      PCIE_DEBUG_CFG ("Peek PCIE MfHt RD32: 0x%p value: 0x%08X\n", Addr + 0xc, RegC);
      MfHt = RegC >> 16;
      PCIE_DEBUG_CFG ("  Peek RD8 MfHt=0x%02X\n", MfHt);

      if ((MfHt & 0x7F)!= 0) { /* Type 1 header */
        Reg18 = MmioRead32 ((UINT64)Addr + 0x18);
        Primary = Reg18; Sec = Reg18 >> 8; Sub = Reg18 >> 16;
        PCIE_DEBUG_CFG ("  Bus Peek PCIE Sub:%01X Sec:%01X Primary:%01X  RD: 0x%p value: 0x%08X\n",
                Sub, Sec, Primary, Addr + 0x18, Reg18);
      }
      if (!MfHt || Primary) { /* QS RPs Primary Bus is 0b */
        *Val = 0xffffffff;
        PCIE_DEBUG_CFG ("  Skip RD32 B%X|D%X PCIE CFG RD: 0x%p return 0xffffffff\n",
                        BUS_NUM(Addr), DEV_NUM(Addr), Addr);
      }
    }
  } else {
    *Val = MmioRead32 ((UINT64)Addr);
  }
  PCIE_DEBUG_CFG ("PCIE CFG RD: 0x%p value: 0x%08X\n", Addr, *Val);

  return 0;
}

/**
   Read 16-bit value from config space address

   @param Addr          Address within config space
   @param Val           Point to address for read value
**/
INT32
Ac01PcieCfgIn16 (
  IN VOID *Addr,
  OUT UINT16 *Val)
{
  UINT64 AlignedAddr = (UINT64) Addr & ~0x3;
  UINT32 RegC, Reg18;
  UINT8  MfHt, Primary = 0, Sec = 0, Sub = 0;
  UINT32 Val32;

  if ((BUS_NUM(Addr) > 0) && (DEV_NUM(Addr) > 0) && (CFG_REG(Addr) == 0)) {
    *Val = MmioRead32 ((UINT64)Addr);
    PCIE_DEBUG_CFG ("PCIE CFG16 RD: B%X|D%X 0x%p value: 0x%08X\n", BUS_NUM(Addr), DEV_NUM(Addr), Addr, *Val);

    if (*Val != 0xffff) {
      RegC = MmioRead32 ((UINT64)Addr + 0xC);
      PCIE_DEBUG_CFG ("  Peek PCIE MfHt RD: 0x%p value: 0x%08X\n", Addr + 0xc, RegC);
      MfHt = RegC >> 16;
      PCIE_DEBUG_CFG ("  Peek RD8 MfHt=0x%02X\n", MfHt);


      if ((MfHt & 0x7F)!= 0) { /* Type 1 header */
        Reg18 = MmioRead32 ((UINT64)Addr + 0x18);
        Primary = Reg18; Sec = Reg18 >> 8; Sub = Reg18 >> 16;
        PCIE_DEBUG_CFG ("  Bus Peek PCIE Sub:%01X Sec:%01X Primary:%01X  RD: 0x%p value: 0x%08X\n",
                Sub, Sec, Primary, Addr + 0x18, Reg18);
      }
      if (!MfHt || Primary) { /* QS RPs Primary Bus is 0b */
        *Val = 0xffff;
        PCIE_DEBUG_CFG ("  Skip RD16 B%X|D%X PCIE CFG RD: 0x%p return 0xffff\n",
                        BUS_NUM(Addr), DEV_NUM(Addr), Addr);
        return 0;
      }
    }
  }

  Val32 = MmioRead32 (AlignedAddr);
  switch ((UINT64)Addr & 0x3) {
  case 2:
    *Val = Val32 >> 16;
    break;

  case 0:
  default:
    *Val = Val32;
    break;
  }
  PCIE_DEBUG_CFG ("PCIE CFG RD16: 0x%p value: 0x%04X (0x%08llX 0x%08X)\n",
                  Addr, *Val, AlignedAddr, Val32);

  return 0;
}

/**
   Read 8-bit value from config space address

   @param Addr          Address within config space
   @param Val           Point to address for read value
**/
INT32
Ac01PcieCfgIn8 (
  IN VOID *Addr,
  OUT UINT8 *Val
  )
{
  UINT64 AlignedAddr = (UINT64) Addr & ~0x3;
  if ((((UINT64)Addr & DEV_MASK) >> 15 )> 0 && (((UINT64)Addr & BUS_MASK)  >> 20)> 0) {
    *Val = 0xff;
   return 0;
  }

  UINT32 Val32 = MmioRead32 (AlignedAddr);
  switch ((UINT64)Addr & 0x3) {
  case 3:
    *Val = Val32 >> 24;
    break;

  case 2:
    *Val = Val32 >> 16;
    break;

  case 1:
    *Val = Val32 >> 8;
    break;

  case 0:
  default:
    *Val = Val32;
    break;
  }
  PCIE_DEBUG_CFG ("PCIE CFG RD8: 0x%p value: 0x%02X (0x%08llX 0x%08X)\n",
                  Addr, *Val, AlignedAddr, Val32);

  return 0;
}

/**
   Return the next extended capability address

   @param RC              Pointer to AC01_RC structure
   @param PcieIndex       PCIe index
   @param IsRC            0x1: Checking RC configuration space
                          0x0: Checking EP configuration space
   @param ExtendedCapId
**/
UINT64
PcieCheckCap (
  IN AC01_RC  *RC,
  IN INTN      PcieIndex,
  IN BOOLEAN   IsRC,
  IN UINT16    ExtendedCapId
  )
{
  VOID         *CfgAddr;
  UINT32        Val = 0, NextCap = 0, CapId = 0, ExCap = 0;

  if (IsRC) {
    CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));
  } else {
    CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 20));
  }

  Ac01PcieCsrIn32 (CfgAddr + TYPE1_CAP_PTR_REG, &Val);
  NextCap = Val & 0xFF;

  // Loop untill desired capability is found else return 0
  while (1) {
    if (NextCap & 0x3) {
     /* Not alignment, just return */
      return 0;
    }
    Ac01PcieCsrIn32 (CfgAddr + NextCap, &Val);
    if (NextCap < EXT_CAP_OFFSET_START) {
      CapId = Val & 0xFF;
    } else {
      CapId = Val & 0xFFFF;
    }

    if (CapId == ExtendedCapId) {
      return (UINT64) (CfgAddr + NextCap);
    }

    if (NextCap < EXT_CAP_OFFSET_START) {
      NextCap = (Val & 0xFFFF) >> 8;
    } else {
      NextCap = (Val & 0xFFFF0000) >> 20;
    }

    if ((NextCap == 0) && (ExCap == 0)) {
      ExCap = 1;
      NextCap = EXT_CAP_OFFSET_START;
    }

    if ((NextCap == 0) && (ExCap == 1)) {
      return (UINT64) 0;
    }
  }
}

/**
   Read 8-bit value from config space address

   @param RC          Pointer to AC01_RC strucutre
   @param RegBase     Base address of CSR, TCU, Hostbridge, Msg, Serdes, and MMCFG register
   @param MmioBase    Base address of 32-bit MMIO
   @param Mmio32Base  Base address of 64-bit MMIO
**/
VOID
Ac01PcieCoreBuildRCStruct (
  AC01_RC *RC,
  UINT64 RegBase,
  UINT64 MmioBase,
  UINT64 Mmio32Base
  )
{
  INTN          PcieIndex;

  RC->BaseAddr = RegBase;
  RC->TcuAddr = RegBase + TCU_OFFSET;
  RC->HBAddr = RegBase + HB_CSR_OFFSET;
  RC->SerdesAddr = RegBase + SERDES_CSR_OFFSET;
  RC->MmcfgAddr = RegBase + MMCONFIG_OFFSET;
  RC->MmioAddr = MmioBase;
  RC->Mmio32Addr = Mmio32Base;
  RC->IoAddr = Mmio32Base + MMIO32_SPACE - IO_SPACE;

  RC->Type = (RC->ID < MAX_RCA) ? RCA : RCB;
  RC->MaxPcieController = (RC->Type == RCB) ? MAX_PCIE_B : MAX_PCIE_A;

  PcieBoardParseRCParams (RC);

  for (PcieIndex = 0; PcieIndex < RC->MaxPcieController; PcieIndex++) {
    RC->Pcie[PcieIndex].ID = PcieIndex;
    RC->Pcie[PcieIndex].CsrAddr = RC->BaseAddr + PCIE0_CSR_OFFSET + PcieIndex*0x10000;
    RC->Pcie[PcieIndex].SnpsRamAddr = RC->Pcie[PcieIndex].CsrAddr + SNPSRAM_OFFSET;
    RC->Pcie[PcieIndex].DevNum = PcieIndex + 1;
  }

  PCIE_DEBUG (" + S%d - RC%a%d, MMCfgAddr:0x%lx, MmioAddr:0x%lx, Mmio32Addr:0x%lx, Enabled:%a\n",
              RC->Socket,
              (RC->Type == RCA) ? "A" : "B",
              RC->ID,
              RC->MmcfgAddr,
              RC->MmioAddr,
              RC->Mmio32Addr,
              (RC->Active == TRUE) ? "Y" : "N");
  PCIE_DEBUG (" +   DevMapLo/Hi: 0x%x/0x%x\n", RC->DevMapLo, RC->DevMapHi);
  for (PcieIndex = 0; PcieIndex < RC->MaxPcieController; PcieIndex++) {
    PCIE_DEBUG (" +     PCIE%d:0x%lx - Enabled:%a - DevNum:0x%x\n", PcieIndex,
                RC->Pcie[PcieIndex].CsrAddr,
                (RC->Pcie[PcieIndex].Active) ? "Y" : "N",
                RC->Pcie[PcieIndex].DevNum);
  }
}

/**
   Configure equalization settings

   @param RC              Pointer to AC01_RC structure
   @param PcieIndex       PCIe index
**/
STATIC
VOID
Ac01PcieConfigureEqualization (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex
  )
{
  VOID *CfgAddr;
  UINT32 Val;

  CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  // Select the FoM method, need double-write to convey settings
  Ac01PcieCfgIn32 (CfgAddr + GEN3_EQ_CONTROL_OFF, &Val);
  Val = GEN3_EQ_FB_MODE (Val, 0x1);
  Val = GEN3_EQ_PRESET_VEC (Val, 0x3FF);
  Val = GEN3_EQ_INIT_EVAL (Val, 0x1);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_EQ_CONTROL_OFF, Val);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_EQ_CONTROL_OFF, Val);
  Ac01PcieCfgIn32 (CfgAddr + GEN3_EQ_CONTROL_OFF, &Val);
}

/**
   Configure presets for GEN3 equalization

   @param RC              Pointer to AC01_RC structure
   @param PcieIndex       PCIe index
**/
STATIC
VOID
Ac01PcieConfigurePresetGen3 (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex
  )
{
  VOID *CfgAddr, *SpcieBaseAddr;
  UINT32 Val, Idx;
  CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  // Bring to legacy mode
  Ac01PcieCfgIn32 (CfgAddr + GEN3_RELATED_OFF, &Val);
  Val = RATE_SHADOW_SEL_SET (Val, 0);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_RELATED_OFF, Val);
  Val = EQ_PHASE_2_3_SET (Val, 0);
  Val = RXEQ_REGRDLESS_SET (Val, 1);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_RELATED_OFF, Val);

  // Generate SPCIE capability address
  SpcieBaseAddr = (VOID *)PcieCheckCap (RC, PcieIndex, 0x1, SPCIE_CAP_ID);
  if (SpcieBaseAddr == 0) {
    PCIE_ERR ("PCIE%d.%d: Cannot get SPCIE capability address\n",
              RC->ID, PcieIndex);
    return;
  }

  for (Idx=0; Idx < RC->Pcie[PcieIndex].MaxWidth/2; Idx++) {
    // Program Preset to Gen3 EQ Lane Control
    Ac01PcieCfgIn32 (SpcieBaseAddr + CAP_OFF_0C + Idx*4, &Val);
    Val = DSP_TX_PRESET0_SET (Val, 0x7);
    Val = DSP_TX_PRESET1_SET (Val, 0x7);
    Ac01PcieCfgOut32 (SpcieBaseAddr + CAP_OFF_0C + Idx*4, Val);
  }
}

/**
   Configure presets for GEN4 equalization

   @param RC              Pointer to AC01_RC structure
   @param PcieIndex       PCIe index
**/
STATIC
VOID
Ac01PcieConfigurePresetGen4 (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex
  )
{
  UINT32    Val;
  VOID      *CfgAddr, *SpcieBaseAddr, *Pl16BaseAddr;
  UINT32    LinkWidth, i;
  UINT8     Preset;

  CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  // Bring to legacy mode
  Ac01PcieCfgIn32 (CfgAddr + GEN3_RELATED_OFF, &Val);
  Val = RATE_SHADOW_SEL_SET (Val, 1);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_RELATED_OFF, Val);
  Val = EQ_PHASE_2_3_SET (Val, 0);
  Val = RXEQ_REGRDLESS_SET (Val, 1);
  Ac01PcieCfgOut32 (CfgAddr + GEN3_RELATED_OFF, Val);

  // Generate the PL16 capability address
  Pl16BaseAddr = (VOID *) PcieCheckCap (RC, PcieIndex, 0x1, PL16_CAP_ID);
  if (Pl16BaseAddr == 0) {
    PCIE_ERR ("PCIE%d.%d: Cannot get PL16 capability address\n",
              RC->ID, PcieIndex);
    return;
  }

  // Generate the SPCIE capability address
  SpcieBaseAddr = (VOID *) PcieCheckCap (RC, PcieIndex, 0x1, SPCIE_CAP_ID);
  if (SpcieBaseAddr == 0) {
    PCIE_ERR ("PCIE%d.%d: Cannot get SPICE capability address\n",
              RC->ID, PcieIndex);
    return;
  }

  // Configure downstream Gen4 Tx preset
  if (RC->PresetGen4[PcieIndex] == PRESET_INVALID) {
    Preset = 0x57; // Default Gen4 preset
  } else {
    Preset = RC->PresetGen4[PcieIndex];
  }

  LinkWidth = RC->Pcie[PcieIndex].MaxWidth;
  if (LinkWidth == 0x2) {
    Ac01PcieCfgIn32 (Pl16BaseAddr + PL16G_CAP_OFF_20H_REG_OFF, &Val);
    Val = DSP_16G_RXTX_PRESET0_SET (Val, Preset);
    Val = DSP_16G_RXTX_PRESET1_SET (Val, Preset);
    Ac01PcieCfgOut32 (Pl16BaseAddr + PL16G_CAP_OFF_20H_REG_OFF, Val);
  } else {
    for (i = 0 ; i < LinkWidth/4; i++) {
      Ac01PcieCfgIn32 (Pl16BaseAddr + PL16G_CAP_OFF_20H_REG_OFF + i*4, &Val);
      Val = DSP_16G_RXTX_PRESET0_SET (Val, Preset);
      Val = DSP_16G_RXTX_PRESET1_SET (Val, Preset);
      Val = DSP_16G_RXTX_PRESET2_SET (Val, Preset);
      Val = DSP_16G_RXTX_PRESET3_SET (Val, Preset);
      Ac01PcieCfgOut32 (Pl16BaseAddr + PL16G_CAP_OFF_20H_REG_OFF + i*4, Val);
    }
  }

  // Configure Gen3 preset
  for (i = 0; i < LinkWidth/2; i++) {
    Ac01PcieCfgIn32 (SpcieBaseAddr + CAP_OFF_0C + i*4, &Val);
    Val = DSP_TX_PRESET0_SET (Val, 0x7);
    Val = DSP_TX_PRESET1_SET (Val, 0x7);
    Ac01PcieCfgOut32 (SpcieBaseAddr + CAP_OFF_0C + i*4, Val);
  }
}

STATIC BOOLEAN RasdpMitigationCheck (AC01_RC *RC, INTN PcieIndex)
{
  EFI_GUID           PlatformHobGuid = PLATFORM_INFO_HOB_GUID_V2;
  PlatformInfoHob_V2 *PlatformHob;
  VOID               *Hob;

  Hob = GetFirstGuidHob (&PlatformHobGuid);
  PlatformHob = (PlatformInfoHob_V2 *) GET_GUID_HOB_DATA (Hob);
  if ((PlatformHob->ScuProductId[0] & 0xff) == 0x01) {
    if (AsciiStrCmp ((CONST CHAR8 *)PlatformHob->CpuVer, "A0") == 0) {
      return ((RC->Type == RCB)||(PcieIndex > 0)) ? TRUE : FALSE;
    }
  }

  return FALSE;
}

/**
   Setup and initialize the AC01 PCIe Root Complex and underneath PCIe controllers

   @param RC	                Pointer to Root Complex structure
   @param ReInit                Re-init status
   @param ReInitPcieIndex       PCIe index
**/
INT32 Ac01PcieCoreSetupRC (
  IN AC01_RC    *RC,
  IN UINT8      ReInit,
  IN UINT8      ReInitPcieIndex
  )
{
  VOID          *CsrAddr, *CfgAddr, *SnpsRamAddr, *DlinkBaseAddr;
  INTN          PcieIndex;
  INTN          TimeOut;
  UINT32        Val;
  phy_context_t PHYCtx = { 0 };
  phy_plat_resource_t phy_plat_resource = { 0 };
  INTN          Ret;
  UINT16        NextExtendedCapabilityOff;
  UINT32        VsecVal;

  PCIE_DEBUG ("Initializing Socket%d RC%d\n", RC->Socket, RC->ID);

  if (ReInit == 0) {
    // Initialize SERDES
    memset(&PHYCtx, sizeof(PHYCtx), 0);
    PHYCtx.sds_addr = RC->SerdesAddr;
    PHYCtx.pcie_ctrl_info |= ((RC->Socket & 0x1) << 2);
    PHYCtx.pcie_ctrl_info |= ((RC->ID & 0x7) << 4);
    PHYCtx.pcie_ctrl_info |= 0xF << 8;
    phy_plat_resource.mmio_rd_cb = Ac01PcieMmioRd;
    phy_plat_resource.mmio_wr_cb = Ac01PcieMmioWr;
    phy_plat_resource.uDelay_cb = Ac01PcieDelay;
    phy_plat_resource.puts_cb = Ac01PciePuts_cb;
    phy_plat_resource.putint_cb = Ac01PciePutint_cb;
    phy_plat_resource.puthex_cb = Ac01PciePutint_cb;
    phy_plat_resource.puthex64_cb = Ac01Pcieputhex_cb;
    phy_plat_resource.debug_print_cb = Ac01Pciedebug_print_cb;
    PHYCtx.phy_plat_resource = &phy_plat_resource;

    Ret = serdes_init_clkrst (&PHYCtx);
    if (Ret != PHY_INIT_PASS)
      return -1;
  }

  // Setup each controller
  for (PcieIndex = 0; PcieIndex < RC->MaxPcieController; PcieIndex ++) {

    if (ReInit == 1) {
      PcieIndex = ReInitPcieIndex;
    }

    if (!RC->Pcie[PcieIndex].Active) {
      continue;
    }

    PCIE_DEBUG ("Initializing Controller %d\n", PcieIndex);

    CsrAddr = (VOID*) RC->Pcie[PcieIndex].CsrAddr;
    SnpsRamAddr = (VOID*) RC->Pcie[PcieIndex].SnpsRamAddr;
    CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

    // Put Controller into reset if not in reset already
    Ac01PcieCsrIn32 (CsrAddr +  RESET, &Val);
    if (!(Val & RESET_MASK)) {
      Val = DWCPCIE_SET (Val, 1);
      Ac01PcieCsrOut32 (CsrAddr + RESET, Val);

      // Delay 50ms to ensure controller finish its reset
      // FIXME: Is this necessary?
      MicroSecondDelay (50000);
    }

    // Clear memory shutdown
    Ac01PcieCsrIn32 (CsrAddr + RAMSDR, &Val);
    Val = SD_SET (Val, 0);
    Ac01PcieCsrOut32 (CsrAddr + RAMSDR, Val);

    // Poll till mem is ready
    TimeOut = PCIE_MEMRDY_TIMEOUT;
    do {
      Ac01PcieCsrIn32 (CsrAddr + MEMRDYR, &Val);
      if (Val & 1)
        break;

      TimeOut--;
      MicroSecondDelay (1);
    } while (TimeOut > 0);

    if (TimeOut <= 0) {
      PCIE_ERR ("- Pcie[%d] - Mem not ready\n", PcieIndex);
      return -1;
    }

    // Hold link training
    Ac01PcieCsrIn32 (CsrAddr + LINKCTRL, &Val);
    Val = LTSSMENB_SET (Val, 0);
    Ac01PcieCsrOut32 (CsrAddr + LINKCTRL, Val);

    // Enable subsystem clock and release reset
    Ac01PcieCsrIn32 (CsrAddr + CLOCK, &Val);
    Val = AXIPIPE_SET (Val, 1);
    Ac01PcieCsrOut32 (CsrAddr + CLOCK, Val);
    Ac01PcieCsrIn32 (CsrAddr +  RESET, &Val);
    Val = DWCPCIE_SET (Val, 0);
    Ac01PcieCsrOut32 (CsrAddr + RESET, Val);

    //
    // Controller does not provide any indicator for reset released.
    // Must wait at least 1us as per EAS.
    //
    MicroSecondDelay (1);

    // Poll till PIPE clock is stable
    TimeOut = PCIE_PIPE_CLOCK_TIMEOUT;
    do {
      Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &Val);
      if (!(Val & PHY_STATUS_MASK))
        break;

      TimeOut--;
      MicroSecondDelay (1);
    } while (TimeOut > 0);

    if (TimeOut <= 0) {
      PCIE_ERR ("- Pcie[%d] - PIPE clock is not stable\n", PcieIndex);
      return -1;
    }

    // Start PERST pulse
    PcieBoardAssertPerst (RC, PcieIndex, 0, TRUE);

    // Allow programming to config space
    Ac01PcieCsrIn32 (CfgAddr + MISC_CONTROL_1_OFF, &Val);
    Val = DBI_RO_WR_EN_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + MISC_CONTROL_1_OFF, Val);

    // Apply RASDP error mitigation for all x8, x4, and x2 controllers
    // This includes all RCB root ports, and every RCA root port controller
    // except for index 0 (i.e. x16 controllers are exempted from this WA)
    if (RasdpMitigationCheck (RC, PcieIndex)) {
      // Change the Read Margin dual ported RAMs
      Val = 0x10; // Margin to 0x0 (most conservative setting)
      Ac01PcieCsrOut32 (SnpsRamAddr + TPSRAM_RMR, Val);
 
      //Generate the DLINK capability address
      DlinkBaseAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));
      NextExtendedCapabilityOff = 0x100; // This is the 1st extended capability offset
      do {
        Ac01PcieCsrIn32 (DlinkBaseAddr + NextExtendedCapabilityOff, &Val);
        if (Val == 0xFFFFFFFF) {
           DlinkBaseAddr = 0x0;
           break;
         }
         if ((Val & 0xFFFF) == DLINK_VENDOR_CAP_ID) {
           Ac01PcieCsrIn32 (DlinkBaseAddr + NextExtendedCapabilityOff + 0x4, &VsecVal);
             if (VsecVal == DLINK_VSEC) {
               DlinkBaseAddr = DlinkBaseAddr + NextExtendedCapabilityOff;
               break;
             }
         }
         NextExtendedCapabilityOff = (Val >> 20);
      } while (NextExtendedCapabilityOff != 0);
     
      // Disable the scaled credit mode
      if (DlinkBaseAddr != 0x0) {
        Val=1;     
        Ac01PcieCsrOut32 (DlinkBaseAddr + DATA_LINK_FEATURE_CAP_OFF, Val);
        Ac01PcieCsrIn32 (DlinkBaseAddr + DATA_LINK_FEATURE_CAP_OFF, &Val);
        if (Val!=1) {
          PCIE_ERR ("- Pcie[%d] - Unable to disable scaled credit\n", PcieIndex);
          return -1;
        }
      } 
      else {
        PCIE_ERR ("- Pcie[%d] - Unable to locate data link feature cap offset\n", PcieIndex);
        return -1;
      }

      // Reduce Posted Credits to 1 packet header and data credit for all
      // impacted controllers.  Also zero credit scale values for both
      // data and packet headers.
      Val=0x40201020;
      Ac01PcieCsrOut32 (CfgAddr + PORT_LOCIG_VC0_P_RX_Q_CTRL_OFF, Val);
    }

    // Program DTI for ATS support
    Ac01PcieCsrIn32 (CfgAddr + DTIM_CTRL0_OFF, &Val);
    Val = DTIM_CTRL0_ROOT_PORT_ID_SET (Val, 0);
    Ac01PcieCsrOut32 (CfgAddr + DTIM_CTRL0_OFF, Val);

    //
    // Program number of lanes used
    // - Reprogram LINK_CAPABLE of PORT_LINK_CTRL_OFF
    // - Reprogram NUM_OF_LANES of GEN2_CTRL_OFF
    // - Reprogram PCIE_CAP_MAX_LINK_WIDTH of LINK_CAPABILITIES_REG
    //
    Ac01PcieCsrIn32 (CfgAddr + PORT_LINK_CTRL_OFF, &Val);
    switch (RC->Pcie[PcieIndex].MaxWidth) {
    case LNKW_X2:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X2);
      break;
    case LNKW_X4:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X4);
      break;
    case LNKW_X8:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X8);
      break;
    case LNKW_X16:
    default:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X16);
      break;
    };
    Ac01PcieCsrOut32 (CfgAddr + PORT_LINK_CTRL_OFF, Val);
    Ac01PcieCsrIn32 (CfgAddr + GEN2_CTRL_OFF, &Val);
    switch (RC->Pcie[PcieIndex].MaxWidth) {
    case LNKW_X2:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X2);
      break;
    case LNKW_X4:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X4);
      break;
    case LNKW_X8:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X8);
      break;
    case LNKW_X16:
    default:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X16);
      break;
    };
    Ac01PcieCsrOut32 (CfgAddr + GEN2_CTRL_OFF, Val);
    Ac01PcieCsrIn32 (CfgAddr + LINK_CAPABILITIES_REG, &Val);
    switch (RC->Pcie[PcieIndex].MaxWidth) {
    case LNKW_X2:
      Val = PCIE_CAP_MAX_LINK_WIDTH_SET (Val, PCIE_CAP_MAX_LINK_WIDTH_X2);
      break;
    case LNKW_X4:
      Val = PCIE_CAP_MAX_LINK_WIDTH_SET (Val, PCIE_CAP_MAX_LINK_WIDTH_X4);
      break;
    case LNKW_X8:
      Val = PCIE_CAP_MAX_LINK_WIDTH_SET (Val, PCIE_CAP_MAX_LINK_WIDTH_X8);
      break;
    case LNKW_X16:
    default:
      Val = PCIE_CAP_MAX_LINK_WIDTH_SET (Val, PCIE_CAP_MAX_LINK_WIDTH_X16);
      break;
    };

    switch (RC->Pcie[PcieIndex].MaxGen) {
    case SPEED_GEN1:
      Val = PCIE_CAP_MAX_LINK_SPEED_SET(Val, MAX_LINK_SPEED_25);
      break;
    case SPEED_GEN2:
      Val = PCIE_CAP_MAX_LINK_SPEED_SET(Val, MAX_LINK_SPEED_50);
      break;
    case SPEED_GEN3:
      Val = PCIE_CAP_MAX_LINK_SPEED_SET(Val, MAX_LINK_SPEED_80);
      break;
    default:
      Val = PCIE_CAP_MAX_LINK_SPEED_SET(Val, MAX_LINK_SPEED_160);
      break;
    };
    Ac01PcieCsrOut32 (CfgAddr + LINK_CAPABILITIES_REG, Val);

    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, &Val);
    switch (RC->Pcie[PcieIndex].MaxGen) {
    case SPEED_GEN1:
      Val = PCIE_CAP_TARGET_LINK_SPEED_SET(Val, MAX_LINK_SPEED_25);
      break;
    case SPEED_GEN2:
      Val = PCIE_CAP_TARGET_LINK_SPEED_SET(Val, MAX_LINK_SPEED_50);
      break;
    case SPEED_GEN3:
      Val = PCIE_CAP_TARGET_LINK_SPEED_SET(Val, MAX_LINK_SPEED_80);
      break;
    default:
      Val = PCIE_CAP_TARGET_LINK_SPEED_SET(Val, MAX_LINK_SPEED_160);
      break;
    };
    Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, Val);

    // Set Zero byte request handling
    Ac01PcieCsrIn32 (CfgAddr + FILTER_MASK_2_OFF, &Val);
    Val = CX_FLT_MASK_VENMSG0_DROP_SET (Val, 0);
    Val = CX_FLT_MASK_VENMSG1_DROP_SET (Val, 0);
    Val = CX_FLT_MASK_DABORT_4UCPL_SET (Val, 0);
    Ac01PcieCsrOut32 (CfgAddr + FILTER_MASK_2_OFF, Val);
    Ac01PcieCsrIn32 (CfgAddr + AMBA_ORDERING_CTRL_OFF, &Val);
    Val = AX_MSTR_ZEROLREAD_FW_SET (Val, 0);
    Ac01PcieCsrOut32 (CfgAddr + AMBA_ORDERING_CTRL_OFF, Val);

    //
    // Set Completion with CRS handling for CFG Request
    // Set Completion with CA/UR handling non-CFG Request
    //
    Ac01PcieCsrIn32 (CfgAddr + AMBA_ERROR_RESPONSE_DEFAULT_OFF, &Val);
    Val = AMBA_ERROR_RESPONSE_CRS_SET (Val, 0x2);
    Ac01PcieCsrOut32 (CfgAddr + AMBA_ERROR_RESPONSE_DEFAULT_OFF, Val);

    // Set Legacy PCIE interrupt map to INTA
    Ac01PcieCsrIn32 (CfgAddr + BRIDGE_CTRL_INT_PIN_INT_LINE_REG, &Val);
    Val = INT_PIN_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + BRIDGE_CTRL_INT_PIN_INT_LINE_REG, Val);
    Ac01PcieCsrIn32 (CsrAddr + IRQSEL, &Val);
    Val = INTPIN_SET (Val, 1);
    Ac01PcieCsrOut32 (CsrAddr + IRQSEL, Val);

    if (RC->Pcie[PcieIndex].MaxGen != SPEED_GEN1) {
      Ac01PcieConfigureEqualization (RC, PcieIndex);
      if (RC->Pcie[PcieIndex].MaxGen == SPEED_GEN3)
        Ac01PcieConfigurePresetGen3 (RC, PcieIndex);
      else if (RC->Pcie[PcieIndex].MaxGen == SPEED_GEN4)
        Ac01PcieConfigurePresetGen4 (RC, PcieIndex);
    }

    // Mask Completion Timeout
    Ac01PcieCsrIn32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, &Val);
    Val = LINK_TIMEOUT_PERIOD_DEFAULT_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, Val);
    Ac01PcieCsrIn32 (CfgAddr + UNCORR_ERR_MASK_OFF, &Val);
    Val = CMPLT_TIMEOUT_ERR_MASK_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + UNCORR_ERR_MASK_OFF, Val);

    // Program Class Code
    Ac01PcieCsrIn32 (CfgAddr + TYPE1_CLASS_CODE_REV_ID_REG, &Val);
    Val = REVISION_ID_SET (Val, 4);
    Val = SUBCLASS_CODE_SET (Val, 4);
    Val = BASE_CLASS_CODE_SET (Val, 6);
    Ac01PcieCsrOut32 (CfgAddr + TYPE1_CLASS_CODE_REV_ID_REG, Val);

    // Program VendorID and DeviceID
    Ac01PcieCsrIn32 (CfgAddr + TYPE1_DEV_ID_VEND_ID_REG, &Val);
    Val = VENDOR_ID_SET (Val, AMPERE_PCIE_VENDORID);
    Val = DEVICE_ID_SET (Val, AC01_PCIE_BRIDGE_DEVICEID);
    Ac01PcieCsrOut32 (CfgAddr + TYPE1_DEV_ID_VEND_ID_REG, Val);

    // Enable common clock for downstream
    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
    Val = PCIE_CAP_SLOT_CLK_CONFIG_SET (Val, 1);
    Val = PCIE_CAP_COMMON_CLK_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, Val);

    // Assert PERST low to reset endpoint
    PcieBoardAssertPerst (RC, PcieIndex, 0, FALSE);

    // Start link training
    Ac01PcieCsrIn32 (CsrAddr + LINKCTRL, &Val);
    Val = LTSSMENB_SET (Val, 1);
    Ac01PcieCsrOut32 (CsrAddr + LINKCTRL, Val);

    // Complete the PERST pulse
    PcieBoardAssertPerst (RC, PcieIndex, 0, TRUE);

    // Match aux_clk to system
    Ac01PcieCsrIn32 (CfgAddr + AUX_CLK_FREQ_OFF, &Val);
    Val = AUX_CLK_FREQ_SET (Val, AUX_CLK_500MHZ);
    Ac01PcieCsrOut32 (CfgAddr + AUX_CLK_FREQ_OFF, Val);

    // Lock programming of config space
    Ac01PcieCsrIn32 (CfgAddr + MISC_CONTROL_1_OFF, &Val);
    Val = DBI_RO_WR_EN_SET (Val, 0);
    Ac01PcieCsrOut32 (CfgAddr + MISC_CONTROL_1_OFF, Val);

    if (ReInit == 1)
      break;
  }

  // Program VendorID and DeviceId
  if (!EFI_ERROR (SMProRegRd (RC->Socket, RC->HBAddr  + HBPDVIDR, &Val))) {
    Val = PCIVENDID_SET (Val, AMPERE_PCIE_VENDORID);
    Val = PCIDEVID_SET (Val, AMPERE_PCIE_DEVICEID);
    SMProRegWr (RC->Socket, RC->HBAddr  + HBPDVIDR, Val);
  }

  return 0;
}

STATIC BOOLEAN PcieLinkUpCheck (
  IN AC01_PCIE *Pcie,
  OUT UINT32 *LtssmState
  )
{
  VOID*         CsrAddr;
  UINT32        BlockEvent, LinkStat;

  CsrAddr = (VOID*) Pcie->CsrAddr;

  // Check if card present
  // smlh_ltssm_state[13:8] = 0
  // phy_status[2] = 0
  // smlh_link_up[1] = 0
  // rdlh_link_up[0] = 0
  Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
  LinkStat = LinkStat & (SMLH_LTSSM_STATE_MASK | PHY_STATUS_MASK_BIT |
                         SMLH_LINK_UP_MASK_BIT |RDLH_LINK_UP_MASK_BIT);
  if (LinkStat == 0x0000) {
    return 0;
  }

   Ac01PcieCsrIn32 (CsrAddr +  BLOCKEVENTSTAT, &BlockEvent);
   Ac01PcieCsrIn32 (CsrAddr +  LINKSTAT, &LinkStat);

   if (BlockEvent & LINKUP_MASK) {
     *LtssmState = SMLH_LTSSM_STATE_GET(LinkStat);
     PCIE_DEBUG ("%a *LtssmState=%lx Linkup\n", __func__, *LtssmState);
     return 1;
   }

   return 0;
}

VOID Ac01PcieCoreUpdateLink (
  IN AC01_RC  *RC,
  OUT BOOLEAN *IsNextRoundNeeded,
  OUT INT8    *FailedPciePtr,
  OUT INT8    *FailedPcieCount
  )
{
  INTN                      PcieIndex;
  AC01_PCIE                *Pcie;
  UINT32                    Ltssm, i;
  UINT32                    Val;
  VOID                     *CfgAddr;

  *IsNextRoundNeeded = FALSE;
  *FailedPcieCount   = 0;
  for (i = 0; i < MAX_PCIE_B; i++)
    FailedPciePtr[i] = -1;

  if (!RC->Active)
    return;

  // Loop for all controllers
  for (PcieIndex = 0; PcieIndex < RC->MaxPcieController; PcieIndex++) {
    Pcie = &RC->Pcie[PcieIndex];
    CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

    if (Pcie->Active && !Pcie->LinkUp) {
      if (PcieLinkUpCheck (Pcie, &Ltssm)) {
        Pcie->LinkUp = 1;
        Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
        PCIE_DEBUG ("%a S%d RC%d RP%d NEGO_LINK_WIDTH: 0x%x LINK_SPEED: 0x%x\n",
                    __func__, RC->Socket, RC->ID, PcieIndex,
                    PCIE_CAP_NEGO_LINK_WIDTH_GET(Val),
                    PCIE_CAP_LINK_SPEED_GET(Val)
                    );

        // Un-mask Completion Timeout
        Ac01PcieCsrIn32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, &Val);
        Val = LINK_TIMEOUT_PERIOD_DEFAULT_SET (Val, 32);
        Ac01PcieCsrOut32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, Val);
        Ac01PcieCsrIn32 (CfgAddr + UNCORR_ERR_MASK_OFF, &Val);
        Val = CMPLT_TIMEOUT_ERR_MASK_SET (Val, 0);
        Ac01PcieCsrOut32 (CfgAddr + UNCORR_ERR_MASK_OFF, Val);

        // Post link up, verify link width and speed,
        // if link does not attain desired width and speed, retrain link.
        Ac01PcieCoreQoSLinkCheckRecovery (RC, PcieIndex);
      } else {
        *IsNextRoundNeeded = TRUE;
        FailedPciePtr[*FailedPcieCount] = PcieIndex;
        *FailedPcieCount += 1;
      }
    }
  }
}

VOID Ac01PcieCoreEndEnumeration (AC01_RC *RC)
{
  //
  // Reserved for hook from stack ending of enumeration phase processing.
  // Emptry for now.
  //
}

/**
   Retrain the link to the desired speed

   @param RC          Pointer to AC01_RC structure
   @param PcieIndex   PCIe index
   @param Speed       The target link speed for retraining
   @retval            -1: Retraing had failed
                       0: Retraining success
**/
INT32
PcieRetrainLink (
  IN AC01_RC   *RC,
  IN UINT32    PcieIndex,
  IN UINT32    Speed
  )
{
  INT32       Ret = LINK_RETRAIN_SUCCESS;
  VOID        *CsrAddr, *CfgAddr, *SpcieBaseAddr, *Pl16BaseAddr;
  INTN        TimeOut;
  UINT32      Val, LinkStat;

  CsrAddr = (VOID*) RC->Pcie[PcieIndex].CsrAddr;
  CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  // Allow programming to config space
  Ac01PcieCsrIn32 (CfgAddr + MISC_CONTROL_1_OFF, &Val);
  Val = DBI_RO_WR_EN_SET (Val, 1);
  Ac01PcieCsrOut32 (CfgAddr + MISC_CONTROL_1_OFF, Val);

  // Generate the SPCIE capability address
  // SPCIE_CAP_ID = 0x19
  SpcieBaseAddr = (VOID *) PcieCheckCap (RC, PcieIndex, 0x1, SPCIE_CAP_ID);

  if (SpcieBaseAddr == 0) {
    PCIE_ERR ("PCIE%d.%d: Cannot get SPCIE capability address\n",
              RC->ID, PcieIndex);
    return LINK_RETRAIN_FAILED;
  }

  // Performing Equalization
  Ac01PcieCsrIn32 (SpcieBaseAddr + LINK_CONTROL3_REG_OFF, &Val);
  Val = Val | 0x1;
  Ac01PcieCsrOut32 (SpcieBaseAddr + LINK_CONTROL3_REG_OFF, Val);

  Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, &Val);
  Val = PCIE_CAP_TARGET_LINK_SPEED_SET (Val, 0x3);
  Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, Val);

  // Check GEN3 equalization phase 1/2/3 are set.
  // Check link is not in GEN4
  // The 8.0 GT/s equalization procedure is deemed to have been successfully executed if the
  // Equalization 8.0 GT/s Phase 3 Successful bit and Equalization 8.0 GT/s
  // Complete bit of the Link Status 2 register are both set to 1b.
  //
  // Trigger Link Retraining
  Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
  Val = PCIE_CAP_RETRAIN_LINK_SET (Val, 1);
  Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, Val);

  TimeOut = PCIE_RETRAIN_TRANSITION_TIMEOUT;
  do {
    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
    TimeOut--;
    if (PCIE_CAP_LINK_TRAINING_GET (Val) == 0) {
        break;
    }

    MicroSecondDelay (1);
  } while (TimeOut > 0);

  if (TimeOut <= 0) {
    Ret = LINK_RETRAIN_FAILED;
  }

  TimeOut = PCIE_RETRAIN_TRANSITION_TIMEOUT;
  do {
    Ac01PcieCfgIn32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, &Val);
    TimeOut--;
    if (PCIE_CAP_EQ_CPL_GET(Val) && PCIE_CAP_EQ_CPL_P3_GET(Val)) {
      break;
    }
    MicroSecondDelay (1);
  } while (TimeOut > 0);

  if (TimeOut <= 0) {
    Ret = LINK_RETRAIN_FAILED;
  }

  if ((Speed == MAX_LINK_SPEED_160) && TimeOut > 0) {
    // Performing Equalization
    Ac01PcieCsrIn32 (SpcieBaseAddr + LINK_CONTROL3_REG_OFF, &Val);
    Val = Val | 0x1;
    Ac01PcieCsrOut32 (SpcieBaseAddr + LINK_CONTROL3_REG_OFF, Val);

    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, &Val);
    Val = PCIE_CAP_TARGET_LINK_SPEED_SET (Val, Speed);
    Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL2_LINK_STATUS2_REG, Val);

    // Trigger Link Retraining
    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
    Val = PCIE_CAP_RETRAIN_LINK_SET (Val, 1);
    Ac01PcieCsrOut32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, Val);

    TimeOut = PCIE_RETRAIN_TRANSITION_TIMEOUT;
    do {
      Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
      TimeOut--;
      if (PCIE_CAP_LINK_TRAINING_GET (Val) == 0)
        break;
      MicroSecondDelay (1);
    } while (TimeOut > 0);

    // Generate the PL16 capability address
    Pl16BaseAddr = (VOID *) PcieCheckCap (RC, PcieIndex, 0x1, PL16_CAP_ID);
    if (Pl16BaseAddr == 0) {
      PCIE_ERR ("PCIE%d.%d: Cannot get PL16 capability address\n",
                RC->ID, PcieIndex);
      return LINK_RETRAIN_FAILED;
    }

    TimeOut = PCIE_RETRAIN_TRANSITION_TIMEOUT;
    do {
      Ac01PcieCfgIn32 (Pl16BaseAddr + PL16G_STATUS_REG_OFF, &Val);
      TimeOut--;
      if (PL16G_STATUS_EQ_CPL_GET(Val) && PL16G_STATUS_EQ_CPL_P3_GET(Val))
          break;
      MicroSecondDelay (1);
    } while (TimeOut > 0);

    if (TimeOut <= 0) {
      Ret = LINK_RETRAIN_FAILED;
    }
  }

  Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
  PCIE_DEBUG ("PCIE%d.%d: LINKSTAT 0x%x\n", RC->ID, PcieIndex, LinkStat);
  return Ret;
}

/**
   Upon link up, if link does not attain desired width and speed retrain link.
   If link width is degraded then, trigger soft reset
   If link speed is degraded then, retrain link to GEN1 and then to desired speed

   @param RC            Pointer to AC01_RC structure
   @param PcieIndex     PCIe index
   @param EpMaxWidth    EP max link width
   @param EpMaxGen      EP max link speed
   @retval              -1: Retraing had failed
                         1: Link width and speed are not correct
                         0: Retraining success
**/
INT32
Ac01PcieCoreLinkRetraining (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex,
  IN UINT8     EpMaxWidth,
  IN UINT8     EpMaxGen
  )
{
  VOID         *CsrAddr, *CfgAddr;
  UINT32        Val, LinkStat;
  INT32         NumberOfRetrain = MAX_RETRAIN; // TODO: optimize retry count
  UINT32        MaxWidth = 0, MaxGen = 0;

  CsrAddr = (VOID*) RC->Pcie[PcieIndex].CsrAddr;
  CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  if (!RC->Pcie[PcieIndex].LinkUp) {
     return LINK_RETRAIN_FAILED;
  }

  Ac01PcieCsrIn32 (CfgAddr + LINK_CAPABILITIES_REG, &Val);

  if ((PCIE_CAP_MAX_LINK_WIDTH_GET (Val) == 0) ||
      (PCIE_CAP_MAX_LINK_SPEED_GET (Val) == 0)) {
    return LINK_RETRAIN_WRONG_PARAMETER;
  }

  if ((EpMaxWidth == 0) || (EpMaxGen == 0)) {
    return LINK_RETRAIN_WRONG_PARAMETER;
  }

  // Compare RC and EP capabilities
  if (PCIE_CAP_MAX_LINK_WIDTH_GET (Val) > EpMaxWidth) {
    MaxWidth = EpMaxWidth;
  } else {
    MaxWidth = PCIE_CAP_MAX_LINK_WIDTH_GET (Val);
  }

  // Compare RC and EP capabilities
  if (PCIE_CAP_MAX_LINK_SPEED_GET (Val) > EpMaxGen) {
    MaxGen = EpMaxGen;
  } else {
    MaxGen = PCIE_CAP_MAX_LINK_SPEED_GET (Val);
  }

  PCIE_DEBUG ("PCIE%d.%d: Link Retrain MaxWidth %d MaxGen %d\n", RC->ID,
              PcieIndex, MaxWidth, MaxGen);

retry_retrain:
  NumberOfRetrain--;
  if (NumberOfRetrain >= 0) {
    Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
    Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);

    // if link width is not correct, return failed right away
    // and fast-foward the retraining to go to soft reset
    if ((PCIE_CAP_NEGO_LINK_WIDTH_GET (Val) != MaxWidth) ||
        (RDLH_SMLH_LINKUP_STATUS_GET (LinkStat) != 0x3)) {
      return LINK_RETRAIN_FAILED;
    }

    // 0x1103 stand for LTSSM state at L0 (0x11) and Link is up (0x03)
    // If link speed is not equal to Max speed and LTSSM state is not L0,
    // then retraining sequence will be called
    if ((PCIE_CAP_LINK_SPEED_GET (Val) != MaxGen)  ||
        (RDLH_SMLH_LINKUP_STATUS_GET (LinkStat) != 0x3)) {

      if (PcieRetrainLink (RC, PcieIndex, MaxGen) == LINK_RETRAIN_FAILED) {
        goto retry_retrain;
      }

      // After retraining, wait until link is healthy
      MicroSecondDelay (100);

      Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
      Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
      PCIE_DEBUG ("PCIE%d.%d RC Width %d RC Speed %d, ", RC->ID, PcieIndex,
                  PCIE_CAP_NEGO_LINK_WIDTH_GET (Val), PCIE_CAP_LINK_SPEED_GET (Val));

      if ((PCIE_CAP_LINK_SPEED_GET (Val) == MaxGen) &&
          (RDLH_SMLH_LINKUP_STATUS_GET (LinkStat) == 0x3)) {
        PCIE_DEBUG (" Link Retrain Passed!!!\n");
        return LINK_RETRAIN_SUCCESS;
      } else {
        PCIE_DEBUG (" Link Retrain Failed!!!\n");
        Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
        Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
        return LINK_RETRAIN_FAILED;
      }
    } else {
      /* FIXME: Why this delay? */
      MicroSecondDelay (100);

      Ac01PcieCsrIn32 (CsrAddr + LINKSTAT, &LinkStat);
      Ac01PcieCsrIn32 (CfgAddr + LINK_CONTROL_LINK_STATUS_REG, &Val);
      PCIE_DEBUG ("PCIE%d.%d RC Width %d RC Speed %d, ", RC->ID, PcieIndex,
                  PCIE_CAP_NEGO_LINK_WIDTH_GET (Val), PCIE_CAP_LINK_SPEED_GET (Val));
    }
  }
  return LINK_RETRAIN_SUCCESS;
}

/**
   Get link capabilities link width and speed of endpoint

   @param  RC          Pointer to AC01_RC structure
   @param  PcieIndex   PCIe index
   @retval EpMaxWidth  EP max link width
   @retval EpMaxGen    EP max link speed
**/
VOID
Ac01PcieCoreSyncEndpointInfo (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex,
  OUT UINT8    *EpMaxWidth,
  OUT UINT8    *EpMaxGen
  )
{
  VOID         *RcCfgAddr, *PcieCapBaseAddr, *EpCfgAddr;
  UINT32       Val = 0;

  RcCfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

  // Allow programming to config space
  Ac01PcieCsrIn32 (RcCfgAddr + MISC_CONTROL_1_OFF, &Val);
  Val = DBI_RO_WR_EN_SET (Val, 1);
  Ac01PcieCsrOut32 (RcCfgAddr + MISC_CONTROL_1_OFF, Val);

  Ac01PcieCsrIn32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, &Val);
  Val = SUB_BUS_SET (Val, RC->Pcie[PcieIndex].DevNum);
  Ac01PcieCsrOut32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, Val);

  Ac01PcieCsrIn32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, &Val);
  Val = SEC_BUS_SET (Val, RC->Pcie[PcieIndex].DevNum);
  Ac01PcieCsrOut32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, Val);

  Ac01PcieCsrIn32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, &Val);
  Val = PRIM_BUS_SET (Val, 0x0);
  Ac01PcieCsrOut32 (RcCfgAddr + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, Val);

  EpCfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 20));
  Ac01PcieCsrIn32 (EpCfgAddr, &Val);

  // Check whether EP config space is accessible or not
  if (Val == 0xFFFFFFFF) {
    *EpMaxWidth = 0;   // Invalid Width
    *EpMaxGen = 0;     // Invalid Speed
    PCIE_ERR ("PCIE%d.%d Cannot access EP config space!\n", RC->ID, PcieIndex);
  } else {
    PcieCapBaseAddr = (VOID *)PcieCheckCap (RC, PcieIndex, 0x0, PCIE_CAP_ID);
    if (PcieCapBaseAddr == 0) {
      *EpMaxWidth = 0;   // Invalid Width
      *EpMaxGen = 0;     // Invalid Speed
      PCIE_ERR ("PCIE%d.%d Cannot get PCIe capability extended address!\n",
                RC->ID, PcieIndex);
    } else {
      Ac01PcieCsrIn32 (PcieCapBaseAddr + LINK_CAPABILITIES_REG_OFF, &Val);
      *EpMaxWidth = (Val >> 4) & 0x3F;
      *EpMaxGen = Val & 0xF;
      PCIE_DEBUG ("PCIE%d.%d EP MaxWidth %d EP MaxGen %d \n", RC->ID,
                  PcieIndex, *EpMaxWidth, *EpMaxGen);

      // Enable common clock for Upstream
      Ac01PcieCsrIn32 (PcieCapBaseAddr + LINK_CONTROL_LINK_STATUS_OFF, &Val);
      Val = PCIE_CAP_SLOT_CLK_CONFIG_SET (Val, 1);
      Val = PCIE_CAP_COMMON_CLK_SET (Val, 1);
      Ac01PcieCsrOut32 (PcieCapBaseAddr + LINK_CONTROL_LINK_STATUS_OFF, Val);

    }
  }

  // Lock programming of config space
  Ac01PcieCsrIn32 (RcCfgAddr + MISC_CONTROL_1_OFF, &Val);
  Val = DBI_RO_WR_EN_SET (Val, 0);
  Ac01PcieCsrOut32 (RcCfgAddr + MISC_CONTROL_1_OFF, Val);
}

/**
   Check active pcie controllers of RC, retrain or soft reset if needed

   @param RC    Pointer to AC01_RC structure
   @retval      -1: Link recovery had failed
                 1: Link width and speed are not correct
                 0: Link recovery succeed
**/
INT32
Ac01PcieCoreQoSLinkCheckRecovery (
  IN AC01_RC   *RC,
  IN INTN      PcieIndex
  )
{
  VOID*       CfgAddr;
  UINT32      Val, Ltssm;
  INT32       NumberOfReset; // Number of soft reset retry
  UINT8       EpMaxWidth = 0, EpMaxGen = 0;

  // PCIe controller is not active or Link is not up
  // Nothing to be done
  if ((!RC->Pcie[PcieIndex].Active) || (!RC->Pcie[PcieIndex].LinkUp)) {
    return LINK_RETRAIN_WRONG_PARAMETER;
  }

  Ac01PcieCoreSyncEndpointInfo (RC, PcieIndex, &EpMaxWidth, &EpMaxGen);

  NumberOfReset = MAX_RETRAIN;
  while (Ac01PcieCoreLinkRetraining (RC, PcieIndex, EpMaxWidth, EpMaxGen) == LINK_RETRAIN_FAILED) {
    NumberOfReset--;
    if (NumberOfReset >= 0) {
      PCIE_DEBUG ("PCIE%d.%d Start link re-initialization..\n", RC->ID, PcieIndex);
      CfgAddr = (VOID*) (RC->MmcfgAddr + (RC->Pcie[PcieIndex].DevNum << 15));

      if (PcieLinkUpCheck (&RC->Pcie[PcieIndex], &Ltssm)) {
        RC->Pcie[PcieIndex].LinkUp = 1;
        // Un-mask Completion Timeout
        Ac01PcieCsrIn32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, &Val);
        Val = LINK_TIMEOUT_PERIOD_DEFAULT_SET (Val, 32);
        Ac01PcieCsrOut32 (CfgAddr + AMBA_LINK_TIMEOUT_OFF, Val);
        Ac01PcieCsrIn32 (CfgAddr + UNCORR_ERR_MASK_OFF, &Val);
        Val = CMPLT_TIMEOUT_ERR_MASK_SET (Val, 0);
        Ac01PcieCsrOut32 (CfgAddr + UNCORR_ERR_MASK_OFF, Val);
        PCIE_ERR ("PCIE%d.%d Link re-initialization passed!\n", RC->ID,
                  PcieIndex);
      } else {
        RC->Pcie[PcieIndex].LinkUp = 0;
      }
    } else {
      PCIE_ERR ("PCIE%d.%d Link re-initialization failed!\n", RC->ID,
                PcieIndex);
      return LINK_RETRAIN_FAILED;
    }
  }

  return LINK_RETRAIN_SUCCESS;
}

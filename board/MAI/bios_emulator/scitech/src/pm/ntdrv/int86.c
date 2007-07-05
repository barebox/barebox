/****************************************************************************
*
*                   SciTech OS Portability Manager Library
*
*  ========================================================================
*
*    The contents of this file are subject to the SciTech MGL Public
*    License Version 1.0 (the "License"); you may not use this file
*    except in compliance with the License. You may obtain a copy of
*    the License at http://www.scitechsoft.com/mgl-license.txt
*
*    Software distributed under the License is distributed on an
*    "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*    implied. See the License for the specific language governing
*    rights and limitations under the License.
*
*    The Original Code is Copyright (C) 1991-1998 SciTech Software, Inc.
*
*    The Initial Developer of the Original Code is SciTech Software, Inc.
*    All Rights Reserved.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  32-bit Windows NT device drivers.
*
* Description:  Implementation for the real mode software interrupt
*               handling functions.
*
****************************************************************************/

#include "pmapi.h"
#include "drvlib/os/os.h"
#include "sdd/sddhelp.h"
#include "mtrr.h"
#include "oshdr.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
REMARKS:
We do have limited BIOS access under Windows NT device drivers.
****************************************************************************/
ibool PMAPI PM_haveBIOSAccess(void)
{
    /* Return false unless we have full buffer passing! */
    return false;
}

/****************************************************************************
PARAMETERS:
len     - Place to store the length of the buffer
rseg    - Place to store the real mode segment of the buffer
roff    - Place to store the real mode offset of the buffer

REMARKS:
This function returns the address and length of the global VESA transfer
buffer that is used for communicating with the VESA BIOS functions from
Win16 and Win32 programs under Windows.
****************************************************************************/
void * PMAPI PM_getVESABuf(
    uint *len,
    uint *rseg,
    uint *roff)
{
    /* No buffers supported under Windows NT (Windows XP has them however if */
    /* we ever decide to support this!) */
    return NULL;
}

/****************************************************************************
REMARKS:
Issue a protected mode software interrupt.
****************************************************************************/
int PMAPI PM_int386(
    int intno,
    PMREGS *in,
    PMREGS *out)
{
    PMSREGS sregs;
    PM_segread(&sregs);
    return PM_int386x(intno,in,out,&sregs);
}

/****************************************************************************
REMARKS:
Map a real mode pointer to a protected mode pointer.
****************************************************************************/
void * PMAPI PM_mapRealPointer(
    uint r_seg,
    uint r_off)
{
    /* Not used for Windows NT drivers! */
    return NULL;
}

/****************************************************************************
REMARKS:
Allocate a block of real mode memory
****************************************************************************/
void * PMAPI PM_allocRealSeg(
    uint size,
    uint *r_seg,
    uint *r_off)
{
    /* Not supported in NT drivers */
    (void)size;
    (void)r_seg;
    (void)r_off;
    return NULL;
}

/****************************************************************************
REMARKS:
Free a block of real mode memory.
****************************************************************************/
void PMAPI PM_freeRealSeg(
    void *mem)
{
    /* Not supported in NT drivers */
    (void)mem;
}

/****************************************************************************
REMARKS:
Issue a real mode interrupt (parameters in DPMI compatible structure)
****************************************************************************/
void PMAPI DPMI_int86(
    int intno,
    DPMI_regs *regs)
{
    /* Not used in NT drivers */
}

/****************************************************************************
REMARKS:
Call a V86 real mode function with the specified register values
loaded before the call. The call returns with a far ret.
****************************************************************************/
void PMAPI PM_callRealMode(
    uint seg,
    uint off,
    RMREGS *regs,
    RMSREGS *sregs)
{
    /* TODO!! */
}

/****************************************************************************
REMARKS:
Issue a V86 real mode interrupt with the specified register values
loaded before the interrupt.
****************************************************************************/
int PMAPI PM_int86(
    int intno,
    RMREGS *in,
    RMREGS *out)
{
    /* TODO!! */
    *out = *in;
    return out->x.ax;
}

/****************************************************************************
REMARKS:
Issue a V86 real mode interrupt with the specified register values
loaded before the interrupt.
****************************************************************************/
int PMAPI PM_int86x(
    int intno,
    RMREGS *in,
    RMREGS *out,
    RMSREGS *sregs)
{
    /* TODO!! */
    *out = *in;
    return out->x.ax;
}

/*
 * File:    dma_utils.c
 * Purpose: General purpose utilities for the multi-channel DMA
 *
 * Notes:   The methodology used in these utilities assumes that
 *          no single initiator will be tied to more than one
 *          task/channel
 */

#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <mach/mcf54xx-regs.h>
#include <proc/mcdapi/MCD_dma.h>

#include <proc/dma_utils.h>

/*
 * This global keeps track of which initiators have been
 * used of the available assignments.  Initiators 0-15 are
 * hardwired.  Initiators 16-31 are multiplexed and controlled
 * via the Initiatior Mux Control Registe (IMCR).  The
 * assigned requestor is stored with the associated initiator
 * number.
 */
static int8_t used_reqs[32] =
{
    DMA_ALWAYS,  DMA_DSPI_RX, DMA_DSPI_TX, DMA_DREQ0,
    DMA_PSC0_RX, DMA_PSC0_TX, DMA_USBEP0,  DMA_USBEP1,
    DMA_USBEP2,  DMA_USBEP3,  DMA_PCI_TX,  DMA_PCI_RX,
    DMA_PSC1_RX, DMA_PSC1_TX, DMA_I2C_RX,  DMA_I2C_TX,
    0,           0,           0,           0,
    0,           0,           0,           0,
    0,           0,           0,           0,
    0,           0,           0,           0
};

/*
 * This global keeps track of which channels have been assigned
 * to tasks.  This methology assumes that no single initiator
 * will be tied to more than one task/channel
 */
typedef struct
{
    int     req;
    void    (*handler)(void);
} DMA_CHANNEL_STRUCT;

static DMA_CHANNEL_STRUCT dma_channel[NCHANNELS] =
{
    {-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL},
    {-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL},
    {-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL},
    {-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL}
};

/*
 * Enable all DMA interrupts
 *
 * Parameters:
 *  pri     Interrupt Priority
 *  lvl     Interrupt Level
 */
void
dma_irq_enable(uint8_t lvl, uint8_t pri)
{
//FIXME    ASSERT(lvl > 0 && lvl < 8);
//FIXME    ASSERT(pri < 8);

    /* Setup the DMA ICR (#48) */
    MCF_INTC_ICR48 = 0
        | MCF_INTC_ICRn_IP(pri)
        | MCF_INTC_ICRn_IL(lvl);

    /* Unmask all task interrupts */
    MCF_DMA_DIMR = 0;

    /* Clear the interrupt pending register */
    MCF_DMA_DIPR = 0;

    /* Unmask the DMA interrupt in the interrupt controller */
    MCF_INTC_IMRH &= ~MCF_INTC_IMRH_INT_MASK48;
}

/*
 * Disable all DMA interrupts
 */
void
dma_irq_disable(void)
{
    /* Mask all task interrupts */
    MCF_DMA_DIMR = (uint32_t)~0;

    /* Clear any pending task interrupts */
    MCF_DMA_DIPR = (uint32_t)~0;

    /* Mask the DMA interrupt in the interrupt controller */
    MCF_INTC_IMRH |= MCF_INTC_IMRH_INT_MASK48;
}

/*
 * Attempt to enable the provided Initiator in the Initiator
 * Mux Control Register
 *
 * Parameters:
 *  initiator   Initiator identifier
 *
 * Return Value:
 *  1   if unable to make the assignment
 *  0   successful
 */
int
dma_set_initiator(int initiator)
{
    switch (initiator)
    {
        /* These initiators are always active */
        case DMA_ALWAYS:
        case DMA_DSPI_RX:
        case DMA_DSPI_TX:
        case DMA_DREQ0:
        case DMA_PSC0_RX:
        case DMA_PSC0_TX:
        case DMA_USBEP0:
        case DMA_USBEP1:
        case DMA_USBEP2:
        case DMA_USBEP3:
        case DMA_PCI_TX:
        case DMA_PCI_RX:
        case DMA_PSC1_RX:
        case DMA_PSC1_TX:
        case DMA_I2C_RX:
        case DMA_I2C_TX:
            break;
        case DMA_FEC0_RX:
            MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC16(3))
                            | MCF_DMA_IMCR_SRC16_FEC0RX;
            used_reqs[16] = DMA_FEC0_RX;
            break;
        case DMA_FEC0_TX:
            MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC17(3))
                            | MCF_DMA_IMCR_SRC17_FEC0TX;
            used_reqs[17] = DMA_FEC0_TX;
            break;
        case DMA_FEC1_RX:
            MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC20(3))
                            | MCF_DMA_IMCR_SRC20_FEC1RX;
            used_reqs[20] = DMA_FEC1_RX;
            break;
        case DMA_FEC1_TX:
            if (used_reqs[21] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC21(3))
                                | MCF_DMA_IMCR_SRC21_FEC1TX;
                used_reqs[21] = DMA_FEC1_TX;
            }
            else if (used_reqs[25] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC25(3))
                                | MCF_DMA_IMCR_SRC25_FEC1TX;
                used_reqs[25] = DMA_FEC1_TX;
            }
            else if (used_reqs[31] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC31(3))
                                | MCF_DMA_IMCR_SRC31_FEC1TX;
                used_reqs[31] = DMA_FEC1_TX;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_DREQ1:
            if (used_reqs[29] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC29(3))
                                | MCF_DMA_IMCR_SRC29_DREQ1;
                used_reqs[29] = DMA_DREQ1;
            }
            else if (used_reqs[21] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC21(3))
                                | MCF_DMA_IMCR_SRC21_DREQ1;
                used_reqs[21] = DMA_DREQ1;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM0:
            if (used_reqs[24] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC24(3))
                                | MCF_DMA_IMCR_SRC24_CTM0;
                used_reqs[24] = DMA_CTM0;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM1:
            if (used_reqs[25] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC25(3))
                                | MCF_DMA_IMCR_SRC25_CTM1;
                used_reqs[25] = DMA_CTM1;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM2:
            if (used_reqs[26] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC26(3))
                                | MCF_DMA_IMCR_SRC26_CTM2;
                used_reqs[26] = DMA_CTM2;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM3:
            if (used_reqs[27] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC27(3))
                                | MCF_DMA_IMCR_SRC27_CTM3;
                used_reqs[27] = DMA_CTM3;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM4:
            if (used_reqs[28] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC28(3))
                                | MCF_DMA_IMCR_SRC28_CTM4;
                used_reqs[28] = DMA_CTM4;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM5:
            if (used_reqs[29] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC29(3))
                                | MCF_DMA_IMCR_SRC29_CTM5;
                used_reqs[29] = DMA_CTM5;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM6:
            if (used_reqs[30] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC30(3))
                                | MCF_DMA_IMCR_SRC30_CTM6;
                used_reqs[30] = DMA_CTM6;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_CTM7:
            if (used_reqs[31] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC31(3))
                                | MCF_DMA_IMCR_SRC31_CTM7;
                used_reqs[31] = DMA_CTM7;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_USBEP4:
            if (used_reqs[26] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC26(3))
                                | MCF_DMA_IMCR_SRC26_USBEP4;
                used_reqs[26] = DMA_USBEP4;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_USBEP5:
            if (used_reqs[27] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC27(3))
                                | MCF_DMA_IMCR_SRC27_USBEP5;
                used_reqs[27] = DMA_USBEP5;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_USBEP6:
            if (used_reqs[28] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC28(3))
                                | MCF_DMA_IMCR_SRC28_USBEP6;
                used_reqs[28] = DMA_USBEP6;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_PSC2_RX:
            if (used_reqs[28] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC28(3))
                                | MCF_DMA_IMCR_SRC28_PSC2RX;
                used_reqs[28] = DMA_PSC2_RX;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_PSC2_TX:
            if (used_reqs[29] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC29(3))
                                | MCF_DMA_IMCR_SRC29_PSC2TX;
                used_reqs[29] = DMA_PSC2_TX;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_PSC3_RX:
            if (used_reqs[30] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC30(3))
                                | MCF_DMA_IMCR_SRC30_PSC3RX;
                used_reqs[30] = DMA_PSC3_RX;
            }
            else /* No empty slots */
                return 1;
            break;
        case DMA_PSC3_TX:
            if (used_reqs[31] == 0)
            {
                MCF_DMA_IMCR = (MCF_DMA_IMCR & ~MCF_DMA_IMCR_SRC31(3))
                                | MCF_DMA_IMCR_SRC31_PSC3TX;
                used_reqs[31] = DMA_PSC3_TX;
            }
            else /* No empty slots */
                return 1;
            break;
        default:
            return 1;
    }
    return 0;
}

/*
 * Return the initiator number for the given requestor
 *
 * Parameters:
 *  requestor   Initiator/Requestor identifier
 *
 * Return Value:
 *  The initiator number (0-31) if initiator has been assigned
 *  0 (always initiator) otherwise
 */
uint32_t
dma_get_initiator(int requestor)
{
    uint32_t i;

    for (i=0; i<sizeof(used_reqs); ++i)
    {
        if (used_reqs[i] == requestor)
            return i;
    }
    return 0;
}

/*
 * Remove the given initiator from the active list
 *
 * Parameters:
 *  requestor   Initiator/Requestor identifier
 */
void
dma_free_initiator(int requestor)
{
    uint32_t i;

    for (i=16; i<sizeof(used_reqs); ++i)
    {
        if (used_reqs[i] == requestor)
        {
            used_reqs[i] = 0;
            break;
        }
    }
}

/*
 * Attempt to find an available channel and mark it as used
 *
 * Parameters:
 *  requestor   Initiator/Requestor identifier
 *
 * Return Value:
 *  First available channel or -1 if they are all occupied
 */
int
dma_set_channel(int requestor, void (*handler)(void))
{
    int i;

    /* Check to see if this requestor is already assigned to a channel */
    if ((i = dma_get_channel(requestor)) != -1)
        return i;

    for (i=0; i<NCHANNELS; ++i)
    {
        if (dma_channel[i].req == -1)
        {
            dma_channel[i].req = requestor;
            dma_channel[i].handler = handler;
            return i;
        }
    }

    /* All channels taken */
    return -1;
}

/*
 * Return the channel being initiated by the given requestor
 *
 * Parameters:
 *  requestor   Initiator/Requestor identifier
 *
 * Return Value:
 *  Channel that the requestor is controlling or -1 if hasn't been
 *  activated
 */
int
dma_get_channel(int requestor)
{
    uint32_t i;

    for (i=0; i<NCHANNELS; ++i)
    {
        if (dma_channel[i].req == requestor)
            return i;
    }
    return -1;
}

/*
 * Remove the channel being initiated by the given requestor from
 * the active list
 *
 * Parameters:
 *  requestor   Initiator/Requestor identifier
 */
void
dma_free_channel(int requestor)
{
    uint32_t i;

    for (i=0; i<NCHANNELS; ++i)
    {
        if (dma_channel[i].req == requestor)
        {
            dma_channel[i].req = -1;
            dma_channel[i].handler = NULL;
            break;
        }
    }
}

/*
 * This is the catch-all interrupt handler for the mult-channel DMA
 */
int
dma_interrupt_handler (void *arg1, void *arg2)
{
    uint32_t i, interrupts;
    (void)arg1;
    (void)arg2;

    disable_interrupts(); // was: board_irq_disable();

    /*
     * Determine which interrupt(s) triggered by AND'ing the
     * pending interrupts with those that aren't masked.
     */
    interrupts = MCF_DMA_DIPR & ~MCF_DMA_DIMR;

    /* Make sure we are here for a reason */
//    ASSERT(interrupts != 0);

    /* Clear the interrupt in the pending register */
    MCF_DMA_DIPR = interrupts;

    for (i=0; i<16; ++i, interrupts>>=1)
    {
        if (interrupts & 0x1)
        {
            /* If there is a handler, call it */
            if (dma_channel[i].handler != NULL)
                dma_channel[i].handler();
        }
    }

    enable_interrupts(); // board_irq_enable();
    return 1;
}

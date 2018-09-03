/*
* CAAM/SEC 4.x transport/backend driver
* JobR backend functionality
 *
 * Copyright 2008-2015 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <clock.h>
#include <dma.h>
#include <driver.h>
#include <init.h>
#include <linux/barebox-wrapper.h>
#include <linux/spinlock.h>
#include <linux/circ_buf.h>
#include <linux/compiler.h>
#include <linux/err.h>

#include "regs.h"
#include "jr.h"
#include "desc.h"
#include "intern.h"

static int caam_reset_hw_jr(struct device_d *dev)
{
	struct caam_drv_private_jr *jrp = dev->priv;
	uint64_t start;

	/* initiate flush (required prior to reset) */
	wr_reg32(&jrp->rregs->jrcommand, JRCR_RESET);

	start = get_time_ns();
	while ((rd_reg32(&jrp->rregs->jrintstatus) & JRINT_ERR_HALT_MASK) ==
	        JRINT_ERR_HALT_INPROGRESS) {
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(dev, "job ring %d timed out on flush\n",
				jrp->ridx);
			return -ETIMEDOUT;
		}
	}

	/* initiate reset */
	wr_reg32(&jrp->rregs->jrcommand, JRCR_RESET);

	start = get_time_ns();
	while (rd_reg32(&jrp->rregs->jrcommand) & JRCR_RESET) {
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(dev, "job ring %d timed out on reset\n",
				jrp->ridx);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/* Deferred service handler, run as interrupt-fired tasklet */
static int caam_jr_dequeue(struct caam_drv_private_jr *jrp)
{
	int hw_idx, sw_idx, i, head, tail;
	void (*usercall)(struct device_d *dev, u32 *desc, u32 status, void *arg);
	u32 *userdesc, userstatus;
	void *userarg;
	int found;

	while (rd_reg32(&jrp->rregs->outring_used)) {
		head = jrp->head;

		sw_idx = tail = jrp->tail;
		hw_idx = jrp->out_ring_read_index;

		found = 0;

		for (i = 0; CIRC_CNT(head, tail + i, JOBR_DEPTH) >= 1; i++) {
			sw_idx = (tail + i) & (JOBR_DEPTH - 1);

			if (jrp->outring[hw_idx].desc ==
			    caam_dma_to_cpu(jrp->entinfo[sw_idx].desc_addr_dma)) {
				found = 1;
				break; /* found */
			}
		}

		if (!found)
			return -ENOENT;

		barrier();

		/* mark completed, avoid matching on a recycled desc addr */
		jrp->entinfo[sw_idx].desc_addr_dma = 0;

		/* Stash callback params for use outside of lock */
		usercall = jrp->entinfo[sw_idx].callbk;
		userarg = jrp->entinfo[sw_idx].cbkarg;
		userdesc = jrp->entinfo[sw_idx].desc_addr_virt;
		userstatus = caam32_to_cpu(jrp->outring[hw_idx].jrstatus);

		barrier();

		/* set done */
		wr_reg32(&jrp->rregs->outring_rmvd, 1);

		jrp->out_ring_read_index = (jrp->out_ring_read_index + 1) &
					   (JOBR_DEPTH - 1);

		/*
		 * if this job completed out-of-order, do not increment
		 * the tail.  Otherwise, increment tail by 1 plus the
		 * number of subsequent jobs already completed out-of-order
		 */
		if (sw_idx == tail) {
			do {
				tail = (tail + 1) & (JOBR_DEPTH - 1);
			} while (CIRC_CNT(head, tail, JOBR_DEPTH) >= 1 &&
				 jrp->entinfo[tail].desc_addr_dma == 0);

			jrp->tail = tail;
		}

		/* Finally, execute user's callback */
		usercall(jrp->dev, userdesc, userstatus, userarg);
	}

	return 0;
}

/* Main per-ring interrupt handler */
static int caam_jr_interrupt(struct caam_drv_private_jr *jrp)
{
	uint64_t start;
	u32 irqstate;

	start = get_time_ns();
	while (!(irqstate = rd_reg32(&jrp->rregs->jrintstatus))) {
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(jrp->dev, "timeout waiting for interrupt\n");
			return -ETIMEDOUT;
		}
	}

	/*
	 * If JobR error, we got more development work to do
	 * Flag a bug now, but we really need to shut down and
	 * restart the queue (and fix code).
	 */
	if (irqstate & JRINT_JR_ERROR) {
		dev_err(jrp->dev, "job ring error: irqstate: %08x\n", irqstate);
		BUG();
	}

	/* Have valid interrupt at this point, just ACK and trigger */
	wr_reg32(&jrp->rregs->jrintstatus, irqstate);

	return caam_jr_dequeue(jrp);
}


/**
 * caam_jr_enqueue() - Enqueue a job descriptor head. Returns 0 if OK,
 * -EBUSY if the queue is full, -EIO if it cannot map the caller's
 * descriptor.
 * @dev:  device of the job ring to be used.
 * @desc: points to a job descriptor that execute our request. All
 *        descriptors (and all referenced data) must be in a DMAable
 *        region, and all data references must be physical addresses
 *        accessible to CAAM (i.e. within a PAMU window granted
 *        to it).
 * @cbk:  pointer to a callback function to be invoked upon completion
 *        of this request. This has the form:
 *        callback(struct device *dev, u32 *desc, u32 stat, void *arg)
 *        where:
 *        @dev:    contains the job ring device that processed this
 *                 response.
 *        @desc:   descriptor that initiated the request, same as
 *                 "desc" being argued to caam_jr_enqueue().
 *        @status: untranslated status received from CAAM. See the
 *                 reference manual for a detailed description of
 *                 error meaning, or see the JRSTA definitions in the
 *                 register header file
 *        @areq:   optional pointer to an argument passed with the
 *                 original request
 * @areq: optional pointer to a user argument for use at callback
 *        time.
 **/
int caam_jr_enqueue(struct device_d *dev, u32 *desc,
		    void (*cbk)(struct device_d *dev, u32 *desc,
				u32 status, void *areq),
		    void *areq)
{
	struct caam_drv_private_jr *jrp;
	struct caam_jrentry_info *head_entry;
	int head, tail, desc_size;

	desc_size = (caam32_to_cpu(*desc) & HDR_JD_LENGTH_MASK) * sizeof(u32);

	if (!dev->priv)
		return -ENODEV;

	jrp = dev->priv;

	head = jrp->head;
	tail = jrp->tail;
	if (!rd_reg32(&jrp->rregs->inpring_avail) ||
	    CIRC_SPACE(head, tail, JOBR_DEPTH) <= 0) {
		return -EBUSY;
	}

	head_entry = &jrp->entinfo[head];
	head_entry->desc_addr_virt = phys_to_virt((u32) desc);
	head_entry->desc_size = desc_size;
	head_entry->callbk = (void *)cbk;
	head_entry->cbkarg = areq;
	head_entry->desc_addr_dma = (dma_addr_t)desc;

	if (!jrp->inpring)
		return -EIO;

	jrp->inpring[jrp->inp_ring_write_index] =
		cpu_to_caam_dma((dma_addr_t)desc);

	barrier();

	jrp->inp_ring_write_index = (jrp->inp_ring_write_index + 1) &
				    (JOBR_DEPTH - 1);
	jrp->head = (head + 1) & (JOBR_DEPTH - 1);

	barrier();
	wr_reg32(&jrp->rregs->inpring_jobadd, 1);

	clrsetbits_32(&jrp->rregs->rconfig_lo, JRCFG_IMSK, 0);

	return caam_jr_interrupt(jrp);
}
EXPORT_SYMBOL(caam_jr_enqueue);

/*
 * Init JobR independent of platform property detection
 */
static int caam_jr_init(struct device_d *dev)
{
	struct caam_drv_private_jr *jrp;
	dma_addr_t dma_inpring;
	dma_addr_t dma_outring;
	int i, error;

	jrp = dev->priv;

	error = caam_reset_hw_jr(dev);
	if (error)
		return error;

	jrp->inpring = dma_alloc_coherent(sizeof(*jrp->inpring) * JOBR_DEPTH,
					  &dma_inpring);
	if (!jrp->inpring)
		return -ENOMEM;

	jrp->outring = dma_alloc_coherent(sizeof(*jrp->outring) *
					  JOBR_DEPTH, &dma_outring);
	if (!jrp->outring) {
		dma_free_coherent(jrp->inpring, 0, sizeof(dma_addr_t) * JOBR_DEPTH);
		dev_err(dev, "can't allocate job rings for %d\n", jrp->ridx);
		return -ENOMEM;
	}

	jrp->entinfo = xzalloc(sizeof(*jrp->entinfo) * JOBR_DEPTH);

	for (i = 0; i < JOBR_DEPTH; i++)
		jrp->entinfo[i].desc_addr_dma = !0;

	/* Setup rings */
	jrp->inp_ring_write_index = 0;
	jrp->out_ring_read_index = 0;
	jrp->head = 0;
	jrp->tail = 0;

	wr_reg64(&jrp->rregs->inpring_base, dma_inpring);
	wr_reg64(&jrp->rregs->outring_base, dma_outring);
	wr_reg32(&jrp->rregs->inpring_size, JOBR_DEPTH);
	wr_reg32(&jrp->rregs->outring_size, JOBR_DEPTH);

	jrp->ringsize = JOBR_DEPTH;

	return 0;
}

/*
 * Probe routine for each detected JobR subsystem.
 */
int caam_jr_probe(struct device_d *dev)
{
	struct device_node *nprop;
	struct caam_job_ring __iomem *ctrl;
	struct caam_drv_private_jr *jrpriv;
	static int total_jobrs;
	int error;

	jrpriv = xzalloc(sizeof(*jrpriv));

	dev->priv = jrpriv;
	jrpriv->dev = dev;

	/* save ring identity relative to detection */
	jrpriv->ridx = total_jobrs++;

	nprop = dev->device_node;
	/* Get configuration properties from device tree */
	/* First, get register page */
	ctrl = dev_get_mem_region(dev, 0);
	if (IS_ERR(ctrl))
		return PTR_ERR(ctrl);

	jrpriv->rregs = (struct caam_job_ring __iomem __force *)ctrl;

	/* Now do the platform independent part */
	error = caam_jr_init(dev); /* now turn on hardware */
	if (error)
		return error;

	jrpriv->tfm_count = 0;

	return 0;
}

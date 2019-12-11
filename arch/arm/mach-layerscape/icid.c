#include <common.h>
#include <io.h>
#include <init.h>
#include <soc/fsl/immap_lsch2.h>
#include <soc/fsl/fsl_qbman.h>
#include <soc/fsl/fsl_fman.h>

/*
 * Stream IDs on Chassis-2 (for example ls1043a, ls1046a, ls1012) devices
 * are not hardwired and are programmed by sw.  There are a limited number
 * of stream IDs available, and the partitioning of them is scenario
 * dependent. This header defines the partitioning between legacy, PCI,
 * and DPAA1 devices.
 *
 * This partitioning can be customized in this file depending
 * on the specific hardware config:
 *
 *  -non-PCI legacy, platform devices (USB, SDHC, SATA, DMA, QE etc)
 *     -all legacy devices get a unique stream ID assigned and programmed in
 *      their AMQR registers by u-boot
 *
 *  -PCIe
 *     -there is a range of stream IDs set aside for PCI in this
 *      file.  U-boot will scan the PCI bus and for each device discovered:
 *         -allocate a streamID
 *         -set a PEXn LUT table entry mapping 'requester ID' to 'stream ID'
 *         -set a msi-map entry in the PEXn controller node in the
 *          device tree (see Documentation/devicetree/bindings/pci/pci-msi.txt
 *          for more info on the msi-map definition)
 *         -set a iommu-map entry in the PEXn controller node in the
 *          device tree (see Documentation/devicetree/bindings/pci/pci-iommu.txt
 *          for more info on the iommu-map definition)
 *
 *  -DPAA1
 *     - Stream ids for DPAA1 use are reserved for future usecase.
 *
 */

/* legacy devices */
#define FSL_USB1_STREAM_ID		1
#define FSL_USB2_STREAM_ID		2
#define FSL_USB3_STREAM_ID		3
#define FSL_SDHC_STREAM_ID		4
#define FSL_SATA_STREAM_ID		5
#define FSL_QE_STREAM_ID		6
#define FSL_QDMA_STREAM_ID		7
#define FSL_EDMA_STREAM_ID		8
#define FSL_ETR_STREAM_ID		9
#define FSL_DEBUG_STREAM_ID		10

/* PCI - programmed in PEXn_LUT */
#define FSL_PEX_STREAM_ID_START		11
#define FSL_PEX_STREAM_ID_END		26

/* DPAA1 - Stream-ID that can be programmed in DPAA1 h/w */
#define DPAA1_SID_START	27
#define DPAA1_SID_END	63

struct icid_id_table {
	const char *compat;
	u32 id;
	u32 reg;
	phys_addr_t compat_addr;
	phys_addr_t reg_addr;
};

struct fman_icid_id_table {
	u32 port_id;
	u32 icid;
};

#define QMAN_CQSIDR_REG        0x20a80

#define SEC_JRnICID_LS(n)	((0x10 + (n) * 0x8) + 0x4)
#define SEC_RTICnICID_LS(n)	((0x60 + (n) * 0x8) + 0x4)
#define SEC_DECOnICID_LS(n)	((0xa0 + (n) * 0x8) + 0x4)
#define SEC_QIIC_LS	0x70024
#define	SEC_IRBAR_JRn(n) 	(0x10000 * ((n) + 1))

struct icid_id_table icid_tbl_ls1046a[] = {
	{
		.compat = "fsl,qman",
		.id = DPAA1_SID_START,
		.reg = DPAA1_SID_START,
		.compat_addr = LSCH2_QMAN_ADDR,
		.reg_addr = offsetof(struct ccsr_qman, liodnr) + LSCH2_QMAN_ADDR,
	}, {
		.compat = "fsl,bman",
		.id = DPAA1_SID_START + 1,
		.reg = DPAA1_SID_START + 1,
		.compat_addr = LSCH2_BMAN_ADDR,
		.reg_addr = offsetof(struct ccsr_bman, liodnr) + LSCH2_BMAN_ADDR,
	}, {
		.compat = "fsl,esdhc",
		.id = FSL_SDHC_STREAM_ID,
		.reg = (((FSL_SDHC_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_ESDHC_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, sdhc_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "snps,dwc3",
		.id = FSL_USB1_STREAM_ID,
		.reg = (((FSL_USB1_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_XHCI_USB1_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, usb1_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "snps,dwc3",
		.id = FSL_USB2_STREAM_ID,
		.reg = (((FSL_USB2_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_XHCI_USB2_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, usb2_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "snps,dwc3",
		.id = FSL_USB3_STREAM_ID,
		.reg = (((FSL_USB3_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_XHCI_USB3_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, usb3_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "fsl,ls1046a-ahci",
		.id = FSL_SATA_STREAM_ID,
		.reg = (((FSL_SATA_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_HCI_BASE_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, sata_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "fsl,ls1046a-qdma",
		.id = FSL_QDMA_STREAM_ID,
		.reg = (1 << 31) | (FSL_QDMA_STREAM_ID),
		.compat_addr = LSCH2_QDMA_BASE_ADDR,
		.reg_addr = LSCH2_QDMA_BASE_ADDR + QMAN_CQSIDR_REG,
	}, {
		.id = FSL_QDMA_STREAM_ID,
		.reg = (1 << 31) | (FSL_QDMA_STREAM_ID),
		.compat_addr = LSCH2_QDMA_BASE_ADDR,
		.reg_addr = LSCH2_QDMA_BASE_ADDR + QMAN_CQSIDR_REG + 4,
	}, {
		.compat = "fsl,vf610-edma",
		.id = FSL_EDMA_STREAM_ID,
		.reg = (((FSL_EDMA_STREAM_ID) << 24) | (1 << 23)),
		.compat_addr = LSCH2_EDMA_BASE_ADDR,
		.reg_addr = offsetof(struct ccsr_scfg, edma_icid) + LSCH2_SCFG_ADDR,
	}, {
		.id = FSL_ETR_STREAM_ID,
		.reg = (((FSL_ETR_STREAM_ID) << 24) | (1 << 23)),
		.reg_addr = offsetof(struct ccsr_scfg, etr_icid) + LSCH2_SCFG_ADDR,
	}, {
		.id = FSL_DEBUG_STREAM_ID,
		.reg = (((FSL_DEBUG_STREAM_ID) << 24) | (1 << 23)),
		.reg_addr = offsetof(struct ccsr_scfg, debug_icid) + LSCH2_SCFG_ADDR,
	}, {
		.compat = "fsl,sec-v4.0",
		.id = DPAA1_SID_END,
		.compat_addr = LSCH2_SEC_ADDR,
		.reg_addr = SEC_QIIC_LS + LSCH2_SEC_ADDR,
	}, {
		.compat = "fsl,sec-v4.0-job-ring",
		.id = DPAA1_SID_START + 3,
		.reg = (((DPAA1_SID_START + 3) << 16) | (DPAA1_SID_START + 3)),
		.compat_addr = SEC_IRBAR_JRn(0),
		.reg_addr = SEC_JRnICID_LS(0) + LSCH2_SEC_ADDR,
	}, {
		.compat = "fsl,sec-v4.0-job-ring",
		.id = DPAA1_SID_START + 4,
		.reg = (((DPAA1_SID_START + 4) << 16) | (DPAA1_SID_START + 4)),
		.compat_addr = SEC_IRBAR_JRn(1),
		.reg_addr = SEC_JRnICID_LS(1) + LSCH2_SEC_ADDR,
	}, {
		.compat = "fsl,sec-v4.0-job-ring",
		.id = DPAA1_SID_START + 5,
		.reg = (((DPAA1_SID_START + 5) << 16) | (DPAA1_SID_START + 5)),
		.compat_addr = SEC_IRBAR_JRn(2),
		.reg_addr = SEC_JRnICID_LS(2) + LSCH2_SEC_ADDR,
	}, {
		.compat = "fsl,sec-v4.0-job-ring",
		.id = DPAA1_SID_START + 6,
		.reg = (((DPAA1_SID_START + 6) << 16) | (DPAA1_SID_START + 6)),
		.compat_addr = SEC_IRBAR_JRn(3),
		.reg_addr = SEC_JRnICID_LS(3) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 7,
		.reg = (((DPAA1_SID_START + 7) << 16) | (DPAA1_SID_START + 7)),
		.reg_addr = SEC_RTICnICID_LS(0) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 8,
		.reg = (((DPAA1_SID_START + 8) << 16) | (DPAA1_SID_START + 8)),
		.reg_addr = SEC_RTICnICID_LS(1) + LSCH2_SEC_ADDR,
	},{
		.id = DPAA1_SID_START + 9,
		.reg = (((DPAA1_SID_START + 9) << 16) | (DPAA1_SID_START + 9)),
		.reg_addr = SEC_RTICnICID_LS(2) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 10,
		.reg = (((DPAA1_SID_START + 10) << 16) | (DPAA1_SID_START + 10)),
		.reg_addr = SEC_RTICnICID_LS(3) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 11,
		.reg = (((DPAA1_SID_START + 11) << 16) | (DPAA1_SID_START + 11)),
		.reg_addr = SEC_DECOnICID_LS(0) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 12,
		.reg = (((DPAA1_SID_START + 12) << 16) | (DPAA1_SID_START + 12)),
		.reg_addr = SEC_DECOnICID_LS(1) + LSCH2_SEC_ADDR,
	}, {
		.id = DPAA1_SID_START + 13,
		.reg = (((DPAA1_SID_START + 13) << 16) | (DPAA1_SID_START + 13)),
		.reg_addr = SEC_DECOnICID_LS(2) + LSCH2_SEC_ADDR,
	},
};

struct fman_icid_id_table fman_icid_tbl_ls1046a[] = {
	{
		.port_id = 0x02,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x03,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x04,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x05,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x06,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x07,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x08,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x09,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x0a,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x0b,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x0c,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x0d,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x28,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x29,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x2a,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x2b,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x2c,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x2d,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x10,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x11,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x30,
		.icid = DPAA1_SID_END,
	}, {
		.port_id = 0x31,
		.icid = DPAA1_SID_END,
	},
};

static void set_icid(struct icid_id_table *tbl, int size)
{
	int i;

	for (i = 0; i < size; i++)
		out_be32((u32 *)(tbl[i].reg_addr), tbl[i].reg);
}

static void set_fman_icids(struct fman_icid_id_table *tbl, int size)
{
	int i;
	struct ccsr_fman *fm = (void *)LSCH2_FM1_ADDR;

	for (i = 0; i < size; i++) {
		out_be32(&fm->fm_bmi_common.fmbm_ppid[tbl[i].port_id - 1],
			 tbl[i].icid);
	}
}

static int set_icids(void)
{
	if (!of_machine_is_compatible("fsl,ls1046a"))
		return 0;

	/* setup general icid offsets */
	set_icid(icid_tbl_ls1046a, ARRAY_SIZE(icid_tbl_ls1046a));

	set_fman_icids(fman_icid_tbl_ls1046a, ARRAY_SIZE(fman_icid_tbl_ls1046a));

	return 0;
}
postcore_initcall(set_icids);
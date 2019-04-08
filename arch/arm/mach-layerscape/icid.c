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


#define FSL_INVALID_STREAM_ID		0

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
#define FSL_DPAA1_STREAM_ID_START	27
#define FSL_DPAA1_STREAM_ID_END		63

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

#define SET_ICID_ENTRY(name, idA, regA, addr, compataddr) \
	{					\
		.compat = name,			\
		.id = idA,			\
		.reg = regA,			\
		.compat_addr = compataddr,	\
		.reg_addr = addr,		\
	}

#define SET_SCFG_ICID(compat, streamid, name, compataddr) \
	SET_ICID_ENTRY(compat, streamid, (((streamid) << 24) | (1 << 23)), \
		offsetof(struct ccsr_scfg, name) + LSCH2_SCFG_ADDR, \
		compataddr)

#define SET_USB_ICID(usb_num, compat, streamid) \
	SET_SCFG_ICID(compat, streamid, usb##usb_num##_icid,\
		LSCH2_XHCI_USB##usb_num##_ADDR)

#define SET_SATA_ICID(compat, streamid) \
	SET_SCFG_ICID(compat, streamid, sata_icid,\
		LSCH2_HCI_BASE_ADDR)

#define SET_SDHC_ICID(streamid) \
	SET_SCFG_ICID("fsl,esdhc", streamid, sdhc_icid,\
		LSCH2_ESDHC_ADDR)

#define QMAN_CQSIDR_REG	0x20a80

#define SET_QDMA_ICID(compat, streamid) \
	SET_ICID_ENTRY(compat, streamid, (1 << 31) | (streamid), \
		LSCH2_QDMA_BASE_ADDR + QMAN_CQSIDR_REG, \
		LSCH2_QDMA_BASE_ADDR), \
	SET_ICID_ENTRY(NULL, streamid, (1 << 31) | (streamid), \
		LSCH2_QDMA_BASE_ADDR + QMAN_CQSIDR_REG + 4, \
		LSCH2_QDMA_BASE_ADDR)

#define SET_EDMA_ICID(streamid) \
	SET_SCFG_ICID("fsl,vf610-edma", streamid, edma_icid,\
		LSCH2_EDMA_BASE_ADDR)

#define SET_ETR_ICID(streamid) \
	SET_SCFG_ICID(NULL, streamid, etr_icid, 0)

#define SET_DEBUG_ICID(streamid) \
	SET_SCFG_ICID(NULL, streamid, debug_icid, 0)

#define SET_QE_ICID(streamid) \
	SET_SCFG_ICID("fsl,qe", streamid, qe_icid,\
		LSCH2_QE_BASE_ADDR)

#define SET_QMAN_ICID(streamid) \
	SET_ICID_ENTRY("fsl,qman", streamid, streamid, \
		offsetof(struct ccsr_qman, liodnr) + \
		LSCH2_QMAN_ADDR, \
		LSCH2_QMAN_ADDR)

#define SET_BMAN_ICID(streamid) \
	SET_ICID_ENTRY("fsl,bman", streamid, streamid, \
		offsetof(struct ccsr_bman, liodnr) + \
		LSCH2_BMAN_ADDR, \
		LSCH2_BMAN_ADDR)

#define SET_FMAN_ICID_ENTRY(_port_id, streamid) \
	{ .port_id = (_port_id), .icid = (streamid) }

#define SET_SEC_QI_ICID(streamid) \
	SET_ICID_ENTRY("fsl,sec-v4.0", streamid, \
		0, offsetof(ccsr_sec_t, qilcr_ls) + \
		LSCH2_SEC_ADDR, \
		LSCH2_SEC_ADDR)

#define SET_SEC_JR_ICID_ENTRY(jr_num, streamid) \
	SET_ICID_ENTRY( \
		(CONFIG_ARMV8_SEC_FIRMWARE_SUPPORT && \
		(FSL_SEC_JR##jr_num##_OFFSET ==  \
			SEC_JR3_OFFSET + CONFIG_SYS_FSL_SEC_OFFSET) \
			? NULL \
			: "fsl,sec-v4.0-job-ring"), \
		streamid, \
		(((streamid) << 16) | (streamid)), \
		offsetof(ccsr_sec_t, jrliodnr[jr_num].ls) + \
		LSCH2_SEC_ADDR, \
		FSL_SEC_JR##jr_num##_BASE_ADDR)

#define SET_SEC_DECO_ICID_ENTRY(deco_num, streamid) \
	SET_ICID_ENTRY(NULL, streamid, (((streamid) << 16) | (streamid)), \
		offsetof(ccsr_sec_t, decoliodnr[deco_num].ls) + \
		LSCH2_SEC_ADDR, 0)

#define SET_SEC_RTIC_ICID_ENTRY(rtic_num, streamid) \
	SET_ICID_ENTRY(NULL, streamid, (((streamid) << 16) | (streamid)), \
		offsetof(ccsr_sec_t, rticliodnr[rtic_num].ls) + \
		LSCH2_SEC_ADDR, 0)

static struct icid_id_table icid_tbl_ls1046a[] = {
	SET_QMAN_ICID(FSL_DPAA1_STREAM_ID_START),
	SET_BMAN_ICID(FSL_DPAA1_STREAM_ID_START + 1),

	SET_SDHC_ICID(FSL_SDHC_STREAM_ID),

	SET_USB_ICID(1, "snps,dwc3", FSL_USB1_STREAM_ID),
	SET_USB_ICID(2, "snps,dwc3", FSL_USB2_STREAM_ID),
	SET_USB_ICID(3, "snps,dwc3", FSL_USB3_STREAM_ID),

	SET_SATA_ICID("fsl,ls1046a-ahci", FSL_SATA_STREAM_ID),
	SET_QDMA_ICID("fsl,ls1046a-qdma", FSL_QDMA_STREAM_ID),
	SET_EDMA_ICID(FSL_EDMA_STREAM_ID),
	SET_ETR_ICID(FSL_ETR_STREAM_ID),
	SET_DEBUG_ICID(FSL_DEBUG_STREAM_ID),
};

static struct fman_icid_id_table fman_icid_tbl_ls1046a[] = {
	/* port id, icid */
	SET_FMAN_ICID_ENTRY(0x02, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x03, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x04, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x05, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x06, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x07, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x08, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x09, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x0a, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x0b, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x0c, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x0d, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x28, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x29, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x2a, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x2b, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x2c, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x2d, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x10, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x11, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x30, FSL_DPAA1_STREAM_ID_END),
	SET_FMAN_ICID_ENTRY(0x31, FSL_DPAA1_STREAM_ID_END),
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
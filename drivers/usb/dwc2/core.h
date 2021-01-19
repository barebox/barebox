/* SPDX-License-Identifier: GPL-2.0+ */
/* Maximum number of Endpoints/HostChannels */
#define DWC2_MAX_EPS_CHANNELS	16

/**
 * struct dwc2_core_params - Parameters for configuring the core
 *
 * @otg_cap:            Specifies the OTG capabilities.
 *                       0 - HNP and SRP capable
 *                       1 - SRP Only capable
 *                       2 - No HNP/SRP capable (always available)
 *                      Defaults to best available option (0, 1, then 2)
 * @host_dma:           Specifies whether to use slave or DMA mode for accessing
 *                      the data FIFOs. The driver will automatically detect the
 *                      value for this parameter if none is specified.
 *                       0 - Slave (always available)
 *                       1 - DMA (default, if available)
 * @dma_desc_enable:    When DMA mode is enabled, specifies whether to use
 *                      address DMA mode or descriptor DMA mode for accessing
 *                      the data FIFOs. The driver will automatically detect the
 *                      value for this if none is specified.
 *                       0 - Address DMA
 *                       1 - Descriptor DMA (default, if available)
 * @dma_desc_fs_enable: When DMA mode is enabled, specifies whether to use
 *                      address DMA mode or descriptor DMA mode for accessing
 *                      the data FIFOs in Full Speed mode only. The driver
 *                      will automatically detect the value for this if none is
 *                      specified.
 *                       0 - Address DMA
 *                       1 - Descriptor DMA in FS (default, if available)
 * @speed:              Specifies the maximum speed of operation in host and
 *                      device mode. The actual speed depends on the speed of
 *                      the attached device and the value of phy_type.
 *                       0 - High Speed
 *                           (default when phy_type is UTMI+ or ULPI)
 *                       1 - Full Speed
 *                           (default when phy_type is Full Speed)
 * @enable_dynamic_fifo: 0 - Use coreConsultant-specified FIFO size parameters
 *                       1 - Allow dynamic FIFO sizing (default, if available)
 * @en_multiple_tx_fifo: Specifies whether dedicated per-endpoint transmit FIFOs
 *                      are enabled for non-periodic IN endpoints in device
 *                      mode.
 * @host_rx_fifo_size:  Number of 4-byte words in the Rx FIFO in host mode when
 *                      dynamic FIFO sizing is enabled
 *                       16 to 32768
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @host_nperio_tx_fifo_size: Number of 4-byte words in the non-periodic Tx FIFO
 *                      in host mode when dynamic FIFO sizing is enabled
 *                       16 to 32768
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @host_perio_tx_fifo_size: Number of 4-byte words in the periodic Tx FIFO in
 *                      host mode when dynamic FIFO sizing is enabled
 *                       16 to 32768
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @max_transfer_size:  The maximum transfer size supported, in bytes
 *                       2047 to 65,535
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @max_packet_count:   The maximum number of packets in a transfer
 *                       15 to 511
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @host_channels:      The number of host channel registers to use
 *                       1 to 16
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @phy_type:           Specifies the type of PHY interface to use. By default,
 *                      the driver will automatically detect the phy_type.
 *                       0 - Full Speed Phy
 *                       1 - UTMI+ Phy
 *                       2 - ULPI Phy
 *                      Defaults to best available option (2, 1, then 0)
 * @phy_utmi_width:     Specifies the UTMI+ Data Width (in bits). This parameter
 *                      is applicable for a phy_type of UTMI+ or ULPI. (For a
 *                      ULPI phy_type, this parameter indicates the data width
 *                      between the MAC and the ULPI Wrapper.) Also, this
 *                      parameter is applicable only if the OTG_HSPHY_WIDTH cC
 *                      parameter was set to "8 and 16 bits", meaning that the
 *                      core has been configured to work at either data path
 *                      width.
 *                       8 or 16 (default 16 if available)
 * @phy_ulpi_ddr:       Specifies whether the ULPI operates at double or single
 *                      data rate. This parameter is only applicable if phy_type
 *                      is ULPI.
 *                       0 - single data rate ULPI interface with 8 bit wide
 *                           data bus (default)
 *                       1 - double data rate ULPI interface with 4 bit wide
 *                           data bus
 * @phy_ulpi_ext_vbus:  For a ULPI phy, specifies whether to use the internal or
 *                      external supply to drive the VBus
 *                       0 - Internal supply (default)
 *                       1 - External supply
 * @i2c_enable:         Specifies whether to use the I2Cinterface for a full
 *                      speed PHY. This parameter is only applicable if phy_type
 *                      is FS.
 *                       0 - No (default)
 *                       1 - Yes
 * @ipg_isoc_en:        Indicates the IPG supports is enabled or disabled.
 *                       0 - Disable (default)
 *                       1 - Enable
 * @acg_enable:		For enabling Active Clock Gating in the controller
 *                       0 - No
 *                       1 - Yes
 * @ulpi_fs_ls:         Make ULPI phy operate in FS/LS mode only
 *                       0 - No (default)
 *                       1 - Yes
 * @host_support_fs_ls_low_power: Specifies whether low power mode is supported
 *                      when attached to a Full Speed or Low Speed device in
 *                      host mode.
 *                       0 - Don't support low power mode (default)
 *                       1 - Support low power mode
 * @host_ls_low_power_phy_clk: Specifies the PHY clock rate in low power mode
 *                      when connected to a Low Speed device in host
 *                      mode. This parameter is applicable only if
 *                      host_support_fs_ls_low_power is enabled.
 *                       0 - 48 MHz
 *                           (default when phy_type is UTMI+ or ULPI)
 *                       1 - 6 MHz
 *                           (default when phy_type is Full Speed)
 * @oc_disable:		Flag to disable overcurrent condition.
 *			0 - Allow overcurrent condition to get detected
 *			1 - Disable overcurrent condtion to get detected
 * @ts_dline:           Enable Term Select Dline pulsing
 *                       0 - No (default)
 *                       1 - Yes
 * @reload_ctl:         Allow dynamic reloading of HFIR register during runtime
 *                       0 - No (default for core < 2.92a)
 *                       1 - Yes (default for core >= 2.92a)
 * @ahbcfg:             This field allows the default value of the GAHBCFG
 *                      register to be overridden
 *                       -1         - GAHBCFG value will be set to 0x06
 *                                    (INCR, default)
 *                       all others - GAHBCFG value will be overridden with
 *                                    this value
 *                      Not all bits can be controlled like this, the
 *                      bits defined by GAHBCFG_CTRL_MASK are controlled
 *                      by the driver and are ignored in this
 *                      configuration value.
 * @uframe_sched:       True to enable the microframe scheduler
 * @external_id_pin_ctl: Specifies whether ID pin is handled externally.
 *                      Disable CONIDSTSCHNG controller interrupt in such
 *                      case.
 *                      0 - No (default)
 *                      1 - Yes
 * @power_down:         Specifies whether the controller support power_down.
 *			If power_down is enabled, the controller will enter
 *			power_down in both peripheral and host mode when
 *			needed.
 *			0 - No (default)
 *			1 - Partial power down
 *			2 - Hibernation
 * @lpm:		Enable LPM support.
 *			0 - No
 *			1 - Yes
 * @lpm_clock_gating:		Enable core PHY clock gating.
 *			0 - No
 *			1 - Yes
 * @besl:		Enable LPM Errata support.
 *			0 - No
 *			1 - Yes
 * @hird_threshold_en:	HIRD or HIRD Threshold enable.
 *			0 - No
 *			1 - Yes
 * @hird_threshold:	Value of BESL or HIRD Threshold.
 * @activate_stm_fs_transceiver: Activate internal transceiver using GGPIO
 *			register.
 *			0 - Deactivate the transceiver (default)
 *			1 - Activate the transceiver
 * @g_dma:              Enables gadget dma usage (default: autodetect).
 * @g_dma_desc:         Enables gadget descriptor DMA (default: autodetect).
 * @g_rx_fifo_size:	The periodic rx fifo size for the device, in
 *			DWORDS from 16-32768 (default: 2048 if
 *			possible, otherwise autodetect).
 * @g_np_tx_fifo_size:	The non-periodic tx fifo size for the device in
 *			DWORDS from 16-32768 (default: 1024 if
 *			possible, otherwise autodetect).
 * @g_tx_fifo_size:	An array of TX fifo sizes in dedicated fifo
 *			mode. Each value corresponds to one EP
 *			starting from EP1 (max 15 values). Sizes are
 *			in DWORDS with possible values from from
 *			16-32768 (default: 256, 256, 256, 256, 768,
 *			768, 768, 768, 0, 0, 0, 0, 0, 0, 0).
 * @change_speed_quirk: Change speed configuration to DWC2_SPEED_PARAM_FULL
 *                      while full&low speed device connect. And change speed
 *                      back to DWC2_SPEED_PARAM_HIGH while device is gone.
 *			0 - No (default)
 *			1 - Yes
 *
 * The following parameters may be specified when starting the module. These
 * parameters define how the DWC_otg controller should be configured. A
 * value of -1 (or any other out of range value) for any parameter means
 * to read the value from hardware (if possible) or use the builtin
 * default described above.
 */
struct dwc2_core_params {
	u8 otg_cap;
#define DWC2_CAP_PARAM_HNP_SRP_CAPABLE		0
#define DWC2_CAP_PARAM_SRP_ONLY_CAPABLE		1
#define DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE	2

	u8 phy_type;
#define DWC2_PHY_TYPE_PARAM_FS		0
#define DWC2_PHY_TYPE_PARAM_UTMI	1
#define DWC2_PHY_TYPE_PARAM_ULPI	2

	u8 speed;
#define DWC2_SPEED_PARAM_HIGH	0
#define DWC2_SPEED_PARAM_FULL	1
#define DWC2_SPEED_PARAM_LOW	2

	u8 phy_utmi_width;
	bool phy_ulpi_ddr;
	bool phy_ulpi_ext_vbus;
	bool phy_ulpi_ext_vbus_ind;
	bool phy_ulpi_ext_vbus_ind_complement;
	bool phy_ulpi_ext_vbus_ind_passthrough;

	bool enable_dynamic_fifo;
	bool en_multiple_tx_fifo;
	bool i2c_enable;
	bool acg_enable;
	bool ulpi_fs_ls;
	bool ts_dline;
	bool reload_ctl;
	bool uframe_sched;
	bool external_id_pin_ctl;

	int power_down;
#define DWC2_POWER_DOWN_PARAM_NONE		0
#define DWC2_POWER_DOWN_PARAM_PARTIAL		1
#define DWC2_POWER_DOWN_PARAM_HIBERNATION	2

	bool lpm;
	bool lpm_clock_gating;
	bool besl;
	bool hird_threshold_en;
	u8 hird_threshold;
	bool activate_stm_fs_transceiver;
	bool ipg_isoc_en;
	u16 max_packet_count;
	u32 max_transfer_size;
	u32 ahbcfg;

	bool dma;
	bool dma_desc;

	/* Host parameters */
	bool host_support_fs_ls_low_power;
	bool host_ls_low_power_phy_clk;

	u8 host_channels;
	u16 host_rx_fifo_size;
	u16 host_nperio_tx_fifo_size;
	u16 host_perio_tx_fifo_size;

	/* Gadget parameters */
	u32 g_rx_fifo_size;
	u32 g_np_tx_fifo_size;
	u32 g_tx_fifo_size[DWC2_MAX_EPS_CHANNELS];

	bool change_speed_quirk;
};

/**
 * struct dwc2_hw_params - Autodetected parameters.
 *
 * These parameters are the various parameters read from hardware
 * registers during initialization. They typically contain the best
 * supported or maximum value that can be configured in the
 * corresponding dwc2_core_params value.
 *
 * The values that are not in dwc2_core_params are documented below.
 *
 * @op_mode:             Mode of Operation
 *                       0 - HNP- and SRP-Capable OTG (Host & Device)
 *                       1 - SRP-Capable OTG (Host & Device)
 *                       2 - Non-HNP and Non-SRP Capable OTG (Host & Device)
 *                       3 - SRP-Capable Device
 *                       4 - Non-OTG Device
 *                       5 - SRP-Capable Host
 *                       6 - Non-OTG Host
 * @arch:                Architecture
 *                       0 - Slave only
 *                       1 - External DMA
 *                       2 - Internal DMA
 * @ipg_isoc_en:        This feature indicates that the controller supports
 *                      the worst-case scenario of Rx followed by Rx
 *                      Interpacket Gap (IPG) (32 bitTimes) as per the utmi
 *                      specification for any token following ISOC OUT token.
 *                       0 - Don't support
 *                       1 - Support
 * @power_optimized:    Are power optimizations enabled?
 * @num_dev_ep:         Number of device endpoints available
 * @num_dev_in_eps:     Number of device IN endpoints available
 * @num_dev_perio_in_ep: Number of device periodic IN endpoints
 *                       available
 * @dev_token_q_depth:  Device Mode IN Token Sequence Learning Queue
 *                      Depth
 *                       0 to 30
 * @host_perio_tx_q_depth:
 *                      Host Mode Periodic Request Queue Depth
 *                       2, 4 or 8
 * @nperio_tx_q_depth:
 *                      Non-Periodic Request Queue Depth
 *                       2, 4 or 8
 * @hs_phy_type:         High-speed PHY interface type
 *                       0 - High-speed interface not supported
 *                       1 - UTMI+
 *                       2 - ULPI
 *                       3 - UTMI+ and ULPI
 * @fs_phy_type:         Full-speed PHY interface type
 *                       0 - Full speed interface not supported
 *                       1 - Dedicated full speed interface
 *                       2 - FS pins shared with UTMI+ pins
 *                       3 - FS pins shared with ULPI pins
 * @total_fifo_size:    Total internal RAM for FIFOs (bytes)
 * @hibernation:	Is hibernation enabled?
 * @utmi_phy_data_width: UTMI+ PHY data width
 *                       0 - 8 bits
 *                       1 - 16 bits
 *                       2 - 8 or 16 bits
 * @snpsid:             Value from SNPSID register
 * @dev_ep_dirs:        Direction of device endpoints (GHWCFG1)
 * @g_tx_fifo_size:	Power-on values of TxFIFO sizes
 * @dma_desc_enable:    When DMA mode is enabled, specifies whether to use
 *                      address DMA mode or descriptor DMA mode for accessing
 *                      the data FIFOs. The driver will automatically detect the
 *                      value for this if none is specified.
 *                       0 - Address DMA
 *                       1 - Descriptor DMA (default, if available)
 * @enable_dynamic_fifo: 0 - Use coreConsultant-specified FIFO size parameters
 *                       1 - Allow dynamic FIFO sizing (default, if available)
 * @en_multiple_tx_fifo: Specifies whether dedicated per-endpoint transmit FIFOs
 *                      are enabled for non-periodic IN endpoints in device
 *                      mode.
 * @host_nperio_tx_fifo_size: Number of 4-byte words in the non-periodic Tx FIFO
 *                      in host mode when dynamic FIFO sizing is enabled
 *                       16 to 32768
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @host_perio_tx_fifo_size: Number of 4-byte words in the periodic Tx FIFO in
 *                      host mode when dynamic FIFO sizing is enabled
 *                       16 to 32768
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @max_transfer_size:  The maximum transfer size supported, in bytes
 *                       2047 to 65,535
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @max_packet_count:   The maximum number of packets in a transfer
 *                       15 to 511
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @host_channels:      The number of host channel registers to use
 *                       1 to 16
 *                      Actual maximum value is autodetected and also
 *                      the default.
 * @dev_nperio_tx_fifo_size: Number of 4-byte words in the non-periodic Tx FIFO
 *			     in device mode when dynamic FIFO sizing is enabled
 *			     16 to 32768
 *			     Actual maximum value is autodetected and also
 *			     the default.
 * @i2c_enable:         Specifies whether to use the I2Cinterface for a full
 *                      speed PHY. This parameter is only applicable if phy_type
 *                      is FS.
 *                       0 - No (default)
 *                       1 - Yes
 * @acg_enable:		For enabling Active Clock Gating in the controller
 *                       0 - Disable
 *                       1 - Enable
 * @lpm_mode:		For enabling Link Power Management in the controller
 *                       0 - Disable
 *                       1 - Enable
 * @rx_fifo_size:	Number of 4-byte words in the  Rx FIFO when dynamic
 *			FIFO sizing is enabled 16 to 32768
 *			Actual maximum value is autodetected and also
 *			the default.
 */
struct dwc2_hw_params {
	unsigned op_mode:3;
	unsigned arch:2;
	unsigned dma_desc_enable:1;
	unsigned enable_dynamic_fifo:1;
	unsigned en_multiple_tx_fifo:1;
	unsigned rx_fifo_size:16;
	unsigned host_nperio_tx_fifo_size:16;
	unsigned dev_nperio_tx_fifo_size:16;
	unsigned host_perio_tx_fifo_size:16;
	unsigned nperio_tx_q_depth:3;
	unsigned host_perio_tx_q_depth:3;
	unsigned dev_token_q_depth:5;
	unsigned max_transfer_size:26;
	unsigned max_packet_count:11;
	unsigned host_channels:5;
	unsigned hs_phy_type:2;
	unsigned fs_phy_type:2;
	unsigned i2c_enable:1;
	unsigned acg_enable:1;
	unsigned num_dev_ep:4;
	unsigned num_dev_in_eps:4;
	unsigned num_dev_perio_in_ep:4;
	unsigned total_fifo_size:16;
	unsigned power_optimized:1;
	unsigned hibernation:1;
	unsigned utmi_phy_data_width:2;
	unsigned lpm_mode:1;
	unsigned ipg_isoc_en:1;
	u32 snpsid;
	u32 dev_ep_dirs;
	u32 g_tx_fifo_size[DWC2_MAX_EPS_CHANNELS];
};

#define MAX_DEVICE			16
#define MAX_ENDPOINT DWC2_MAX_EPS_CHANNELS

struct dwc2_ep {
	struct usb_ep ep;
	struct dwc2 *dwc2;
	struct list_head queue;
	struct dwc2_request *req;
	char name[8];

	unsigned int            size_loaded;
	unsigned int            last_load;
	unsigned short          fifo_size;
	unsigned short		fifo_index;

	u8 dir_in;
	u8 epnum;
	u8 mc;
	u16 interval;

	unsigned int            halted:1;
	unsigned int            periodic:1;
	unsigned int            isochronous:1;
	unsigned int            send_zlp:1;
	unsigned int            target_frame;
#define TARGET_FRAME_INITIAL   0xFFFFFFFF
	bool			frame_overrun;
};

struct dwc2_request {
	struct usb_request req;
	struct list_head queue;
};

/* Gadget ep0 states */
enum dwc2_ep0_state {
	DWC2_EP0_SETUP,
	DWC2_EP0_DATA_IN,
	DWC2_EP0_DATA_OUT,
	DWC2_EP0_STATUS_IN,
	DWC2_EP0_STATUS_OUT,
};

/* Size of control and EP0 buffers */
#define DWC2_CTRL_BUFF_SIZE 8

struct dwc2 {
	struct device_d *dev;
	void __iomem *regs;
	enum usb_dr_mode dr_mode;
	struct dwc2_hw_params hw_params;
	struct dwc2_core_params params;

	struct phy *phy; /* optional */
	struct clk *clk;

#ifdef CONFIG_USB_DWC2_HOST
	struct usb_host host;
	u8 in_data_toggle[MAX_DEVICE][MAX_ENDPOINT];
	u8 out_data_toggle[MAX_DEVICE][MAX_ENDPOINT];
	int root_hub_devnum;
#endif

#ifdef CONFIG_USB_DWC2_GADGET
	struct usb_gadget gadget;
	struct dwc2_ep *eps_in[MAX_ENDPOINT];
	struct dwc2_ep *eps_out[MAX_ENDPOINT];
	struct usb_request *ctrl_req;
	void *ep0_buff;
	void *ctrl_buff;
	enum dwc2_ep0_state ep0_state;
	struct usb_gadget_driver *driver;

	int num_eps;
	u16 frame_number;
	u32 fifo_map;
	unsigned int dedicated_fifos:1;
	unsigned int enabled:1;
	unsigned int connected:1;
	unsigned int is_selfpowered:1;
#endif
};

#define host_to_dwc2(ptr) container_of(ptr, struct dwc2, host)
#define gadget_to_dwc2(ptr) container_of(ptr, struct dwc2, gadget)

#define dwc2_err(d, arg...) dev_err((d)->dev, ## arg)
#define dwc2_warn(d, arg...) dev_err((d)->dev, ## arg)
#define dwc2_info(d, arg...) dev_info((d)->dev, ## arg)
#define dwc2_dbg(d, arg...) dev_dbg((d)->dev, ## arg)
#define dwc2_vdbg(d, arg...) dev_vdbg((d)->dev, ## arg)

static inline u32 dwc2_readl(struct dwc2 *dwc2, u32 offset)
{
	u32 val;

	val = readl(dwc2->regs + offset);

	return val;
}

static inline void dwc2_writel(struct dwc2 *dwc2, u32 value, u32 offset)
{
	writel(value, dwc2->regs + offset);
}

/**
 * dwc2_wait_bit_set - Waits for bit to be set.
 * @dwc2: Programming view of DWC2 controller.
 * @offset: Register's offset where bit/bits must be set.
 * @mask: Mask of the bit/bits which must be set.
 * @timeout: Timeout to wait.
 *
 * Return: 0 if bit/bits are set or -ETIMEDOUT in case of timeout.
 */
static inline int dwc2_wait_bit_set(struct dwc2 *dwc2, u32 offset, u32 mask,
			    u32 timeout)
{
	return wait_on_timeout(timeout * USECOND,
			dwc2_readl(dwc2, offset) & mask);
}

/**
 * dwc2_wait_bit_clear - Waits for bit to be clear.
 * @dwc2: Programming view of DWC2 controller.
 * @offset: Register's offset where bit/bits must be set.
 * @mask: Mask of the bit/bits which must be set.
 * @timeout: Timeout to wait.
 *
 * Return: 0 if bit/bits are set or -ETIMEDOUT in case of timeout.
 */
static inline int dwc2_wait_bit_clear(struct dwc2 *dwc2, u32 offset, u32 mask,
			    u32 timeout)
{
	return wait_on_timeout(timeout * USECOND,
			!(dwc2_readl(dwc2, offset) & mask));
}

/*
 * Returns the mode of operation, host or device
 */
static inline int dwc2_is_host_mode(struct dwc2 *dwc2)
{
	return (dwc2_readl(dwc2, GINTSTS) & GINTSTS_CURMODE_HOST) != 0;
}

static inline int dwc2_is_device_mode(struct dwc2 *dwc2)
{
	return (dwc2_readl(dwc2, GINTSTS) & GINTSTS_CURMODE_HOST) == 0;
}

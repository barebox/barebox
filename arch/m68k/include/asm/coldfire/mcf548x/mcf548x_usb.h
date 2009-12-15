/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Register and bit definitions for the MCF548X and MCF547x
 *  Universal Serial Bus (USB)
 *
 * @note  According to FreeScale errata sheet, the USB controller
 *  isn't really usable on MCF54xx V4E CPUs.
 *  Check V4M cores or wait for errata fixed
 *  Last update: 25.02.2008 10:55:00
 */
#ifndef __MCF548X_USB_H__
#define __MCF548X_USB_H__

/*
 *  Universal Serial Bus (USB)
 */

/* Register read/write macros */
#define MCF_USB_USBAISR          (*(vuint8_t *)(&__MBAR[0x00B000]))
#define MCF_USB_USBAIMR          (*(vuint8_t *)(&__MBAR[0x00B001]))
#define MCF_USB_EPINFO           (*(vuint8_t *)(&__MBAR[0x00B003]))
#define MCF_USB_CFGR             (*(vuint8_t *)(&__MBAR[0x00B004]))
#define MCF_USB_CFGAR            (*(vuint8_t *)(&__MBAR[0x00B005]))
#define MCF_USB_SPEEDR           (*(vuint8_t *)(&__MBAR[0x00B006]))
#define MCF_USB_FRMNUMR          (*(vuint16_t*)(&__MBAR[0x00B00E]))
#define MCF_USB_EPTNR            (*(vuint16_t*)(&__MBAR[0x00B010]))
#define MCF_USB_IFUR             (*(vuint16_t*)(&__MBAR[0x00B014]))
#define MCF_USB_IFR0             (*(vuint16_t*)(&__MBAR[0x00B040]))
#define MCF_USB_IFR1             (*(vuint16_t*)(&__MBAR[0x00B042]))
#define MCF_USB_IFR2             (*(vuint16_t*)(&__MBAR[0x00B044]))
#define MCF_USB_IFR3             (*(vuint16_t*)(&__MBAR[0x00B046]))
#define MCF_USB_IFR4             (*(vuint16_t*)(&__MBAR[0x00B048]))
#define MCF_USB_IFR5             (*(vuint16_t*)(&__MBAR[0x00B04A]))
#define MCF_USB_IFR6             (*(vuint16_t*)(&__MBAR[0x00B04C]))
#define MCF_USB_IFR7             (*(vuint16_t*)(&__MBAR[0x00B04E]))
#define MCF_USB_IFR8             (*(vuint16_t*)(&__MBAR[0x00B050]))
#define MCF_USB_IFR9             (*(vuint16_t*)(&__MBAR[0x00B052]))
#define MCF_USB_IFR10            (*(vuint16_t*)(&__MBAR[0x00B054]))
#define MCF_USB_IFR11            (*(vuint16_t*)(&__MBAR[0x00B056]))
#define MCF_USB_IFR12            (*(vuint16_t*)(&__MBAR[0x00B058]))
#define MCF_USB_IFR13            (*(vuint16_t*)(&__MBAR[0x00B05A]))
#define MCF_USB_IFR14            (*(vuint16_t*)(&__MBAR[0x00B05C]))
#define MCF_USB_IFR15            (*(vuint16_t*)(&__MBAR[0x00B05E]))
#define MCF_USB_IFR16            (*(vuint16_t*)(&__MBAR[0x00B060]))
#define MCF_USB_IFR17            (*(vuint16_t*)(&__MBAR[0x00B062]))
#define MCF_USB_IFR18            (*(vuint16_t*)(&__MBAR[0x00B064]))
#define MCF_USB_IFR19            (*(vuint16_t*)(&__MBAR[0x00B066]))
#define MCF_USB_IFR20            (*(vuint16_t*)(&__MBAR[0x00B068]))
#define MCF_USB_IFR21            (*(vuint16_t*)(&__MBAR[0x00B06A]))
#define MCF_USB_IFR22            (*(vuint16_t*)(&__MBAR[0x00B06C]))
#define MCF_USB_IFR23            (*(vuint16_t*)(&__MBAR[0x00B06E]))
#define MCF_USB_IFR24            (*(vuint16_t*)(&__MBAR[0x00B070]))
#define MCF_USB_IFR25            (*(vuint16_t*)(&__MBAR[0x00B072]))
#define MCF_USB_IFR26            (*(vuint16_t*)(&__MBAR[0x00B074]))
#define MCF_USB_IFR27            (*(vuint16_t*)(&__MBAR[0x00B076]))
#define MCF_USB_IFR28            (*(vuint16_t*)(&__MBAR[0x00B078]))
#define MCF_USB_IFR29            (*(vuint16_t*)(&__MBAR[0x00B07A]))
#define MCF_USB_IFR30            (*(vuint16_t*)(&__MBAR[0x00B07C]))
#define MCF_USB_IFR31            (*(vuint16_t*)(&__MBAR[0x00B07E]))
#define MCF_USB_IFRn(x)          (*(vuint16_t*)(&__MBAR[0x00B040+((x)*0x002)]))
#define MCF_USB_PPCNT            (*(vuint16_t*)(&__MBAR[0x00B080]))
#define MCF_USB_DPCNT            (*(vuint16_t*)(&__MBAR[0x00B082]))
#define MCF_USB_CRCECNT          (*(vuint16_t*)(&__MBAR[0x00B084]))
#define MCF_USB_BSECNT           (*(vuint16_t*)(&__MBAR[0x00B086]))
#define MCF_USB_PIDECNT          (*(vuint16_t*)(&__MBAR[0x00B088]))
#define MCF_USB_FRMECNT          (*(vuint16_t*)(&__MBAR[0x00B08A]))
#define MCF_USB_TXPCNT           (*(vuint16_t*)(&__MBAR[0x00B08C]))
#define MCF_USB_CNTOVR           (*(vuint8_t *)(&__MBAR[0x00B08E]))
#define MCF_USB_EP0ACR           (*(vuint8_t *)(&__MBAR[0x00B101]))
#define MCF_USB_EP0MPSR          (*(vuint16_t*)(&__MBAR[0x00B102]))
#define MCF_USB_EP0IFR           (*(vuint8_t *)(&__MBAR[0x00B104]))
#define MCF_USB_EP0SR            (*(vuint8_t *)(&__MBAR[0x00B105]))
#define MCF_USB_BMRTR            (*(vuint8_t *)(&__MBAR[0x00B106]))
#define MCF_USB_BRTR             (*(vuint8_t *)(&__MBAR[0x00B107]))
#define MCF_USB_WVALUER          (*(vuint16_t*)(&__MBAR[0x00B108]))
#define MCF_USB_WINDEXR          (*(vuint16_t*)(&__MBAR[0x00B10A]))
#define MCF_USB_WLENGTH          (*(vuint16_t*)(&__MBAR[0x00B10C]))
#define MCF_USB_EP1OUTACR        (*(vuint8_t *)(&__MBAR[0x00B131]))
#define MCF_USB_EP2OUTACR        (*(vuint8_t *)(&__MBAR[0x00B161]))
#define MCF_USB_EP3OUTACR        (*(vuint8_t *)(&__MBAR[0x00B191]))
#define MCF_USB_EP4OUTACR        (*(vuint8_t *)(&__MBAR[0x00B1C1]))
#define MCF_USB_EP5OUTACR        (*(vuint8_t *)(&__MBAR[0x00B1F1]))
#define MCF_USB_EP6OUTACR        (*(vuint8_t *)(&__MBAR[0x00B221]))
#define MCF_USB_EPnOUTACR(x)     (*(vuint8_t *)(&__MBAR[0x00B131+((x)*0x030)]))
#define MCF_USB_EP1OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B132]))
#define MCF_USB_EP2OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B162]))
#define MCF_USB_EP3OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B192]))
#define MCF_USB_EP4OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B1C2]))
#define MCF_USB_EP5OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B1F2]))
#define MCF_USB_EP6OUTMPSR       (*(vuint16_t*)(&__MBAR[0x00B222]))
#define MCF_USB_EPnOUTMPSR(x)    (*(vuint16_t*)(&__MBAR[0x00B132+((x)*0x030)]))
#define MCF_USB_EP1OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B134]))
#define MCF_USB_EP2OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B164]))
#define MCF_USB_EP3OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B194]))
#define MCF_USB_EP4OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B1C4]))
#define MCF_USB_EP5OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B1F4]))
#define MCF_USB_EP6OUTIFR        (*(vuint8_t *)(&__MBAR[0x00B224]))
#define MCF_USB_EPnOUTIFR(x)     (*(vuint8_t *)(&__MBAR[0x00B134+((x)*0x030)]))
#define MCF_USB_EP1OUTSR         (*(vuint8_t *)(&__MBAR[0x00B135]))
#define MCF_USB_EP2OUTSR         (*(vuint8_t *)(&__MBAR[0x00B165]))
#define MCF_USB_EP3OUTSR         (*(vuint8_t *)(&__MBAR[0x00B195]))
#define MCF_USB_EP4OUTSR         (*(vuint8_t *)(&__MBAR[0x00B1C5]))
#define MCF_USB_EP5OUTSR         (*(vuint8_t *)(&__MBAR[0x00B1F5]))
#define MCF_USB_EP6OUTSR         (*(vuint8_t *)(&__MBAR[0x00B225]))
#define MCF_USB_EPnOUTSR(x)      (*(vuint8_t *)(&__MBAR[0x00B135+((x)*0x030)]))
#define MCF_USB_EP1OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B13E]))
#define MCF_USB_EP2OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B16E]))
#define MCF_USB_EP3OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B19E]))
#define MCF_USB_EP4OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B1CE]))
#define MCF_USB_EP5OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B1FE]))
#define MCF_USB_EP6OUTSFR        (*(vuint16_t*)(&__MBAR[0x00B22E]))
#define MCF_USB_EPnOUTSFR(x)     (*(vuint16_t*)(&__MBAR[0x00B13E+((x)*0x030)]))
#define MCF_USB_EP1INACR         (*(vuint8_t *)(&__MBAR[0x00B149]))
#define MCF_USB_EP2INACR         (*(vuint8_t *)(&__MBAR[0x00B179]))
#define MCF_USB_EP3INACR         (*(vuint8_t *)(&__MBAR[0x00B1A9]))
#define MCF_USB_EP4INACR         (*(vuint8_t *)(&__MBAR[0x00B1D9]))
#define MCF_USB_EP5INACR         (*(vuint8_t *)(&__MBAR[0x00B209]))
#define MCF_USB_EP6INACR         (*(vuint8_t *)(&__MBAR[0x00B239]))
#define MCF_USB_EPnINACR(x)      (*(vuint8_t *)(&__MBAR[0x00B149+((x)*0x030)]))
#define MCF_USB_EP1INMPSR        (*(vuint16_t*)(&__MBAR[0x00B14A]))
#define MCF_USB_EP2INMPSR        (*(vuint16_t*)(&__MBAR[0x00B17A]))
#define MCF_USB_EP3INMPSR        (*(vuint16_t*)(&__MBAR[0x00B1AA]))
#define MCF_USB_EP4INMPSR        (*(vuint16_t*)(&__MBAR[0x00B1DA]))
#define MCF_USB_EP5INMPSR        (*(vuint16_t*)(&__MBAR[0x00B20A]))
#define MCF_USB_EP6INMPSR        (*(vuint16_t*)(&__MBAR[0x00B23A]))
#define MCF_USB_EPnINMPSR(x)     (*(vuint16_t*)(&__MBAR[0x00B14A+((x)*0x030)]))
#define MCF_USB_EP1INIFR         (*(vuint8_t *)(&__MBAR[0x00B14C]))
#define MCF_USB_EP2INIFR         (*(vuint8_t *)(&__MBAR[0x00B17C]))
#define MCF_USB_EP3INIFR         (*(vuint8_t *)(&__MBAR[0x00B1AC]))
#define MCF_USB_EP4INIFR         (*(vuint8_t *)(&__MBAR[0x00B1DC]))
#define MCF_USB_EP5INIFR         (*(vuint8_t *)(&__MBAR[0x00B20C]))
#define MCF_USB_EP6INIFR         (*(vuint8_t *)(&__MBAR[0x00B23C]))
#define MCF_USB_EPnINIFR(x)      (*(vuint8_t *)(&__MBAR[0x00B14C+((x)*0x030)]))
#define MCF_USB_EP1INSR          (*(vuint8_t *)(&__MBAR[0x00B14D]))
#define MCF_USB_EP2INSR          (*(vuint8_t *)(&__MBAR[0x00B17D]))
#define MCF_USB_EP3INSR          (*(vuint8_t *)(&__MBAR[0x00B1AD]))
#define MCF_USB_EP4INSR          (*(vuint8_t *)(&__MBAR[0x00B1DD]))
#define MCF_USB_EP5INSR          (*(vuint8_t *)(&__MBAR[0x00B20D]))
#define MCF_USB_EP6INSR          (*(vuint8_t *)(&__MBAR[0x00B23D]))
#define MCF_USB_EPnINSR(x)       (*(vuint8_t *)(&__MBAR[0x00B14D+((x)*0x030)]))
#define MCF_USB_EP1INSFR         (*(vuint16_t*)(&__MBAR[0x00B15A]))
#define MCF_USB_EP2INSFR         (*(vuint16_t*)(&__MBAR[0x00B18A]))
#define MCF_USB_EP3INSFR         (*(vuint16_t*)(&__MBAR[0x00B1BA]))
#define MCF_USB_EP4INSFR         (*(vuint16_t*)(&__MBAR[0x00B1EA]))
#define MCF_USB_EP5INSFR         (*(vuint16_t*)(&__MBAR[0x00B21A]))
#define MCF_USB_EP6INSFR         (*(vuint16_t*)(&__MBAR[0x00B24A]))
#define MCF_USB_EPnINSFR(x)      (*(vuint16_t*)(&__MBAR[0x00B15A+((x)*0x030)]))
#define MCF_USB_USBSR            (*(vuint32_t*)(&__MBAR[0x00B400]))
#define MCF_USB_USBCR            (*(vuint32_t*)(&__MBAR[0x00B404]))
#define MCF_USB_DRAMCR           (*(vuint32_t*)(&__MBAR[0x00B408]))
#define MCF_USB_DRAMDR           (*(vuint32_t*)(&__MBAR[0x00B40C]))
#define MCF_USB_USBISR           (*(vuint32_t*)(&__MBAR[0x00B410]))
#define MCF_USB_USBIMR           (*(vuint32_t*)(&__MBAR[0x00B414]))
#define MCF_USB_EP0STAT          (*(vuint32_t*)(&__MBAR[0x00B440]))
#define MCF_USB_EP1STAT          (*(vuint32_t*)(&__MBAR[0x00B470]))
#define MCF_USB_EP2STAT          (*(vuint32_t*)(&__MBAR[0x00B4A0]))
#define MCF_USB_EP3STAT          (*(vuint32_t*)(&__MBAR[0x00B4D0]))
#define MCF_USB_EP4STAT          (*(vuint32_t*)(&__MBAR[0x00B500]))
#define MCF_USB_EP5STAT          (*(vuint32_t*)(&__MBAR[0x00B530]))
#define MCF_USB_EP6STAT          (*(vuint32_t*)(&__MBAR[0x00B560]))
#define MCF_USB_EPnSTAT(x)       (*(vuint32_t*)(&__MBAR[0x00B440+((x)*0x030)]))
#define MCF_USB_EP0ISR           (*(vuint32_t*)(&__MBAR[0x00B444]))
#define MCF_USB_EP1ISR           (*(vuint32_t*)(&__MBAR[0x00B474]))
#define MCF_USB_EP2ISR           (*(vuint32_t*)(&__MBAR[0x00B4A4]))
#define MCF_USB_EP3ISR           (*(vuint32_t*)(&__MBAR[0x00B4D4]))
#define MCF_USB_EP4ISR           (*(vuint32_t*)(&__MBAR[0x00B504]))
#define MCF_USB_EP5ISR           (*(vuint32_t*)(&__MBAR[0x00B534]))
#define MCF_USB_EP6ISR           (*(vuint32_t*)(&__MBAR[0x00B564]))
#define MCF_USB_EPnISR(x)        (*(vuint32_t*)(&__MBAR[0x00B444+((x)*0x030)]))
#define MCF_USB_EP0IMR           (*(vuint32_t*)(&__MBAR[0x00B448]))
#define MCF_USB_EP1IMR           (*(vuint32_t*)(&__MBAR[0x00B478]))
#define MCF_USB_EP2IMR           (*(vuint32_t*)(&__MBAR[0x00B4A8]))
#define MCF_USB_EP3IMR           (*(vuint32_t*)(&__MBAR[0x00B4D8]))
#define MCF_USB_EP4IMR           (*(vuint32_t*)(&__MBAR[0x00B508]))
#define MCF_USB_EP5IMR           (*(vuint32_t*)(&__MBAR[0x00B538]))
#define MCF_USB_EP6IMR           (*(vuint32_t*)(&__MBAR[0x00B568]))
#define MCF_USB_EPnIMR(x)        (*(vuint32_t*)(&__MBAR[0x00B448+((x)*0x030)]))
#define MCF_USB_EP0FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B44C]))
#define MCF_USB_EP1FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B47C]))
#define MCF_USB_EP2FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B4AC]))
#define MCF_USB_EP3FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B4DC]))
#define MCF_USB_EP4FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B50C]))
#define MCF_USB_EP5FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B53C]))
#define MCF_USB_EP6FRCFGR        (*(vuint32_t*)(&__MBAR[0x00B56C]))
#define MCF_USB_EPnFRCFGR(x)     (*(vuint32_t*)(&__MBAR[0x00B44C+((x)*0x030)]))
#define MCF_USB_EP0FDR           (*(vuint32_t*)(&__MBAR[0x00B450]))
#define MCF_USB_EP1FDR           (*(vuint32_t*)(&__MBAR[0x00B480]))
#define MCF_USB_EP2FDR           (*(vuint32_t*)(&__MBAR[0x00B4B0]))
#define MCF_USB_EP3FDR           (*(vuint32_t*)(&__MBAR[0x00B4E0]))
#define MCF_USB_EP4FDR           (*(vuint32_t*)(&__MBAR[0x00B510]))
#define MCF_USB_EP5FDR           (*(vuint32_t*)(&__MBAR[0x00B540]))
#define MCF_USB_EP6FDR           (*(vuint32_t*)(&__MBAR[0x00B570]))
#define MCF_USB_EPnFDR(x)        (*(vuint32_t*)(&__MBAR[0x00B450+((x)*0x030)]))
#define MCF_USB_EP0FSR           (*(vuint32_t*)(&__MBAR[0x00B454]))
#define MCF_USB_EP1FSR           (*(vuint32_t*)(&__MBAR[0x00B484]))
#define MCF_USB_EP2FSR           (*(vuint32_t*)(&__MBAR[0x00B4B4]))
#define MCF_USB_EP3FSR           (*(vuint32_t*)(&__MBAR[0x00B4E4]))
#define MCF_USB_EP4FSR           (*(vuint32_t*)(&__MBAR[0x00B514]))
#define MCF_USB_EP5FSR           (*(vuint32_t*)(&__MBAR[0x00B544]))
#define MCF_USB_EP6FSR           (*(vuint32_t*)(&__MBAR[0x00B574]))
#define MCF_USB_EPnFSR(x)        (*(vuint32_t*)(&__MBAR[0x00B454+((x)*0x030)]))
#define MCF_USB_EP0FCR           (*(vuint32_t*)(&__MBAR[0x00B458]))
#define MCF_USB_EP1FCR           (*(vuint32_t*)(&__MBAR[0x00B488]))
#define MCF_USB_EP2FCR           (*(vuint32_t*)(&__MBAR[0x00B4B8]))
#define MCF_USB_EP3FCR           (*(vuint32_t*)(&__MBAR[0x00B4E8]))
#define MCF_USB_EP4FCR           (*(vuint32_t*)(&__MBAR[0x00B518]))
#define MCF_USB_EP5FCR           (*(vuint32_t*)(&__MBAR[0x00B548]))
#define MCF_USB_EP6FCR           (*(vuint32_t*)(&__MBAR[0x00B578]))
#define MCF_USB_EPnFCR(x)        (*(vuint32_t*)(&__MBAR[0x00B458+((x)*0x030)]))
#define MCF_USB_EP0FAR           (*(vuint32_t*)(&__MBAR[0x00B45C]))
#define MCF_USB_EP1FAR           (*(vuint32_t*)(&__MBAR[0x00B48C]))
#define MCF_USB_EP2FAR           (*(vuint32_t*)(&__MBAR[0x00B4BC]))
#define MCF_USB_EP3FAR           (*(vuint32_t*)(&__MBAR[0x00B4EC]))
#define MCF_USB_EP4FAR           (*(vuint32_t*)(&__MBAR[0x00B51C]))
#define MCF_USB_EP5FAR           (*(vuint32_t*)(&__MBAR[0x00B54C]))
#define MCF_USB_EP6FAR           (*(vuint32_t*)(&__MBAR[0x00B57C]))
#define MCF_USB_EPnFAR(x)        (*(vuint32_t*)(&__MBAR[0x00B45C+((x)*0x030)]))
#define MCF_USB_EP0FRP           (*(vuint32_t*)(&__MBAR[0x00B460]))
#define MCF_USB_EP1FRP           (*(vuint32_t*)(&__MBAR[0x00B490]))
#define MCF_USB_EP2FRP           (*(vuint32_t*)(&__MBAR[0x00B4C0]))
#define MCF_USB_EP3FRP           (*(vuint32_t*)(&__MBAR[0x00B4F0]))
#define MCF_USB_EP4FRP           (*(vuint32_t*)(&__MBAR[0x00B520]))
#define MCF_USB_EP5FRP           (*(vuint32_t*)(&__MBAR[0x00B550]))
#define MCF_USB_EP6FRP           (*(vuint32_t*)(&__MBAR[0x00B580]))
#define MCF_USB_EPnFRP(x)        (*(vuint32_t*)(&__MBAR[0x00B460+((x)*0x030)]))
#define MCF_USB_EP0FWP           (*(vuint32_t*)(&__MBAR[0x00B464]))
#define MCF_USB_EP1FWP           (*(vuint32_t*)(&__MBAR[0x00B494]))
#define MCF_USB_EP2FWP           (*(vuint32_t*)(&__MBAR[0x00B4C4]))
#define MCF_USB_EP3FWP           (*(vuint32_t*)(&__MBAR[0x00B4F4]))
#define MCF_USB_EP4FWP           (*(vuint32_t*)(&__MBAR[0x00B524]))
#define MCF_USB_EP5FWP           (*(vuint32_t*)(&__MBAR[0x00B554]))
#define MCF_USB_EP6FWP           (*(vuint32_t*)(&__MBAR[0x00B584]))
#define MCF_USB_EPnFWP(x)        (*(vuint32_t*)(&__MBAR[0x00B464+((x)*0x030)]))
#define MCF_USB_EP0LRFP          (*(vuint32_t*)(&__MBAR[0x00B468]))
#define MCF_USB_EP1LRFP          (*(vuint32_t*)(&__MBAR[0x00B498]))
#define MCF_USB_EP2LRFP          (*(vuint32_t*)(&__MBAR[0x00B4C8]))
#define MCF_USB_EP3LRFP          (*(vuint32_t*)(&__MBAR[0x00B4F8]))
#define MCF_USB_EP4LRFP          (*(vuint32_t*)(&__MBAR[0x00B528]))
#define MCF_USB_EP5LRFP          (*(vuint32_t*)(&__MBAR[0x00B558]))
#define MCF_USB_EP6LRFP          (*(vuint32_t*)(&__MBAR[0x00B588]))
#define MCF_USB_EPnLRFP(x)       (*(vuint32_t*)(&__MBAR[0x00B468+((x)*0x030)]))
#define MCF_USB_EP0LWFP          (*(vuint32_t*)(&__MBAR[0x00B46C]))
#define MCF_USB_EP1LWFP          (*(vuint32_t*)(&__MBAR[0x00B49C]))
#define MCF_USB_EP2LWFP          (*(vuint32_t*)(&__MBAR[0x00B4CC]))
#define MCF_USB_EP3LWFP          (*(vuint32_t*)(&__MBAR[0x00B4FC]))
#define MCF_USB_EP4LWFP          (*(vuint32_t*)(&__MBAR[0x00B52C]))
#define MCF_USB_EP5LWFP          (*(vuint32_t*)(&__MBAR[0x00B55C]))
#define MCF_USB_EP6LWFP          (*(vuint32_t*)(&__MBAR[0x00B58C]))
#define MCF_USB_EPnLWFP(x)       (*(vuint32_t*)(&__MBAR[0x00B46C+((x)*0x030)]))

/* Bit definitions and macros for MCF_USB_USBAISR */
#define MCF_USB_USBAISR_SETUP             (0x01)
#define MCF_USB_USBAISR_IN                (0x02)
#define MCF_USB_USBAISR_OUT               (0x04)
#define MCF_USB_USBAISR_EPHALT            (0x08)
#define MCF_USB_USBAISR_TRANSERR          (0x10)
#define MCF_USB_USBAISR_ACK               (0x20)
#define MCF_USB_USBAISR_CTROVFL           (0x40)
#define MCF_USB_USBAISR_EPSTALL           (0x80)

/* Bit definitions and macros for MCF_USB_USBAIMR */
#define MCF_USB_USBAIMR_SETUPEN           (0x01)
#define MCF_USB_USBAIMR_INEN              (0x02)
#define MCF_USB_USBAIMR_OUTEN             (0x04)
#define MCF_USB_USBAIMR_EPHALTEN          (0x08)
#define MCF_USB_USBAIMR_TRANSERREN        (0x10)
#define MCF_USB_USBAIMR_ACKEN             (0x20)
#define MCF_USB_USBAIMR_CTROVFLEN         (0x40)
#define MCF_USB_USBAIMR_EPSTALLEN         (0x80)

/* Bit definitions and macros for MCF_USB_EPINFO */
#define MCF_USB_EPINFO_EPDIR              (0x01)
#define MCF_USB_EPINFO_EPNUM(x)           (((x)&0x07)<<1)

/* Bit definitions and macros for MCF_USB_CFGAR */
#define MCF_USB_CFGAR_RESERVED            (0xA0)
#define MCF_USB_CFGAR_RMTWKEUP            (0xE0)

/* Bit definitions and macros for MCF_USB_SPEEDR */
#define MCF_USB_SPEEDR_HS                 (0x01)
#define MCF_USB_SPEEDR_FS                 (0x02)

/* Bit definitions and macros for MCF_USB_FRMNUMR */
#define MCF_USB_FRMNUMR_FRMNUM(x)         (((x)&0x0FFF)<<0)

/* Bit definitions and macros for MCF_USB_EPTNR */
#define MCF_USB_EPTNR_EP1T(x)             (((x)&0x0003)<<0)
#define MCF_USB_EPTNR_EP2T(x)             (((x)&0x0003)<<2)
#define MCF_USB_EPTNR_EP3T(x)             (((x)&0x0003)<<4)
#define MCF_USB_EPTNR_EP4T(x)             (((x)&0x0003)<<6)
#define MCF_USB_EPTNR_EP5T(x)             (((x)&0x0003)<<8)
#define MCF_USB_EPTNR_EP6T(x)             (((x)&0x0003)<<10)
#define MCF_USB_EPTNR_EPnT1               (0)
#define MCF_USB_EPTNR_EPnT2               (1)
#define MCF_USB_EPTNR_EPnT3               (2)

/* Bit definitions and macros for MCF_USB_IFUR */
#define MCF_USB_IFUR_ALTSET(x)            (((x)&0x00FF)<<0)
#define MCF_USB_IFUR_IFNUM(x)             (((x)&0x00FF)<<8)

/* Bit definitions and macros for MCF_USB_IFRn */
#define MCF_USB_IFRn_ALTSET(x)            (((x)&0x00FF)<<0)
#define MCF_USB_IFRn_IFNUM(x)             (((x)&0x00FF)<<8)

/* Bit definitions and macros for MCF_USB_CNTOVR */
#define MCF_USB_CNTOVR_PPCNT              (0x01)
#define MCF_USB_CNTOVR_DPCNT              (0x02)
#define MCF_USB_CNTOVR_CRCECNT            (0x04)
#define MCF_USB_CNTOVR_BSECNT             (0x08)
#define MCF_USB_CNTOVR_PIDECNT            (0x10)
#define MCF_USB_CNTOVR_FRMECNT            (0x20)
#define MCF_USB_CNTOVR_TXPCNT             (0x40)

/* Bit definitions and macros for MCF_USB_EP0ACR */
#define MCF_USB_EP0ACR_TTYPE(x)           (((x)&0x03)<<0)
#define MCF_USB_EP0ACR_TTYPE_CTRL         (0)
#define MCF_USB_EP0ACR_TTYPE_ISOC         (1)
#define MCF_USB_EP0ACR_TTYPE_BULK         (2)
#define MCF_USB_EP0ACR_TTYPE_INT          (3)

/* Bit definitions and macros for MCF_USB_EP0MPSR */
#define MCF_USB_EP0MPSR_MAXPKTSZ(x)       (((x)&0x07FF)<<0)
#define MCF_USB_EP0MPSR_ADDTRANS(x)       (((x)&0x0003)<<11)

/* Bit definitions and macros for MCF_USB_EP0SR */
#define MCF_USB_EP0SR_HALT                (0x01)
#define MCF_USB_EP0SR_ACTIVE              (0x02)
#define MCF_USB_EP0SR_PSTALL              (0x04)
#define MCF_USB_EP0SR_CCOMP               (0x08)
#define MCF_USB_EP0SR_TXZERO              (0x20)
#define MCF_USB_EP0SR_INT                 (0x80)

/* Bit definitions and macros for MCF_USB_BMRTR */
#define MCF_USB_BMRTR_DIR                 (0x80)
#define MCF_USB_BMRTR_TYPE_STANDARD       (0x00)
#define MCF_USB_BMRTR_TYPE_CLASS          (0x20)
#define MCF_USB_BMRTR_TYPE_VENDOR         (0x40)
#define MCF_USB_BMRTR_REC_DEVICE          (0x00)
#define MCF_USB_BMRTR_REC_INTERFACE       (0x01)
#define MCF_USB_BMRTR_REC_ENDPOINT        (0x02)
#define MCF_USB_BMRTR_REC_OTHER           (0x03)

/* Bit definitions and macros for MCF_USB_EPnOUTACR */
#define MCF_USB_EPnOUTACR_TTYPE(x)        (((x)&0x03)<<0)

/* Bit definitions and macros for MCF_USB_EPnOUTMPSR */
#define MCF_USB_EPnOUTMPSR_MAXPKTSZ(x)    (((x)&0x07FF)<<0)
#define MCF_USB_EPnOUTMPSR_ADDTRANS(x)    (((x)&0x0003)<<11)

/* Bit definitions and macros for MCF_USB_EPnOUTSR */
#define MCF_USB_EPnOUTSR_HALT             (0x01)
#define MCF_USB_EPnOUTSR_ACTIVE           (0x02)
#define MCF_USB_EPnOUTSR_PSTALL           (0x04)
#define MCF_USB_EPnOUTSR_CCOMP            (0x08)
#define MCF_USB_EPnOUTSR_TXZERO           (0x20)
#define MCF_USB_EPnOUTSR_INT              (0x80)

/* Bit definitions and macros for MCF_USB_EPnOUTSFR */
#define MCF_USB_EPnOUTSFR_FRMNUM(x)       (((x)&0x07FF)<<0)

/* Bit definitions and macros for MCF_USB_EPnINACR */
#define MCF_USB_EPnINACR_TTYPE(x)         (((x)&0x03)<<0)

/* Bit definitions and macros for MCF_USB_EPnINMPSR */
#define MCF_USB_EPnINMPSR_MAXPKTSZ(x)     (((x)&0x07FF)<<0)
#define MCF_USB_EPnINMPSR_ADDTRANS(x)     (((x)&0x0003)<<11)

/* Bit definitions and macros for MCF_USB_EPnINSR */
#define MCF_USB_EPnINSR_HALT              (0x01)
#define MCF_USB_EPnINSR_ACTIVE            (0x02)
#define MCF_USB_EPnINSR_PSTALL            (0x04)
#define MCF_USB_EPnINSR_CCOMP             (0x08)
#define MCF_USB_EPnINSR_TXZERO            (0x20)
#define MCF_USB_EPnINSR_INT               (0x80)

/* Bit definitions and macros for MCF_USB_EPnINSFR */
#define MCF_USB_EPnINSFR_FRMNUM(x)        (((x)&0x07FF)<<0)

/* Bit definitions and macros for MCF_USB_USBSR */
#define MCF_USB_USBSR_SUSP                (0x00000080)
#define MCF_USB_USBSR_ISOERREP            (0x0000000F)

/* Bit definitions and macros for MCF_USB_USBCR */
#define MCF_USB_USBCR_RESUME              (0x00000001)
#define MCF_USB_USBCR_APPLOCK             (0x00000002)
#define MCF_USB_USBCR_RST                 (0x00000004)
#define MCF_USB_USBCR_RAMEN               (0x00000008)
#define MCF_USB_USBCR_RAMSPLIT            (0x00000020)

/* Bit definitions and macros for MCF_USB_DRAMCR */
#define MCF_USB_DRAMCR_DADR(x)            (((x)&0x000003FF)<<0)
#define MCF_USB_DRAMCR_DSIZE(x)           (((x)&0x000007FF)<<16)
#define MCF_USB_DRAMCR_BSY                (0x40000000)
#define MCF_USB_DRAMCR_START              (0x80000000)

/* Bit definitions and macros for MCF_USB_DRAMDR */
#define MCF_USB_DRAMDR_DDAT(x)            (((x)&0x000000FF)<<0)

/* Bit definitions and macros for MCF_USB_USBISR */
#define MCF_USB_USBISR_ISOERR             (0x00000001)
#define MCF_USB_USBISR_FTUNLCK            (0x00000002)
#define MCF_USB_USBISR_SUSP               (0x00000004)
#define MCF_USB_USBISR_RES                (0x00000008)
#define MCF_USB_USBISR_UPDSOF             (0x00000010)
#define MCF_USB_USBISR_RSTSTOP            (0x00000020)
#define MCF_USB_USBISR_SOF                (0x00000040)
#define MCF_USB_USBISR_MSOF               (0x00000080)

/* Bit definitions and macros for MCF_USB_USBIMR */
#define MCF_USB_USBIMR_ISOERR             (0x00000001)
#define MCF_USB_USBIMR_FTUNLCK            (0x00000002)
#define MCF_USB_USBIMR_SUSP               (0x00000004)
#define MCF_USB_USBIMR_RES                (0x00000008)
#define MCF_USB_USBIMR_UPDSOF             (0x00000010)
#define MCF_USB_USBIMR_RSTSTOP            (0x00000020)
#define MCF_USB_USBIMR_SOF                (0x00000040)
#define MCF_USB_USBIMR_MSOF               (0x00000080)

/* Bit definitions and macros for MCF_USB_EPnSTAT */
#define MCF_USB_EPnSTAT_RST               (0x00000001)
#define MCF_USB_EPnSTAT_FLUSH             (0x00000002)
#define MCF_USB_EPnSTAT_DIR               (0x00000080)
#define MCF_USB_EPnSTAT_BYTECNT(x)        (((x)&0x00000FFF)<<16)

/* Bit definitions and macros for MCF_USB_EPnISR */
#define MCF_USB_EPnISR_EOF                (0x00000001)
#define MCF_USB_EPnISR_EOT                (0x00000004)
#define MCF_USB_EPnISR_FIFOLO             (0x00000010)
#define MCF_USB_EPnISR_FIFOHI             (0x00000020)
#define MCF_USB_EPnISR_ERR                (0x00000040)
#define MCF_USB_EPnISR_EMT                (0x00000080)
#define MCF_USB_EPnISR_FU                 (0x00000100)

/* Bit definitions and macros for MCF_USB_EPnIMR */
#define MCF_USB_EPnIMR_EOF                (0x00000001)
#define MCF_USB_EPnIMR_EOT                (0x00000004)
#define MCF_USB_EPnIMR_FIFOLO             (0x00000010)
#define MCF_USB_EPnIMR_FIFOHI             (0x00000020)
#define MCF_USB_EPnIMR_ERR                (0x00000040)
#define MCF_USB_EPnIMR_EMT                (0x00000080)
#define MCF_USB_EPnIMR_FU                 (0x00000100)

/* Bit definitions and macros for MCF_USB_EPnFRCFGR */
#define MCF_USB_EPnFRCFGR_DEPTH(x)        (((x)&0x00001FFF)<<0)
#define MCF_USB_EPnFRCFGR_BASE(x)         (((x)&0x00000FFF)<<16)

/* Bit definitions and macros for MCF_USB_EPnFSR */
#define MCF_USB_EPnFSR_EMT                (0x00010000)
#define MCF_USB_EPnFSR_ALRM               (0x00020000)
#define MCF_USB_EPnFSR_FR                 (0x00040000)
#define MCF_USB_EPnFSR_FU                 (0x00080000)
#define MCF_USB_EPnFSR_OF                 (0x00100000)
#define MCF_USB_EPnFSR_UF                 (0x00200000)
#define MCF_USB_EPnFSR_RXW                (0x00400000)
#define MCF_USB_EPnFSR_FAE                (0x00800000)
#define MCF_USB_EPnFSR_FRM(x)             (((x)&0x0000000F)<<24)
#define MCF_USB_EPnFSR_TXW                (0x40000000)
#define MCF_USB_EPnFSR_IP                 (0x80000000)

/* Bit definitions and macros for MCF_USB_EPnFCR */
#define MCF_USB_EPnFCR_COUNTER(x)         (((x)&0x0000FFFF)<<0)
#define MCF_USB_EPnFCR_TXWMSK             (0x00040000)
#define MCF_USB_EPnFCR_OFMSK              (0x00080000)
#define MCF_USB_EPnFCR_UFMSK              (0x00100000)
#define MCF_USB_EPnFCR_RXWMSK             (0x00200000)
#define MCF_USB_EPnFCR_FAEMSK             (0x00400000)
#define MCF_USB_EPnFCR_IPMSK              (0x00800000)
#define MCF_USB_EPnFCR_GR(x)              (((x)&0x00000007)<<24)
#define MCF_USB_EPnFCR_FRM                (0x08000000)
#define MCF_USB_EPnFCR_TMR                (0x10000000)
#define MCF_USB_EPnFCR_WFR                (0x20000000)
#define MCF_USB_EPnFCR_SHAD               (0x80000000)

/* Bit definitions and macros for MCF_USB_EPnFAR */
#define MCF_USB_EPnFAR_ALRMP(x)           (((x)&0x00000FFF)<<0)

/* Bit definitions and macros for MCF_USB_EPnFRP */
#define MCF_USB_EPnFRP_RP(x)              (((x)&0x00000FFF)<<0)

/* Bit definitions and macros for MCF_USB_EPnFWP */
#define MCF_USB_EPnFWP_WP(x)              (((x)&0x00000FFF)<<0)

/* Bit definitions and macros for MCF_USB_EPnLRFP */
#define MCF_USB_EPnLRFP_LRFP(x)           (((x)&0x00000FFF)<<0)

/* Bit definitions and macros for MCF_USB_EPnLWFP */
#define MCF_USB_EPnLWFP_LWFP(x)           (((x)&0x00000FFF)<<0)


#endif /* __MCF548X_USB_H__ */

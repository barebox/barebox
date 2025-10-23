/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Initialize the Si5338 clock generator.
 *
 * Datasheet: https://www.skyworksinc.com/en/application-pages/-/media/SkyWorks/SL/documents/public/data-sheets/Si5338.pdf
 * Reference manual: https://www.skyworksinc.com/-/media/Skyworks/SL/documents/public/reference-manuals/Si5338-RM.pdf
 */

#pragma once

/**
 * @brief Initialize the SI5338
 *
 * Get the I²C address from the devicetree and write the registers of the
 * SI5338 after the configuration in \c Si5338-RevB-Registers.h, generated
 * by [ClockBuilder Pro](https://www.skyworksinc.com/Application-Pages/Clockbuilder-Pro-Software).
 *
 * @return 0 on success, a negative value from `asm-generic/errno.h` on error.
 */

int si5338_init(void);

/*
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#if defined(CONFIG_MACH_EDB9301)
#define MACH_TYPE MACH_TYPE_EDB9301
#elif defined(CONFIG_MACH_EDB9302)
#define MACH_TYPE MACH_TYPE_EDB9302
#elif defined(CONFIG_MACH_EDB9302A)
#define MACH_TYPE MACH_TYPE_EDB9302A
#elif defined(CONFIG_MACH_EDB9307)
#define MACH_TYPE MACH_TYPE_EDB9307
#elif defined(CONFIG_MACH_EDB9307A)
#define MACH_TYPE MACH_TYPE_EDB9307A
#elif defined(CONFIG_MACH_EDB9312)
#define MACH_TYPE MACH_TYPE_EDB9312
#elif defined(CONFIG_MACH_EDB9315)
#define MACH_TYPE MACH_TYPE_EDB9315
#elif defined(CONFIG_MACH_EDB9315A)
#define MACH_TYPE MACH_TYPE_EDB9315A
#endif

#if defined(CONFIG_MACH_EDB9301) || defined(CONFIG_MACH_EDB9302) || \
	defined(CONFIG_MACH_EDB9302A)
#define EDB93XX_CFI_FLASH_SIZE (16 * 1024 * 1024)
#elif defined(CONFIG_MACH_EDB9307) || defined(CONFIG_MACH_EDB9307A) || \
	defined(CONFIG_MACH_EDB9312) || defined(CONFIG_MACH_EDB9315) ||	\
	defined(CONFIG_MACH_EDB9315A)
#define EDB93XX_CFI_FLASH_SIZE (32 * 1024 * 1024)
#endif

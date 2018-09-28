/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * Authors: Adrian Hunter
 *          Artem Bityutskiy (Битюцкий Артём)
 */

/*
 * This file implements commit-related functionality of the LEB properties
 * subsystem.
 */

#include <linux/err.h>
#include "crc16.h"
#include "ubifs.h"

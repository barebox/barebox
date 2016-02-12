/*
 * CAAM Error Reporting code header
 *
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 */

#ifndef CAAM_ERROR_H
#define CAAM_ERROR_H
#define CAAM_ERROR_STR_MAX 302
void caam_jr_strstatus(struct device_d *jrdev, u32 status);
#endif /* CAAM_ERROR_H */

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 */

struct ablkcipher_request;

int imx_scc_cbc_des_encrypt(struct ablkcipher_request *req);
int imx_scc_cbc_des_decrypt(struct ablkcipher_request *req);
int imx_scc_blob_gen_probe(struct device_d *dev);

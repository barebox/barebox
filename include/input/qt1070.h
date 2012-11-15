/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __QT1070_H__
#define __QT1070_H__

#define QT1070_NB_BUTTONS	7

struct qt1070_platform_data {
	int code[QT1070_NB_BUTTONS];
	int nb_code;
	int irq_pin;
};

#endif /* __QT1070_H__ */

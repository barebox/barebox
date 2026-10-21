Deep probe is now the default on STM32MP13 and STM32MP15
--------------------------------------------------------

barebox now enables deep probe for every ``st,stm32mp1xx`` compatible, even
over ``barebox,disable-deep-probe``. Report to the mailing list if that
breaks your board.

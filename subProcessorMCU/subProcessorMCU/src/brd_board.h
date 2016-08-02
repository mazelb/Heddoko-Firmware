/*
 * brd_board.h
 *
 * Created: 2/29/2016 2:39:00 PM
 *  Author: sean
 */ 


#ifndef BRD_BOARD_H_
#define BRD_BOARD_H_

/** Baudrate setting : 115200 */
#define CONF_TEST_BAUDRATE   115200
/** Char setting     : 8-bit character length (don't care for UART) */
#define CONF_TEST_CHARLENGTH 0
/** Parity setting   : No parity check */
#define CONF_TEST_PARITY     UART_MR_PAR_NO
/** Stopbit setting  : No extra stopbit, i.e., use 1 (don't care for UART) */
#define CONF_TEST_STOPBITS   false

void brd_board_init();
void brd_initAllUarts();
void brd_deInitAllUarts();
void brd_enableWatchdog();

#endif /* BRD_BOARD_H_ */
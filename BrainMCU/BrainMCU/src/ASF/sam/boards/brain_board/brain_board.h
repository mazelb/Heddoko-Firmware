/*
 * brain_board.h
 *
 * Created: 10/14/2015 9:45:10 AM
 *  Author: Sean Cloghesy
 */ 


#ifndef BRAIN_BOARD_H_
#define BRAIN_BOARD_H_

//! Name string macro
#define BOARD_NAME                "BRAIN BOARD"

//! \name Resonator definitions
//@{
#define BOARD_FREQ_SLCK_XTAL      (32768U)
#define BOARD_FREQ_SLCK_BYPASS    (32768U)
#define BOARD_FREQ_MAINCK_XTAL    (16000000U)
#define BOARD_FREQ_MAINCK_BYPASS  (16000000U)
#define BOARD_MCK                 CHIP_FREQ_CPU_MAX
#define BOARD_OSC_STARTUP_US      15625
//@}


//! \name Embedded debugger GPIO interface definitions
//@{
#define EDBG_GPIO0_PIN            IOPORT_CREATE_PIN(PIOA, 6)
#define EDBG_GPIO1_PIN            IOPORT_CREATE_PIN(PIOC, 22)
#define EDBG_GPIO2_PIN            IOPORT_CREATE_PIN(PIOC, 24)
#define EDBG_GPIO3_PIN            IOPORT_CREATE_PIN(PIOC, 25)
//@}

//! \name Embedded debugger USART interface definitions
//@{
#define EDBG_UART_MODULE          UART1
//@}

//! \name Embedded debugger TWI interface definitions
//@{
#define EDBG_TWI_MODULE           TWI0
//@}

//! \name Embedded debugger SPI interface definitions
//@{
#define EDBG_SPI_MODULE           SPI
//@}

#define CONSOLE_UART              UART0
#define CONSOLE_UART_ID           ID_UART0

/** UART1 pins (UTXD1 and URXD1) definitions, PB2,PB3. */
#define PINS_UART1                (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1)
#define PINS_UART1_FLAGS          (PIO_PERIPH_A | PIO_DEFAULT)
#define PINS_UART1_PIO            PIOB


//! \name SD Card
//@{
#define SD_MMC_0_CD_GPIO            (PIO_PB11_IDX)
#define SD_MMC_0_CD_PIO_ID          ID_PIOB
#define SD_MMC_0_CD_FLAGS           (PIO_INPUT | PIO_PULLUP)

#define SD_MMC_0_CD_DETECT_VALUE    1

#define SD_MMC_0_CD    {PIO_PB11, PIOB, ID_PIOB, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE}
#define SD_MMC_0_CD_MASK PIO_PB11
#define SD_MMC_0_CD_PIO PIOB
#define SD_MMC_0_CD_ID ID_PIOB
#define SD_MMC_0_CD_TYPE PIO_INPUT
#define SD_MMC_0_CD_ATTR (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE)

/** HSMCI pins definition. */
/*! Number of slot connected on HSMCI interface */
#define SD_MMC_HSMCI_MEM_CNT      1
#define SD_MMC_HSMCI_SLOT_0_SIZE  4
#define PINS_HSMCI   {0x3fUL << 26, PIOA, ID_PIOA, PIO_PERIPH_C, PIO_PULLUP}
/** HSMCI MCCDA pin definition. */
#define PIN_HSMCI_MCCDA_GPIO            (PIO_PA28_IDX)
#define PIN_HSMCI_MCCDA_FLAGS           (PIO_PERIPH_C | PIO_DEFAULT)
/** HSMCI MCCK pin definition. */
#define PIN_HSMCI_MCCK_GPIO             (PIO_PA29_IDX)
#define PIN_HSMCI_MCCK_FLAGS            (PIO_PERIPH_C | PIO_DEFAULT)
/** HSMCI MCDA0 pin definition. */
#define PIN_HSMCI_MCDA0_GPIO            (PIO_PA30_IDX)
#define PIN_HSMCI_MCDA0_FLAGS           (PIO_PERIPH_C | PIO_DEFAULT)
/** HSMCI MCDA1 pin definition. */
#define PIN_HSMCI_MCDA1_GPIO            (PIO_PA31_IDX)
#define PIN_HSMCI_MCDA1_FLAGS           (PIO_PERIPH_C | PIO_DEFAULT)
/** HSMCI MCDA2 pin definition. */
#define PIN_HSMCI_MCDA2_GPIO            (PIO_PA26_IDX)
#define PIN_HSMCI_MCDA2_FLAGS           (PIO_PERIPH_C | PIO_DEFAULT)
/** HSMCI MCDA3 pin definition. */
#define PIN_HSMCI_MCDA3_GPIO            (PIO_PA27_IDX)
#define PIN_HSMCI_MCDA3_FLAGS           (PIO_PERIPH_C | PIO_DEFAULT)
/** SD/MMC card detect pin definition. */
#define PIN_HSMCI_CD {PIO_PB11, PIOB, ID_PIOB, PIO_INPUT, PIO_PULLUP}


#endif /* BRAIN_BOARD_H_ */
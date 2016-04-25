/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup group_common_boards
 * \defgroup user_board_group User board
 *
 * @{
 */

void system_board_init(void);

/** Name string macro */
#define BOARD_NAME                "USER_BOARD"

/** @} */

#ifdef __cplusplus
}
#endif

/** \name LED0 definitions
 *  @{ */
#define LED0_PIN                  PIN_PA08
#define LED0_ACTIVE               true
#define LED0_INACTIVE             !LED0_ACTIVE
#define LED0_GPIO                 LED0_PIN
/** @} */

/** \name SW0 definitions
 *  @{ */
#define SW1_INT_PIN                   PIN_PA05
#define SW1_INT_ACTIVE                false
#define SW1_INT_INACTIVE              !SW1_INT_ACTIVE
#define SW1_INT_EIC_PIN               PIN_PA05A_EIC_EXTINT5
#define SW1_INT_EIC_MUX               MUX_PA05A_EIC_EXTINT5
#define SW1_INT_EIC_PINMUX            PINMUX_PA05A_EIC_EXTINT5
#define SW1_INT_EIC_LINE              5
/** @} */

/** \name SW1 definitions
 *  @{ */
#define SW2_PIN                   PIN_PA09
#define SW2_ACTIVE                false
#define SW2_INACTIVE              !SW2_ACTIVE
#define SW2_EIC_PIN               PIN_PA09A_EIC_EXTINT7
#define SW2_EIC_MUX               MUX_PA09A_EIC_EXTINT7
#define SW2_EIC_PINMUX            PINMUX_PA09A_EIC_EXTINT7
#define SW2_EIC_LINE              7
/** @} */

/**
 * \name LED #0 definitions
 *
 * Wrapper macros for LED0, to ensure common naming across all Xplained
 * boards.
 *
 *  @{ */
#define LED_0_NAME                "LED0 (yellow)"
#define LED_0_PIN                 LED0_PIN
#define LED_0_ACTIVE              LED0_ACTIVE
#define LED_0_INACTIVE            LED0_INACTIVE
/** @} */

/** Number of on-board LEDs */
#define LED_COUNT                 1

/**
 * \name Button #0 definitions
 *
 * Wrapper macros for SW0, to ensure common naming across all Xplained
 * boards.
 *
 *  @{ */
#define SW1_INT_NAME             "SW1_INT"
/** @} */




/** Number of on-board buttons */
#define BUTTON_COUNT 1


/** \name Embedded debugger CDC Gateway USART interface definitions
 * @{
 */
//#define USE_DEV_BOARD
#ifdef USE_DEV_BOARD
#define CMD_UART_MODULE              SERCOM2
#define CMD_UART_MUX_SETTING  USART_RX_3_TX_2_XCK_3
#define CMD_UART_PINMUX_PAD0  PINMUX_UNUSED
#define CMD_UART_PINMUX_PAD1  PINMUX_UNUSED
#define CMD_UART_PAD2  PINMUX_PA10D_SERCOM2_PAD2
#define CMD_UART_PAD3  PINMUX_PA11D_SERCOM2_PAD3
#define CMD_UART_DMAC_ID_TX   SERCOM2_DMAC_ID_TX
#define CMD_UART_DMAC_ID_RX   SERCOM2_DMAC_ID_RX


#else
#define CMD_UART_MODULE              SERCOM1
#define CMD_UART_MUX_SETTING		 USART_RX_3_TX_2_XCK_3
#define CMD_UART_PINMUX_PAD0  PINMUX_UNUSED
#define CMD_UART_PINMUX_PAD1  PINMUX_UNUSED
#define CMD_UART_PAD2  PINMUX_PA24C_SERCOM1_PAD2
#define CMD_UART_PAD3  PINMUX_PA25C_SERCOM1_PAD3
#define CMD_UART_DMAC_ID_TX   SERCOM2_DMAC_ID_TX
#define CMD_UART_DMAC_ID_RX   SERCOM2_DMAC_ID_RX
#endif
/** @} */
/**
 * \brief Turns off the specified LEDs.
 *
 * \param led_gpio LED to turn off (LEDx_GPIO).
 *
 * \note The pins of the specified LEDs are set to GPIO output mode.
 */
#define LED_Off(led_gpio)     port_pin_set_output_level(led_gpio, false)

/**
 * \brief Turns on the specified LEDs.
 *
 * \param led_gpio LED to turn on (LEDx_GPIO).
 *
 * \note The pins of the specified LEDs are set to GPIO output mode.
 */
#define LED_On(led_gpio)      port_pin_set_output_level(led_gpio, true)

/**
 * \brief Toggles the specified LEDs.
 *
 * \param led_gpio LED to toggle (LEDx_GPIO).
 *
 * \note The pins of the specified LEDs are set to GPIO output mode.
 */
#define LED_Toggle(led_gpio)  port_pin_toggle_output_level(led_gpio)



#endif // USER_BOARD_H

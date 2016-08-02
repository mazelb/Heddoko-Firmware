/*
 * user_board.h
 *
 * Created: 3/18/2016 3:30:34 PM
 *  Author: sean
 */ 


#ifndef USER_BOARD_H_
#define USER_BOARD_H_

/**
 * \defgroup sam4s_xplained_pro_features_group Features
 *
 * Symbols that describe features and capabilities of the board.
 *
 * @{
 */

//! Name string macro
#define BOARD_NAME                "Power Board"

//! \name Resonator definitions
//@{
#define BOARD_FREQ_SLCK_XTAL      (32768U)
#define BOARD_FREQ_SLCK_BYPASS    (32768U)
#define BOARD_FREQ_MAINCK_XTAL    (12000000U)
#define BOARD_FREQ_MAINCK_BYPASS  (12000000U)
#define BOARD_MCK                 CHIP_FREQ_CPU_MAX
#define BOARD_OSC_STARTUP_US      15625
//@}

//! \name LED0 definitions
//@{
#define LED0_PIN                  IOPORT_CREATE_PIN(PIOC, 23)
#define LED0_ACTIVE               false
#define LED0_INACTIVE             !LED0_ACTIVE
#define LED0				      LED0_GPIO
#define LED0_GPIO				  LED0_PIN
//@}

//! \name SW0 definitions
//@{
#define SW0_PIN                   IOPORT_CREATE_PIN(PIOA, 2)
#define SW0_ACTIVE                false
#define SW0_INACTIVE              !SW0_ACTIVE
#define SW0_SUPC_INPUT            2
//@}

/**
 * \name LED #0 definitions
 *
 * Wrapper macros for LED0, to ensure common naming across all Xplained Pro
 * boards.
 */
//@{
#define LED_0_NAME                "LED0 (yellow)"
#define LED_0_PIN                 LED0_PIN
#define LED_0_ACTIVE              LED0_ACTIVE
#define LED_0_INACTIVE            LED0_INACTIVE
//@}

//! Number of on-board LEDs
#define LED_COUNT 1

/**
 * \name Button #0 definitions
 *
 * Wrapper macros for SW0, to ensure common naming across all Xplained Pro
 * boards.
 */
//@{
#define BUTTON_0_NAME             "SW0"
#define BUTTON_0_PIN              SW0_PIN
#define BUTTON_0_ACTIVE           SW0_ACTIVE
#define BUTTON_0_INACTIVE         SW0_INACTIVE
#define BUTTON_0_SUPC_INPUT       SW0_SUPC_INPUT
#define GPIO_PUSH_BUTTON_0        BUTTON_0_PIN 

#define PIN_PUSHBUTTON_0_MASK     PIO_PA2
#define PIN_PUSHBUTTON_0_PIO      PIOA
#define PIN_PUSHBUTTON_0_ID       ID_PIOA
#define PIN_PUSHBUTTON_0_TYPE     PIO_INPUT
#define PIN_PUSHBUTTON_0_ATTR     (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)
//@}

//! Number of on-board buttons
#define BUTTON_COUNT              1


//! \name Extension header #3 pin definitions
//@{
#define EXT3_PIN_3                IOPORT_CREATE_PIN(PIOC, 29)
#define EXT3_PIN_4                IOPORT_CREATE_PIN(PIOC, 30)
#define EXT3_PIN_5                IOPORT_CREATE_PIN(PIOC, 21)
#define EXT3_PIN_6                IOPORT_CREATE_PIN(PIOC, 22)
#define EXT3_PIN_7                IOPORT_CREATE_PIN(PIOC, 20)
#define EXT3_PIN_8                IOPORT_CREATE_PIN(PIOA, 16)
#define EXT3_PIN_9                IOPORT_CREATE_PIN(PIOA, 0)
#define EXT3_PIN_10               IOPORT_CREATE_PIN(PIOC, 31)
#define EXT3_PIN_11               IOPORT_CREATE_PIN(PIOB, 4)
#define EXT3_PIN_12               IOPORT_CREATE_PIN(PIOB, 5)
#define EXT3_PIN_13               IOPORT_CREATE_PIN(PIOB, 2)
#define EXT3_PIN_14               IOPORT_CREATE_PIN(PIOB, 3)
#define EXT3_PIN_15               IOPORT_CREATE_PIN(PIOA, 10)
#define EXT3_PIN_16               IOPORT_CREATE_PIN(PIOA, 13)
#define EXT3_PIN_17               IOPORT_CREATE_PIN(PIOA, 12)
#define EXT3_PIN_18               IOPORT_CREATE_PIN(PIOA, 14)
//@}

#define CONSOLE_UART              UART1
#define CONSOLE_UART_ID           ID_UART1

/** UART1 pins (UTXD1 and URXD1) definitions, PB2,PB3. */
#define PINS_UART1                (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1)
#define PINS_UART1_FLAGS          (PIO_PERIPH_A | PIO_DEFAULT)
#define PINS_UART1_PIO            PIOB

//! \name SPI
//@{
/** SPI MISO pin definition. */
#define SPI_MISO_GPIO         (PIO_PA12_IDX)
#define SPI_MISO_FLAGS       (PIO_PERIPH_A | PIO_PULLUP)
/** SPI MOSI pin definition. */
#define SPI_MOSI_GPIO         (PIO_PA13_IDX)
#define SPI_MOSI_FLAGS       (PIO_PERIPH_A | PIO_PULLUP)
/** SPI SPCK pin definition. */
#define SPI_SPCK_GPIO         (PIO_PA14_IDX)
#define SPI_SPCK_FLAGS       (PIO_PERIPH_A | PIO_PULLUP)

/** SPI chip select 0 pin definition. (Only one configuration is possible) */
#define SPI_NPCS0_GPIO         (PIO_PA11_IDX)
#define SPI_NPCS0_FLAGS           (PIO_PERIPH_A | PIO_DEFAULT)
/** SPI chip select 1 pin definition. (multiple configurations are possible) */
#define SPI_NPCS1_PA9_GPIO     (PIO_PA9_IDX)
#define SPI_NPCS1_PA9_FLAGS       (PIO_PERIPH_B | PIO_DEFAULT)
#define SPI_NPCS1_PA31_GPIO    (PIO_PA31_IDX)
#define SPI_NPCS1_PA31_FLAGS      (PIO_PERIPH_A | PIO_DEFAULT)
#define SPI_NPCS1_PB14_GPIO    (PIO_PB14_IDX)
#define SPI_NPCS1_PB14_FLAGS      (PIO_PERIPH_A | PIO_DEFAULT)
#define SPI_NPCS1_PC4_GPIO     (PIO_PC4_IDX)
#define SPI_NPCS1_PC4_FLAGS       (PIO_PERIPH_B | PIO_DEFAULT)
/** SPI chip select 2 pin definition. (multiple configurations are possible) */
#define SPI_NPCS2_PA10_GPIO    (PIO_PA10_IDX)
#define SPI_NPCS2_PA10_FLAGS      (PIO_PERIPH_B | PIO_PULLUP)
#define SPI_NPCS2_PA30_GPIO    (PIO_PA30_IDX)
#define SPI_NPCS2_PA30_FLAGS      (PIO_PERIPH_B | PIO_DEFAULT)
#define SPI_NPCS2_PB2_GPIO     (PIO_PB2_IDX)
#define SPI_NPCS2_PB2_FLAGS       (PIO_PERIPH_B | PIO_DEFAULT)
/** SPI chip select 3 pin definition. (multiple configurations are possible) */
#define SPI_NPCS3_PA3_GPIO     (PIO_PA3_IDX)
#define SPI_NPCS3_PA3_FLAGS       (PIO_PERIPH_B | PIO_DEFAULT)
#define SPI_NPCS3_PA5_GPIO     (PIO_PA5_IDX)
#define SPI_NPCS3_PA5_FLAGS       (PIO_PERIPH_B | PIO_DEFAULT)
#define SPI_NPCS3_PA22_GPIO    (PIO_PA22_IDX)
#define SPI_NPCS3_PA22_FLAGS      (PIO_PERIPH_B | PIO_DEFAULT)
//@}

//! \name OLED
//@{
#define UG_2832HSWEG04_DATA_CMD_GPIO      (PIO_PC21_IDX)
#define UG_2832HSWEG04_DATA_CMD_FLAGS    (PIO_OUTPUT_1 | PIO_PULLUP)
#define UG_2832HSWEG04_RESET_GPIO      (PIO_PC31_IDX)
#define UG_2832HSWEG04_RESET_FLAGS    (PIO_OUTPUT_1 | PIO_PULLUP)

//#define UG_2832HSWEG04_SPI              &USARTC0
//#define UG_2832HSWEG04_DATA_CMD         IOPORT_CREATE_PIN(PIOC, 0)
//#define UG_2832HSWEG04_RESET            IOPORT_CREATE_PIN(PIOC, 31)
//#define UG_2832HSWEG04_SPI_SCK          IOPORT_CREATE_PIN(PIOA, 14)
//#define UG_2832HSWEG04_SPI_MOSI         IOPORT_CREATE_PIN(PIOA, 13)
#define UG_2832HSWEG04_SS               2
#define UG_2832HSWEG04_BAUDRATE         5000000
//! \name OLED dimensions
//@{
#define LCD_WIDTH_PIXELS                (128)
#define LCD_HEIGHT_PIXELS               (32)
//@}
//@}

//! \name TWI
//@{
/** TWI0 pin definitions */
#define TWI0_DATA_GPIO   PIO_PA3_IDX
#define TWI0_DATA_FLAGS  (PIO_PERIPH_A | PIO_PULLUP)
#define TWI0_CLK_GPIO    PIO_PA4_IDX
#define TWI0_CLK_FLAGS   (PIO_PERIPH_A | PIO_PULLUP)
//@}



//! \name OLED Xplained Pro LED definitions */
//@{
#define IO1_LED1_PIN                  EXT3_PIN_7
#define IO1_LED1_ACTIVE               false
#define IO1_LED1_INACTIVE             !IO1_LED1_ACTIVE

#define IO1_LED2_PIN                  EXT3_PIN_8
#define IO1_LED2_ACTIVE               false
#define IO1_LED2_INACTIVE             !IO1_LED2_ACTIVE

#define IO1_LED3_PIN                  EXT3_PIN_6
#define IO1_LED3_ACTIVE               false
#define IO1_LED3_INACTIVE             !IO1_LED3_ACTIVE
//@}

//! \name OLED Xplained Pro button definitions */
//@{
/** Push button #1 definition. Attributes = pull-up + debounce + interrupt on rising edge. */
#define PUSHBUTTON_1_NAME    "BUTTON 1"
#define GPIO_PUSH_BUTTON_1   (PIO_PA0_IDX)
#define GPIO_PUSH_BUTTON_1_FLAGS    (PIO_INPUT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)

#define PIN_PUSHBUTTON_1    {PIO_PA0, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE}
#define PIN_PUSHBUTTON_1_MASK PIO_PA0
#define PIN_PUSHBUTTON_1_PIO PIOA
#define PIN_PUSHBUTTON_1_ID ID_PIOA
#define PIN_PUSHBUTTON_1_TYPE PIO_INPUT
#define PIN_PUSHBUTTON_1_ATTR (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)

/** Push button #2 definition. Attributes = pull-up + debounce + interrupt on rising edge. */
#define PUSHBUTTON_2_NAME    "BUTTON 2"
#define GPIO_PUSH_BUTTON_2   (PIO_PC29_IDX)
#define GPIO_PUSH_BUTTON_2_FLAGS    (PIO_INPUT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)

#define PIN_PUSHBUTTON_2    {PIO_PC29, PIOC, ID_PIOC, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE}
#define PIN_PUSHBUTTON_2_MASK PIO_PC29
#define PIN_PUSHBUTTON_2_PIO PIOC
#define PIN_PUSHBUTTON_2_ID ID_PIOC
#define PIN_PUSHBUTTON_2_TYPE PIO_INPUT
#define PIN_PUSHBUTTON_2_ATTR (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)

/** Push button #3 definition. Attributes = pull-up + debounce + interrupt on rising edge. */
#define PUSHBUTTON_3_NAME    "BUTTON 3"
#define GPIO_PUSH_BUTTON_3   (PIO_PC30_IDX)
#define GPIO_PUSH_BUTTON_3_FLAGS    (PIO_INPUT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)

#define PIN_PUSHBUTTON_3    {PIO_PC30, PIOC, ID_PIOC, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE}
#define PIN_PUSHBUTTON_3_MASK PIO_PC30
#define PIN_PUSHBUTTON_3_PIO PIOC
#define PIN_PUSHBUTTON_3_ID ID_PIOC
#define PIN_PUSHBUTTON_3_TYPE PIO_INPUT
#define PIN_PUSHBUTTON_3_ATTR (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE)
//@}

//! \name Light Sensor
//@{
#define LIGHT_SENSOR_GPIO   PIO_PB0_IDX
#define LIGHT_SENSOR_FLAGS  PIO_INPUT
//@}

//! \name SD Card
//@{
#define SD_MMC_0_CD_GPIO            (PIO_PC27_IDX)
#define SD_MMC_0_CD_PIO_ID          ID_PIOC
#define SD_MMC_0_CD_FLAGS           (PIO_INPUT | PIO_PULLUP)

#define SD_MMC_0_CD_DETECT_VALUE    0

#define SD_MMC_0_CD    {PIO_PC27, PIOC, ID_PIOC, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE}
#define SD_MMC_0_CD_MASK PIO_PC27
#define SD_MMC_0_CD_PIO PIOC
#define SD_MMC_0_CD_ID ID_PIOC
#define SD_MMC_0_CD_TYPE PIO_INPUT
#define SD_MMC_0_CD_ATTR (PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE)


/*! \name Connections of the AT86RFX transceiver
 */
/** @{ */
#define AT86RFX_SPI                  SPI
#define AT86RFX_RST_PIN              EXT1_PIN_7
#define AT86RFX_IRQ_PIN              EXT1_PIN_9
#define AT86RFX_SLP_PIN              EXT1_PIN_10
#define AT86RFX_SPI_CS               0
#define AT86RFX_SPI_CS_PIN           SPI_NPCS0_GPIO
#define AT86RFX_SPI_CS_FLAGS         SPI_NPCS0_FLAGS
#define AT86RFX_SPI_MOSI             EXT1_PIN_16
#define AT86RFX_SPI_MISO             EXT1_PIN_17
#define AT86RFX_SPI_SCK              EXT1_PIN_18
#define AT86RFX_CSD     		     EXT1_PIN_5
#define AT86RFX_CPS 	             EXT1_PIN_8


void at86rfx_isr(void);

#define AT86RFX_INTC_INIT()         ioport_set_pin_dir(AT86RFX_IRQ_PIN, IOPORT_DIR_INPUT);\
									ioport_set_pin_sense_mode(AT86RFX_IRQ_PIN, IOPORT_SENSE_RISING);\
									pmc_enable_periph_clk(ID_PIOA);\
									pio_set_debounce_filter(PIOA, PIO_PA1, 10);\
									pio_handler_set(PIOA, ID_PIOA, PIO_PA1, PIO_IT_HIGH_LEVEL, at86rfx_isr);\
									NVIC_EnableIRQ((IRQn_Type) ID_PIOA);\
									pio_enable_interrupt(PIOA, PIO_PA1);									

#define AT86RFX_ISR()               void at86rfx_isr(void)

/** Enables the transceiver main interrupt. */
#define ENABLE_TRX_IRQ()            pio_enable_pin_interrupt(AT86RFX_IRQ_PIN)

/** Disables the transceiver main interrupt. */
#define DISABLE_TRX_IRQ()           pio_disable_pin_interrupt(AT86RFX_IRQ_PIN)

/** Clears the transceiver main interrupt. */
#define CLEAR_TRX_IRQ()             NVIC_ClearPendingIRQ(PIOA_IRQn);
/*
 * This macro saves the trx interrupt status and disables the trx interrupt.
 */
#define ENTER_TRX_REGION()          pio_disable_pin_interrupt(AT86RFX_IRQ_PIN);

/*
 *  This macro restores the transceiver interrupt status
 */
#define LEAVE_TRX_REGION()         pio_enable_pin_interrupt(AT86RFX_IRQ_PIN)





#endif /* USER_BOARD_H_ */
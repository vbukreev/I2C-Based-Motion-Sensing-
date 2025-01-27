 /*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017,2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*  SDK Included Files */
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"

#include "fsl_gpio.h"
#include "fsl_port.h"
#include "MK66F18.h" // Replace with your board-specific header


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ACCEL_I2C_CLK_SRC     I2C0_CLK_SRC
#define ACCEL_I2C_CLK_FREQ    CLOCK_GetFreq(I2C0_CLK_SRC)
#define I2C_BAUDRATE          100000
#define I2C_RELEASE_SDA_PORT  PORTD
#define I2C_RELEASE_SCL_PORT  PORTD
#define I2C_RELEASE_SDA_GPIO  GPIOD
#define I2C_RELEASE_SDA_PIN   9U
#define I2C_RELEASE_SCL_GPIO  GPIOD
#define I2C_RELEASE_SCL_PIN   8U
#define I2C_RELEASE_BUS_COUNT 100U
/* MMA8491 does not have WHO_AM_I, DATA_CFG amd CTRL registers */
#if !(defined(I2C_ACCEL_MMA8491) && I2C_ACCEL_MMA8491)
#define FXOS8700_WHOAMI    0xC7U
#define MMA8451_WHOAMI     0x1AU
#define MMA8652_WHOAMI     0x4AU
#define ACCEL_XYZ_DATA_CFG 0x0EU
#define ACCEL_CTRL_REG1    0x2AU
/* FXOS8700 and MMA8451 have the same who_am_i register address. */
#define ACCEL_WHOAMI_REG 0x0DU
#endif
#define ACCEL_STATUS     0x00U
#define ACCEL_READ_TIMES 10U

#define LED_RED_PORT   PORTC
#define LED_RED_GPIO   GPIOC
#define LED_RED_PIN    9U

#define LED_GREEN_PORT PORTE
#define LED_GREEN_GPIO GPIOE
#define LED_GREEN_PIN  6U

#define LED_BLUE_PORT  PORTA
#define LED_BLUE_GPIO  GPIOA
#define LED_BLUE_PIN   11U
#define THRESHOLD 10
#define Z_AXIS_THRESHOLD 300

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_I2C_ReleaseBus(void);
#if !(defined(I2C_ACCEL_MMA8491) && I2C_ACCEL_MMA8491)
static bool I2C_ReadAccelWhoAmI(void);
static bool I2C_WriteAccelReg(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value);
#endif
static bool I2C_ReadAccelRegs(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize);
extern void ConfigureLEDs(void);
extern void ToggleLED(void);  // ToggleLED function from the assembly file
extern void SetLEDsHigh(void); // SetLEDsHigh function from the assembly file
void GPIOC_IRQHandler(void);
void DisableGPIOInterrupt(void);

extern void ConfigureRedLED(void);   // Configure Port C, Pin 9 as output for Red LED
extern void TurnOnRedLED(void);      // Turn on Red LED (active low)
extern void TurnOffRedLED(void);     // Turn off Red LED
extern void TurnOffBlueLED(void);    // Turn off Blue LED
/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t MSBshift = 8U;
uint8_t LSBshift = 2U;
#if defined(I2C_ACCEL_MMA8491) && I2C_ACCEL_MMA8491
uint8_t g_mma8491_addr = 0x55U;
#else
/* FXOS8700, MMA8652 and MMA8451 device address */
const uint8_t g_accel_address[] = {0x1CU, 0x1DU, 0x1EU, 0x1FU};
#endif
uint8_t g_accel_addr_found = 0x00;
i2c_master_handle_t g_m_handle;

volatile bool completionFlag = false;
volatile bool nakFlag        = false;

int16_t initialX, initialY, initialZ;
volatile int16_t currentX, currentY, currentZ;
volatile bool positionChanged = false;

/*******************************************************************************
 * Code
 ******************************************************************************/



static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++)
    {
        __NOP();
    }
}

void BOARD_I2C_ReleaseBus(void)
{
    uint8_t i = 0;
    gpio_pin_config_t pin_config;
    port_pin_config_t i2c_pin_config = {0};

    /* Config pin mux as gpio */
    i2c_pin_config.pullSelect = kPORT_PullUp;
    i2c_pin_config.mux        = kPORT_MuxAsGpio;

    pin_config.pinDirection = kGPIO_DigitalOutput;
    pin_config.outputLogic  = 1U;
    CLOCK_EnableClock(kCLOCK_PortD);
    PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SCL_PIN, &i2c_pin_config);
    PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SDA_PIN, &i2c_pin_config);

    GPIO_PinInit(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, &pin_config);
    GPIO_PinInit(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, &pin_config);

    /* Drive SDA low first to simulate a start */
    GPIO_PinWrite(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL and keep SDA high */
    for (i = 0; i < 9; i++)
    {
        GPIO_PinWrite(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_PinWrite(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_PinWrite(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_PinWrite(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_PinWrite(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
    i2c_release_bus_delay();
}

static void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
        completionFlag = true;
    }
    /* Signal transfer success when received success status. */
    if ((status == kStatus_I2C_Nak) || (status == kStatus_I2C_Addr_Nak))
    {
        nakFlag = true;
    }
}

static void I2C_InitModule(void)
{
    i2c_master_config_t masterConfig;
    uint32_t sourceClock = 0;

    /*
     * masterConfig.baudRate_Bps = 100000U;
     * masterConfig.enableStopHold = false;
     * masterConfig.glitchFilterWidth = 0U;
     * masterConfig.enableMaster = true;
     */
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUDRATE;
    sourceClock               = ACCEL_I2C_CLK_FREQ;
    I2C_MasterInit(BOARD_ACCEL_I2C_BASEADDR, &masterConfig, sourceClock);
}

#if !(defined(I2C_ACCEL_MMA8491) && I2C_ACCEL_MMA8491)
static bool I2C_ReadAccelWhoAmI(void)
{
    /*
    How to read the device who_am_I value ?
    Start + Device_address_Write , who_am_I_register;
    Repeart_Start + Device_address_Read , who_am_I_value.
    */
    uint8_t who_am_i_reg          = ACCEL_WHOAMI_REG;
    uint8_t who_am_i_value        = 0x00;
    uint8_t accel_addr_array_size = 0x00;
    bool find_device              = false;
    uint8_t i                     = 0;

    I2C_InitModule();

    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress   = g_accel_address[0];
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data           = &who_am_i_reg;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferNoStopFlag;

    accel_addr_array_size = sizeof(g_accel_address) / sizeof(g_accel_address[0]);

    for (i = 0; i < accel_addr_array_size; i++)
    {
        masterXfer.slaveAddress = g_accel_address[i];

        I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

        /*  wait for transfer completed. */
        while ((!nakFlag) && (!completionFlag))
        {
        }

        nakFlag = false;

        if (completionFlag == true)
        {
            completionFlag     = false;
            find_device        = true;
            g_accel_addr_found = masterXfer.slaveAddress;
            break;
        }
    }

    if (find_device == true)
    {
        masterXfer.direction      = kI2C_Read;
        masterXfer.subaddress     = 0;
        masterXfer.subaddressSize = 0;
        masterXfer.data           = &who_am_i_value;
        masterXfer.dataSize       = 1;
        masterXfer.flags          = kI2C_TransferRepeatedStartFlag;

        I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

        /*  wait for transfer completed. */
        while ((!nakFlag) && (!completionFlag))
        {
        }

        nakFlag = false;

        if (completionFlag == true)
        {
            completionFlag = false;
            if (who_am_i_value == FXOS8700_WHOAMI)
            {
                PRINTF("Found an FXOS8700 on board , the device address is 0x%x . \r\n", masterXfer.slaveAddress);
                return true;
            }
            else if (who_am_i_value == MMA8451_WHOAMI)
            {
                PRINTF("Found an MMA8451 on board , the device address is 0x%x . \r\n", masterXfer.slaveAddress);
                return true;
            }
            else if (who_am_i_value == MMA8652_WHOAMI)
            {
                LSBshift = 4U;
                PRINTF("Found an MMA8652 on board , the device address is 0x%x . \r\n", masterXfer.slaveAddress);
                return true;
            }
            else
            {
                PRINTF("Found a device, the WhoAmI value is 0x%x\r\n", who_am_i_value);
                PRINTF("It's not MMA8451 or FXOS8700 or MMA8652. \r\n");
                PRINTF("The device address is 0x%x. \r\n", masterXfer.slaveAddress);
                return false;
            }
        }
        else
        {
            PRINTF("Not a successful i2c communication \r\n");
            return false;
        }
    }
    else
    {
        PRINTF("\r\n Do not find an accelerometer device ! \r\n");
        return false;
    }
}

static bool I2C_WriteAccelReg(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress   = device_addr;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = &value;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;

    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}
#endif

static bool I2C_ReadAccelRegs(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress   = device_addr;
    masterXfer.direction      = kI2C_Read;
    masterXfer.subaddress     = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = rxBuff;
    masterXfer.dataSize       = rxSize;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;

    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}


void GPIOD_IRQHandler(void)
{
    // Check if the interrupt was triggered for the specific pin (e.g., pin 9)
    if (PORT_GetPinsInterruptFlags(PORTD) & (1U << 9))
    {
        // Clear the interrupt flag for pin 9
        PORT_ClearPinsInterruptFlags(PORTD, (1U << 9));

        // Turn on the Red LED (assuming active low)


        // Simple delay for visibility (optional)
        for (volatile int i = 0; i < 1000000; ++i)
        {
            __NOP(); // No operation - just a delay
        }

        // Turn off the Red LED


        PRINTF("Interrupt triggered: Red LED turned ON due to Z-axis change.\n");
    }
}

void ConfigureZAxisInterrupt(void)
{
    gpio_pin_config_t gpioConfig = {kGPIO_DigitalInput, 0};
    port_pin_config_t portConfig = {.pullSelect = kPORT_PullUp, .mux = kPORT_MuxAsGpio};

    // Enable clock for Port D
    CLOCK_EnableClock(kCLOCK_PortD);

    // Configure the pin as a digital input
    PORT_SetPinConfig(PORTD, 9U, &portConfig); // Assuming pin 9 on GPIOD for interrupt
    GPIO_PinInit(GPIOD, 9U, &gpioConfig);

    // Set pin interrupt configuration (e.g., falling edge)
    PORT_SetPinInterruptConfig(PORTD, 9U, kPORT_InterruptFallingEdge);

    // Enable the interrupt in the NVIC
    EnableIRQ(PORTD_IRQn);
}

void RecordInitialPosition(int16_t x, int16_t y, int16_t z)
{
    initialX = x;
    initialY = y;
    initialZ = z;
}

//void TurnOffLEDs(void)
//{
//    GPIO_PinWrite(LED_RED_GPIO, LED_RED_PIN, 0);
//    GPIO_PinWrite(LED_GREEN_GPIO, LED_GREEN_PIN, 0);
//    GPIO_PinWrite(LED_BLUE_GPIO, LED_BLUE_PIN, 0);
//}

void DetectPositionChange(int16_t x, int16_t y, int16_t z, bool *positionChanged, bool *positionZChanged)
{
    int16_t diffX = abs(x - initialX);
    int16_t diffY = abs(y - initialY);
    int16_t diffZ = abs(z - initialZ);

    *positionChanged = (diffX > THRESHOLD || diffY > THRESHOLD);
    *positionZChanged = (diffZ > Z_AXIS_THRESHOLD);

    //PRINTF("Z-axis change detected: diffZ = %d\n", diffZ);
}




void UpdateLEDs(bool positionChanged, bool positionZChanged)
{
    static bool interruptConfigured = false;

    if (positionZChanged)
    {
        if (!interruptConfigured)
        {
        	TurnOffBlueLED();
        	ConfigureRedLED();
        	TurnOnRedLED();
        	ConfigureZAxisInterrupt();
            interruptConfigured = true;
        }

        // The Red LED flashing is now handled in the interrupt handler
        PRINTF("Z-axis change detected. Interrupt configured.\n");
    }
    else if (positionChanged)
    {
        ToggleLED();  // Toggle Green and Blue LEDs using assembly function
    }
    else
    {
        SetLEDsHigh();  // Turn off Green and Blue LEDs using assembly function
    }
}




//void ToggleLED(GPIO_Type *gpio, uint32_t pin)
//{
//    uint32_t state = GPIO_PinRead(gpio, pin);
//    GPIO_PinWrite(gpio, pin, !state); // Toggle LED state
//}
/*!
 * @brief Main function
 */
int main(void)
{
    bool isThereAccel = false;
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_I2C_ReleaseBus();
    BOARD_I2C_ConfigurePins();
    BOARD_InitDebugConsole();

    ConfigureLEDs();  // Initialize LED pins




    PRINTF("\r\nMotion Sensor ON\r\n");

    I2C_MasterTransferCreateHandle(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
#if defined(I2C_ACCEL_MMA8491) && I2C_ACCEL_MMA8491
    uint8_t status0_value                = 0;
    gpio_pin_config_t tilt_enable_config = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0U};

    /* Enable sensor */
    GPIO_PinInit(BOARD_TILT_ENABLE_GPIO, BOARD_TILT_ENABLE_GPIO_PIN, &tilt_enable_config);

    I2C_InitModule();

    /* If using MMA8491, then perform get status operation to g_mma8491_addr to check whether it is welded on board. */
    isThereAccel = I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_mma8491_addr, ACCEL_STATUS, &status0_value, 1);
    if (isThereAccel)
    {
        PRINTF("Found MMA8491 on board, the device address is 0x%x. \r\n", g_mma8491_addr);
        g_accel_addr_found = g_mma8491_addr;
    }
    else
    {
        PRINTF("\r\nDo not find an accelerometer device ! \r\n");
    }
#else
    /* For other sensors, check the type of the sensor */
    isThereAccel = I2C_ReadAccelWhoAmI();
#endif

    /* Read the accel XYZ value if there is accel device on board */
    if (true == isThereAccel)
    {
        uint8_t readBuff[7];
        int16_t x, y, z;

        /* Initialize the accelerometer */
        uint8_t databyte  = 0;
        uint8_t write_reg = 0;

        write_reg = ACCEL_CTRL_REG1;
        databyte  = 0;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);

        write_reg = ACCEL_XYZ_DATA_CFG;
        databyte  = 0x01;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);

        write_reg = ACCEL_CTRL_REG1;
        databyte  = 0x0D;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);

        /* Record the initial position */
        I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, ACCEL_STATUS, readBuff, 7);
        initialX = ((int16_t)(((readBuff[1] << MSBshift) | readBuff[2]))) >> LSBshift;
        initialY = ((int16_t)(((readBuff[3] << MSBshift) | readBuff[4]))) >> LSBshift;
        initialZ = ((int16_t)(((readBuff[5] << MSBshift) | readBuff[6]))) >> LSBshift;

        PRINTF("Initial Position - x: %5d, y: %5d, z: %5d\r\n", initialX, initialY, initialZ);

        /* Continuous monitoring */
        while (1)
                {
                    uint8_t status0_value = 0;
                    while (status0_value != 0xFFU)
                    {
                        I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, ACCEL_STATUS, &status0_value, 1);
                    }

                    I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, ACCEL_STATUS, readBuff, 7);
                    currentX = ((int16_t)(((readBuff[1] << MSBshift) | readBuff[2]))) >> LSBshift;
                    currentY = ((int16_t)(((readBuff[3] << MSBshift) | readBuff[4]))) >> LSBshift;
                    currentZ = ((int16_t)(((readBuff[5] << MSBshift) | readBuff[6]))) >> LSBshift;

                    PRINTF("Current Position - x: %5d, y: %5d, z: %5d\r\n", currentX, currentY, currentZ);

                    bool positionChanged = false;
                    bool positionZChanged = false;

                                // Detect changes in position
                                DetectPositionChange(currentX, currentY, currentZ, &positionChanged, &positionZChanged);

                                // Calculate maxDifference
                                int16_t diffX = abs(currentX - initialX);
                                int16_t diffY = abs(currentY - initialY);
                                int16_t diffZ = abs(currentZ - initialZ);
                                int16_t maxDifference = (diffX > diffY) ? ((diffX > diffZ) ? diffX : diffZ) : ((diffY > diffZ) ? diffY : diffZ);

                                // Update the LEDs based on position changes
                                UpdateLEDs(positionChanged, positionZChanged);
                                SDK_DelayAtLeastUs(100000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
                }
    }

    PRINTF("\r\nEnd of Motion Sensor example.\r\n");
    while (1)
    {
    }
}

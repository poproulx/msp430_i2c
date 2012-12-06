/* Include files */
#include <msp430x20x3.h>
#include <legacymsp430.h>
#include "i2c.h"

/* Constants */
#define RTC_FREQ 100
#define RTC_FREQ_X2 12000
#define RTC_FREQ_X3 18000

#define NBR_BYTES     2
#define I2C_ADDRESS   0x74

#define CMD_OUTPUT_PORT_0 0x02
#define CMD_OUTPUT_PORT_1 0x03
#define CMD_CONFIG_PORT_0 0x06
#define CMD_CONFIG_PORT_1 0x07

#define EIGHT_SEG_1 ~0x14
#define EIGHT_SEG_2 ~0xCD
#define EIGHT_SEG_3 ~0x9D
#define EIGHT_SEG_4 ~0x1E
#define EIGHT_SEG_5 ~0x9B
#define EIGHT_SEG_6 ~0xDB
#define EIGHT_SEG_7 ~0x17
#define EIGHT_SEG_8 ~0xDF
#define EIGHT_SEG_9 ~0x1F
#define EIGHT_SEG_0 ~0xD7

/* static prototypes */
static void initializeTimerA(void);

/* static variables */
static volatile int timerFlag;

/* Initialize the watchdog timer, the basic clock module,
 * the timer A operation, the output pin and the interrupt
 */
static void initializeTimerA(void)
{
  /* Basic clock module+ initialization
     
     BCSCTL1 : 0x87 default. XT2OFF and DCO mid range
     BCSCTL2 : Master clock sourced by DCOCLK
               Master clock not divided
               Sub-main clock sourced by LFXT1CLK (32kHz crystal)
               Sub-main clock not divided
     BCSDTL3 : 6pF capacitor
               Low frequency clock set to VLOCLK
   */
  BCSCTL2  = SELM_0 + SELS;
  BCSCTL3  = XCAP_1;
 
  /* Timer A initialization
     
     TACTL  : Sub-main clock (SMCLK)
              Input divider is 1
              The timer is cleared
     TACCTL : The interrupt is enabled
     TACCR  : 11999 for an interrupt every second
   */
  CCR0 = RTC_FREQ - 1;                           // Set the timer limit
  TACTL = TASSEL_2 + MC_1;
  CCTL0 = CCIE;                           // Enable the timer a0 interrupt

  /* Output pin direction and state  */
  P1DIR |= 0x01;                            // Set P1.0 to output direction
  P1OUT = 0x00;

  /* General interrupt enable */
  //_BIS_SR(LPM2_bits + GIE);
}

void turnOffAllDisplays(void)
{
    /* Supply none of 8 segment displays */ 
    unsigned int data[2] = {CMD_OUTPUT_PORT_1, 0x00};
    i2cWriteData(I2C_ADDRESS, data, 2);
}

void turnOnDisplay(unsigned int displayNbr)
{
    /* Supply none of 8 segment displays */ 
    unsigned int data[2] = {CMD_OUTPUT_PORT_1, displayNbr};
    i2cWriteData(I2C_ADDRESS, data, 2);
}

void displayDigit(unsigned int digit)
{
    /* Supply none of 8 segment displays */ 
    unsigned int data[2] = {CMD_OUTPUT_PORT_0, digit};
    i2cWriteData(I2C_ADDRESS, data, 2);

}

int main( void )
{
    unsigned int data[2] = {CMD_CONFIG_PORT_0, ~0xFF};

    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    initializeTimerA();

    /*********************************/
    /* Configure the USI in I2C mode */
    /*********************************/
    P1OUT  = 0xC0;                        // P1.6 & P1.7 Pullups, others to 0
    P1REN |= 0xC0;                       // P1.6 & P1.7 Pullups
    P1DIR  = 0xFF;                        // Unused pins as outputs
    P2OUT  = 0x00;
    P2DIR  = 0xFF;

    /********************************************/
    /* Pour conprendre les commentaires voir la */
    /* Figure 10-2. USI Block Diagram: I2C Mode */
    /* du manuel User's Guide (slau144)         */
    /********************************************/

    /* USI Control Register 1
       USIPE6: SCL enabled
       USIPE7: SDA enabled
       USIMST: Master mode enabled
       USISWRST: The USI is in reset state. USI interrupt flag inactive. */
    USICTL0 = USIPE6+USIPE7+USIMST+USISWRST;

    /* USI Control Register 2
       USII2C: USI in I2C mode
       USIIE: USI counter interrupt enabled */
    USICTL1 = USII2C+USIIE;

    /* USI Clock Control Register
       SCL = SMCLK/8 (~125kHz) */
    USICKCTL = USIDIV_0+USISSEL_2+USICKPL;

    /* Interrupt USIIFG is not cleared automatically when the counter
       reaches 0 */
    USICNT |= USIIFGCC;

    /* Enable the USI. Clear the software reset. */
    USICTL0 &= ~USISWRST;

    /* Clear the USI interrupt flag */
    USICTL1 &= ~USIIFG;

    /* Enable the interrupts */
    __enable_interrupt();

    /* Configure port 0 */
    i2cWriteData(I2C_ADDRESS, data, 2);
    
    //for (i = 0; i < 62500; i++);

    /* Configure port 1 */
    data[0] = CMD_CONFIG_PORT_1;
    data[1] = ~0x0F;
    i2cWriteData(I2C_ADDRESS, data, 2);
   
    //for (i = 0; i < 62500; i++);
    
    /* Supply none of 8 segment displays */ 
    //data[0] = CMD_OUTPUT_PORT_1;     // set the pca9539 command
    //data[1] = 0x00;              // reset all port 1 outputs
    //i2cWriteData(I2C_ADDRESS, data, 2);
    
    turnOffAllDisplays();

    
    /* Supply all leds */ 
    //data[0] = CMD_OUTPUT_PORT_0;     // set the pca9539 command
    //data[1] = EIGHT_SEG_1;              // reset all port 1 outputs
    //i2cWriteData(I2C_ADDRESS, data, 2);
    
    displayDigit(EIGHT_SEG_1);
    char next = 0;
    while(1)
    {

        //for (i = 0; i < 62500; i++);        // Dummy delay between communication cycles
        _BIS_SR(LPM0_bits + GIE);
        
        if (timerFlag == 1)
        {
            switch(next)
            {
                case 0:
                    turnOffAllDisplays();
                    displayDigit(EIGHT_SEG_2);
                    turnOnDisplay(0x02);
                    next = 1;
                    break;
                case 1:
                    turnOffAllDisplays();
                    displayDigit(EIGHT_SEG_3);
                    turnOnDisplay(0x04);
                    next = 2;
                    break;
                case 2:
                    turnOffAllDisplays();
                    displayDigit(EIGHT_SEG_4);
                    turnOnDisplay(0x08);
                    next = 3;
                    break;
                case 3:
                    turnOffAllDisplays();
                    displayDigit(EIGHT_SEG_1);
                    turnOnDisplay(0x01);
                    next = 0;
                    break;
            }

            i2cWriteData(I2C_ADDRESS, data, 2);
        }

    }
}

interrupt (TIMERA0_VECTOR) wakeup IntServiceRoutine1(void)
{
    timerFlag = 1;
    
    /* remplace le mot wakeup. particularite de mspgcc */
    //_BIC_SR_IRQ(LPM0_bits);
}

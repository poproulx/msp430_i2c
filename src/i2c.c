#include <msp430x20x3.h>
#include <legacymsp430.h>
#include "i2c.h"

static int i2cState = 0;                     // State variable
static int i2cAddress = 0xE8;                  // Address is 0xE0 | 0x08
static unsigned int const *i2cData;
static unsigned int byteCounter = 0;
static unsigned char i2cNbrBytes = 0;

void i2cWriteData (unsigned int const address,
              unsigned int const *data,
              unsigned int const nbrBytes)
{
    i2cAddress = address << 1;
    i2cData = data;
    i2cNbrBytes = nbrBytes;

    USICTL1 |= USIIFG;                 // Set flag and start communication
    LPM0;
    __no_operation();

    byteCounter = 0;
    i2cNbrBytes = 0;

}

interrupt (USI_VECTOR) IntServiceRoutine(void)
{
  switch(i2cState)
    {
      case 0: // Generate Start Condition & send address to slave
              P1OUT |= 0x01;           // LED on: sequence start

              /* Empty the shify register */
              USISRL = 0x00;           // Generate Start Condition...

              /* Operation on SDA gated D-Latch:
                 The latch is enabled.
                 The D input is low.
                 Therefore the Q output is latched low and the line SDA is
                 pulled low.

                 USIIFG is set which enables the block "SCL hold". The pullup
                 resistors keeps the line high.

                 This constitutes a low to high transition, hence the
                 start signal. */
              USICTL0 |= USIGE+USIOE;

              /* Operation on SDA gated D-Latch.
                 The latch is not forced transparent anymore. It now depends
                 on the "shift clock"*/
              USICTL0 &= ~USIGE;

              /* Load the shift register with the slave address. */
              USISRL = i2cAddress;       // ... and transmit address, R/W = 0

              /* Load the counter with the number of bits to transmit. Wait
                 for the counter interrupt flag to be cleared before outputting
                 data. */
              USICNT = (USICNT & 0xE0) + 0x08; // Bit counter = 8, TX Address
              i2cState = 2;           // Go to next state: receive address (N)Ack
              break;

      case 2: // Receive Address Ack/Nack bit
              USICTL0 &= ~USIOE;       // SDA = input
              USICNT |= 0x01;          // Bit counter = 1, receive (N)Ack bit
              i2cState = 4;           // Go to next state: check (N)Ack
              break;

      case 4: // Process Address Ack/Nack & handle data TX
              USICTL0 |= USIOE;        // SDA = output
              if (USISRL & 0x01)       // If Nack received...
              {
                /* Prepare the ISR to send a STOP bit. A STOP occurs when SDA
                   switches from low to high while SCL is high. When the
                   counter flag will be cleared, the SDA line will go low*/
                USISRL = 0x00;
                USICNT |=  0x01;       // Bit counter = 1, SCL high, SDA low
                i2cState = 10;        // Go to next state: generate Stop
                P1OUT |= 0x01;         // Turn on LED: error
              }
              else
              { // Ack received, TX data to slave...
                USISRL = *(i2cData++);     // Load the first data byte
                byteCounter++;
                USICNT |=  0x08;       // Bit counter = 8, start TX
                i2cState = 6;         // Go to next state: receive data (N)Ack
                P1OUT &= ~0x01;        // Turn off LED
              }
              break;

      case 6: // Receive Data Ack/Nack bit
              USICTL0 &= ~USIOE;       // SDA = input
              USICNT |= 0x01;          // Bit counter = 1, receive (N)Ack bit
              /* ACK = LOW, NACK = HIGH */
              i2cState = 8;           // Go to next state: check (N)Ack
              break;

      case 8: // Process Data Ack/Nack & send Stop
              USICTL0 |= USIOE;
              if (USISRL & 0x01)       // If Nack received...
                P1OUT |= 0x01;         // Turn on LED: error
              else                     // Ack received
              {
                P1OUT &= ~0x01;        // Turn off LED

                /* Send the remaining other data bytes */
                if (byteCounter < i2cNbrBytes)
                {
                    USISRL = *(i2cData++);     // Load data byte
                    byteCounter++;
                    USICNT |=  0x08;       // Bit counter = 8, start TX
                    i2cState = 6;         // Go to next state: receive data (N)Ack
                    P1OUT &= ~0x01;        // Turn off LED
                }
                else
                {
                    // Send stop...
                    USISRL = 0x00;
                    USICNT |=  0x01;         // Bit counter = 1, SCL high, SDA low
                    i2cState = 10;          // Go to next state: generate Stop
                }
              }

              break;

      case 10:// Generate Stop Condition
              USISRL = 0x0FF;          // USISRL = 1 to release SDA
              USICTL0 |= USIGE;        // Transparent latch enabled
              USICTL0 &= ~(USIGE+USIOE);// Latch/SDA output disabled
              i2cState = 0;           // Reset state machine for next transmission
              LPM0_EXIT;               // Exit active for next transfer
              break;
    }

  /* When the counter interrupt flag is set, the SCL is held inactive. It
     means SCL is high because of the pullup resistor.*/
  USICTL1 &= ~USIIFG;                  // Clear pending flag
}

/* usb_serial app:
 *
 * == Pinout ==
 *
 * P1_0 = nDTR: general purpose output pin controlled by computer
 * P1_1 = nRTS: general purpose output pin controlled by computer
 * P1_2 = nDSR: general purpose input pin reported to computer
 * P1_3 = nCD: general purpose input pin reported to computer
 * (P1_4: Reserved for CT flow control line in future version.)
 * (P1_5: Reserved for RT flow control line in future version.)
 * P1_6 = TX:  transmits data from computer
 * P1_7 = RX:  receives data and sends it to the computer
 *
 * == Overview ==
 *
 * == Technical Description ==
 *
 * == Parameters ==
 *
 * == Example Uses ==
 */

/*
 * TODO: Support for USB CDC ACM control signals.
 * TODO: use LEDs to give feedback about USB activity.
 * TODO: UART flow control
 * TODO: give feedback about framing, parity, overrun errors to user and also to USB host
 * TODO: allow any control signal (or inverted control signal) to be mapped
 *       to any Wixel pin
 * TODO: add parameter for placing TX and RX in any of the 4 available locations
 * TODO: add a parameter for doing inverted TTL serial on RX and TX.
 * TODO: support the SendBreak request?
 * TODO: look at other usb-to-serial converters to see what their default values
 *       are for DTR and RTS (the current defaults set by usb_cdc_acm.lib are RTS=1,DTR=0)
 */

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>

#include <uart1.h>

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(usbComControlLineState & ACM_CONTROL_LINE_DTR);
    LED_RED(0);
}

void usbToUartService()
{
    // Data
    while(usbComRxAvailable() && uart1TxAvailable())
    {
        uart1TxSendByte(usbComRxReceiveByte());
    }

    while(uart1RxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(uart1RxReceiveByte());
    }

    // Control lines controlled by computer.
    P1_0 = !(usbComControlLineState & ACM_CONTROL_LINE_DTR);
    P1_1 = !(usbComControlLineState & ACM_CONTROL_LINE_RTS);
    P1DIR |= (1<<0) | (1<<1);

    // Control lines controlled by device.

    usbComSerialState = (usbComSerialState & ~(ACM_SERIAL_STATE_RX_CARRIER | ACM_SERIAL_STATE_TX_CARRIER));
    if (!P1_2){ usbComSerialState |= ACM_SERIAL_STATE_TX_CARRIER; } // TX Carrier = DSR
    if (!P1_3){ usbComSerialState |= ACM_SERIAL_STATE_RX_CARRIER; } // RX Carrier = CD
}

void lineCodingChanged()
{
    uart1SetBaudRate(usbComLineCoding.dwDTERate);
    uart1SetParity(usbComLineCoding.bParityType);
    uart1SetStopBits(usbComLineCoding.bCharFormat);
}

void main()
{
    systemInit();
    usbInit();
    usbComLineCodingChangeHandler = &lineCodingChanged;

    uart1Init();
    lineCodingChanged();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        usbToUartService();
    }
}

/*******************************************************************************
 * This file is part of OpenWSN, the Open Wireless Sensor Network Platform.
 *
 * Copyright (C) 2005-2020 zhangwei(TongJi University)
 *
 * OpenWSN is a free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 or (at your option) any later version.
 *
 * OpenWSN is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * For non-opensource or commercial applications, please choose commercial license.
 * Refer to OpenWSN site http://code.google.com/p/openwsn/ for more detail.
 *
 * For other questions, you can contact the author through email openwsn#gmail.com
 * or the mailing address: Dr. Wei Zhang, Dept. of Control, Dianxin Hall, TongJi
 * University, 4800 Caoan Road, Shanghai, China. Zip: 201804
 *
 ******************************************************************************/

/* @modified by zhangwei on 2013.11.28
 * - Revised. Tested Ok with the USART id 0. This USART is the one nearby the JTAG
 *   interface in the opennode base board. 
 */
#include "apl_foundation.h"

/**
 * @attention
 * - You should attention the which UART you use by default. The node hardware (mainly
 *   the MCU) should provide at least 1 U(S)ART. A lot of example and testing programs
 *   needs this UART for I/O. 
 * 
 *   valid UART_ID = 0|1
 */
#ifndef CONFIG_SIO_UART_ID
#define UART_ID 0
#else
#define UART_ID CONFIG_SIO_UART_ID
#endif

TiUartAdapter m_uart;

static void uartecho1(void);
static void uartecho2(void);
static void uart_active_send1(void);
static void uart_active_send2(void);

int main(void)
{
    uartecho1();
    //uartecho2();
    //uart_active_send();
    return 0;
}

void uartecho1()
{
    TiUartAdapter * uart;
    char ch;
    intx count;
	static int seq_id=1;
	char * msg = "welcoome to uartecho example...";

    target_init();
    led_open(LED_ALL);
    led_on( LED_ALL);
    hal_delayms( 500 );
    led_off( LED_ALL );

	//hal_debug_init(UART_ID, 9600);
	//dbc_write(msg, strlen(msg));

    //halUartInit(9600,0);
    uart = uart_construct((void *)(&m_uart), sizeof(m_uart));
    uart = uart_open(uart, UART_ID, 9600, 8, 1, 0);

    while (1)
    {
        count = uart_getchar(uart, &ch);
        if (count > 0)
        {
            while (1)
            {
                count = uart_putchar(uart, ch+1);
                if (count > 0)
                    break;
            }
            led_toggle(LED_RED);
        }
    }
}

void uartecho2()
{
    TiUartAdapter * uart;
    char buf[40];
    intx len, count;

    count = 0;

    //target_init();
    led_open(LED_ALL);
    led_on( LED_ALL);
    hal_delayms( 500 );
    led_off( LED_ALL );

    uart = uart_construct((void *)(&m_uart), sizeof(m_uart));
    uart = uart_open(uart, UART_ID, 9600, 8, 1, 0);

    while (1)
    {
        len = uart_read(uart, buf, sizeof(buf), 0);
        if (len > 0)
        {
            count = 0;
            while (count < len)
            {
                count += uart_write(uart, buf, len, 0);
            }
            
            led_toggle(LED_RED);
        }
    }
}

void uart_active_send1()
{
    TiUartAdapter * uart;
    uint8 ch;
    intx count;

    count = 0;

    target_init();
    led_open(LED_ALL);
    led_on( LED_ALL);
    hal_delayms( 500 );
    led_off( LED_ALL );

    uart = uart_construct((void *)(&m_uart), sizeof(m_uart));
    uart = uart_open(uart, UART_ID, 9600, 8, 1, 0);

    while (1)
    {
        ch = count++;
        uart_putchar(uart, ch);
        hal_delayms(500);
        led_toggle(LED_RED);
    }
}

void uart_active_send2()
{
    TiUartAdapter * uart;
    uint8 ch;
    intx count, i;

    count = 0;

    target_init();
    led_open(LED_ALL);
    led_on( LED_ALL);
    hal_delayms( 500 );
    led_off( LED_ALL );

    uart = uart_construct((void *)(&m_uart), sizeof(m_uart));
    uart = uart_open(uart, UART_ID, 9600, 8, 1, 0);

    while (1)
    {
        ch = count++;
        for (i=0; i<20; i++)
        {
            uart_write(uart, &ch, 1, 0x00);
        }
        hal_delayms(500);
        led_toggle(LED_RED);
    }
}

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
 
/**
 * @modified by zhangwei on 2011.09.12
 * - Revised
 */ 

/*
 * @todo目前用的都是查询方式，中断方式还没有解决
 */

#include "../hal_configall.h"
#include <stdlib.h>
#include <string.h>
#include <iocc2430.h>
#include "../hal_foundation.h"
#include "../hal_device.h"
#include "../hal_targetboard.h"
#include "../hal_led.h"
#include "../hal_assert.h"
#include "../hal_cpu.h"
#include "../hal_mcu.h"
#include "../hal_uart.h"
#include "hal_cc2430_foundation.h"

#define UART_ENABLE_RECEIVE 0x40

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void _uart_rx0_interrupt_handler( void * uartptr, TiEvent * e );
void _uart_tx0_interrupt_handler( void * uartptr, TiEvent * e );
void _uart_rx1_interrupt_handler( void * uartptr, TiEvent * e );
void _uart_tx1_interrupt_handler( void * uartptr, TiEvent * e );
#endif

TiUartAdapter * uart_construct( char * buf, uint16 size )
{
    hal_assert( sizeof(TiUartAdapter) <= size );
    memset( buf, 0x00, size );
    return (TiUartAdapter *)buf;
}

void uart_destroy( TiUartAdapter * uart )
{
	uart_close( uart );
}

/****************************************************************************** 
 * initialze the UART hardware and object
 * @param
 * 	id		0	UART0
 * 			1 or other values	UART1
 * @return 
 * 	0		success
 *  -1		failed
 * 
 * @modified by zhangwei on 20061010
 * @TODO
 * zhangwei kept the old declaration of the function in order to keep other modules 
 * running. you should call uart_configure() after uart_construct()
 *****************************************************************************/
/****************************************************************************** 
 * @TODO 20061013
 * if the uart adapter is driven by interrupt, then you should enable the interrupt 
 * in configure function. however, whether the ISR really works or not still depends
 * on the global interrupt flag. 
 *
 * @assume: the global interrupt should be disabled before calling this function.
 * @todo stop bits input is actually no use now.
 *****************************************************************************/
TiUartAdapter * uart_open( TiUartAdapter * uart, uint8 id, uint16 baudrate, uint8 databits, uint8 stopbits, uint8 option )
{
    //USART_InitTypeDef USART_InitStructure;

	/* assume: 
	 * - the global interrupt is disabled when calling this function 
	 * - you have already call HAL_SET_PIN_DIRECTIONS. the pin should initialized correctly. or else it doesn't work.
	 */

	uart->id = id;
	uart->baudrate = baudrate;
	uart->databits = databits;
	uart->stopbits = stopbits;
	uart->option = option;

    #ifdef CONFIG_UART_INTERRUPT_DRIVEN
    uart->txlen = 0;
	uart->txidx = 0;
    uart->rxlen = 0;
    uart->listener = NULL;
	uart->lisowner = NULL;
    #endif

	switch (id)
	{
	    case 0:
            IO_PER_LOC_USART0_AT_PORT0_PIN2345();
            SET_MAIN_CLOCK_SOURCE(CRYSTAL);

            UART_SETUP(0, 38400, HIGH_STOP);
            //UART_SETUP(0, 57600, HIGH_STOP);
            UTX0IF = 1;
		    break;

	    case 1:
            break;

        case 2:
		    break;

	    default:
            // not support now.
            hal_assert(false);
		    uart = NULL;
		    break;
	}

	return uart;
}

void USART_Init(char uart, char USART_InitStruct)
{ 
     
   IO_PER_LOC_USART0_AT_PORT0_PIN2345();
   SET_MAIN_CLOCK_SOURCE(CRYSTAL);

   UART_SETUP(0, 57600, HIGH_STOP);
   UTX0IF = 1;
}


void uart_close( TiUartAdapter * uart )
{
	// todo:
	// you should disable interrutps here
	// 

	#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart->rxlen = 0;
	uart->txlen = 0;
    
    switch (uart->id)
    {
    case 0:
		//hal_detachhandler( INTNUM_USART1_RX );
		//hal_detachhandler( INTNUM_USART1_UDRE );
        break;
    case 1:
		//hal_detachhandler( INTNUM_USART1_RX );
		//hal_detachhandler( INTNUM_USART1_UDRE );
    };
	#endif
}

/******************************************************************************* 
 * this function is hardware related
 * you should change the register in this function
 *
 * attention: this function will return immediately. it will not wait for the 
 * incoming data. if there's no arrival data pending in the USART's register,
 * this function will simply return -1.
 * 
 * @return
 * 	0		success, *ch is char just read from UART
 *  -1		failed. The value of -1 means the uart object doesn't exists.
 ******************************************************************************/
intx uart_getchar( TiUartAdapter * uart, char * pc )
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    intx ret = -1;
    hal_enter_critical();
    if (uart->rxlen > 0)
    {
        *pc = uart->rxbuf[0];
        uart->rxlen --;
        memmove( (void *)&(uart->rxbuf[0]), (void *)&(uart->rxbuf[1]), uart->rxlen );
        ret = 0;
    }
    hal_leave_critical();
    return ret;
#endif

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
    int8 ret=0;
    char c;
    BYTE status;

    switch (uart->id)
    {
    case 0:
        // Turning on reception
        status = U0CSR;
        U0CSR |= UART_ENABLE_RECEIVE;

        while (!URX0IF );
        c = U0DBUF;
        URX0IF = FALSE;

        // Restoring old state
        U0CSR = status;
        *pc = c;
        ret = 0;
        break;

    case 1:
        break;
            
    case 2:
        break;

    default:
        ret = -1;
    }
    
    return ret;
#endif
}

char uart_getchar_wait( TiUartAdapter * uart )   
{
    #ifdef CONFIG_UART_INTERRUPT_DRIVEN
    char ch=0;
    while (uart->rxlen <= 0) {};
    uart_getchar( uart, &ch );
    return ch;
    #endif

    #ifndef CONFIG_UART_INTERRUPT_DRIVEN
	char ch=0;
    BYTE status;
    switch (uart->id)
    {
    case 0:
        // Turning on reception
        status = U0CSR;
        U0CSR |= UART_ENABLE_RECEIVE;

        while (!URX0IF );
        ch = U0DBUF;
        URX0IF = FALSE;

        // Restoring old state
        U0CSR = status;
        break;
    case 1:
        break;
    case 2:
        //while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET){};
        //ch = (USART_ReceiveData(USART3) & 0xFF); 
        break;
    default:
        ch = 0x00;
        break;
    }

    return ch;
    #endif
}

/* uart_putchar()
 * this function sends one character only through the UART hardware. 
 * 
 * @return
 *	0 or positive means success, and -1 means failed (ususally due to the buffer is full)
 *  when this functions returns -1, you need retry.
 * 
 * -1 means the uart object doesn't exist.
 */
intx uart_putchar( TiUartAdapter * uart, char ch )
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    intx ret;
/*
    // put the character to be send in the internal buffer "txbuf"
    //
    hal_enter_critical();
    ret = CONFIG_UART_TXBUFFER_SIZE - uart->txlen;
    if (ret > 0)
    {
        uart->txbuf[uart->txlen] = ch;
        uart->txlen ++;
    }

    // if the background ISR sending doesn't active, then enable and trigger this interrupt
    //
    if (uart->txidx == 0)
    {
        switch (uart->id)
        {
        case 0:
            while (!(UCSR0A & (1<<UDRE0))) {};      // check whether UDR0 is available for the 
            // new character to be sent
            UCSR0B |= (1 << UDRIE);                 // enable sending interrupt. the ISR will
            // continue sending until uart->txbuf is emtpy
            break;
        case 1:
            UCSR1B |= (1 << UDRIE);
            break;
        }
    }	
    hal_leave_critical();
*/
    return (ret > 0) ? 0 : -1;
#endif

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
    intx ret=1;

    switch (uart->id)
    {
    case 0:
        // if (c == '\n')  {
        //     while (!UTX0IF);
        //     UTX0IF = 0;
        //     U0DBUF = 0x0d;       /* output CR  */
        // }

        while (!UTX0IF);
        UTX0IF = 0;
        U0DBUF= ch;
        break;

    case 1:
        // if (c == '\n')  
        // {
        //     while (!UTX1IF);
        //     UTX1IF = 0;
        //     U1DBUF = 0x0d;       /* output CR  */
        // }

        while (!UTX1IF);
        UTX1IF = 0;
        break;
        
    case 2:
    default: 
        ret = -1;
    }
    
    return ret;
#endif
}

intx uart_read( TiUartAdapter * uart, char * buf, intx size, uint8 opt )
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    intx copied=0;
    int8 count =0;
    
    hal_enter_critical();
    copied = min( uart->rxlen, size );
    if (copied > 0)
    {
        memmove( (void *)buf, (void *)&(uart->rxbuf[0]), copied );
        uart->rxlen -= copied;
        if (uart->rxlen > 0)
            memmove( (void *)&(uart->rxbuf[0]), (void *)&(uart->rxbuf[copied]), uart->rxlen );
    }
    hal_leave_critical();
    return copied;
#endif

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
    intx ret=0;
    if (size > 0)
    {
        ret = uart_getchar( uart, buf );
	}
    return ret;
#endif
}

intx uart_write( TiUartAdapter * uart, char * buf, intx len, uint8 opt )
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    intx count = 0;

/*  
    hal_enter_critical();
    count = min(CONFIG_UART_TXBUFFER_SIZE - uart->txlen, len);
    if (count > 0)
    {
        memmove( (void *)&(uart->txbuf[uart->txlen]), (void *)buf, count);
        uart->txlen += count;
    }

    // if the background ISR sending doesn't active, then enable and trigger this interrupt
    //
    if (uart->txidx == 0)
    {
        switch (uart->id)
        {
        case 0:
            while (!(UCSR0A & (1<<UDRE0))) {};      // check whether UDR0 is available for the 
            // new character to be sent
            UCSR0B |= (1 << UDRIE);                 // enable sending interrupt. the ISR will
            // continue sending until uart->txbuf is emtpy
            break;
        case 1:
            UCSR1B |= (1 << UDRIE);
            break;
        }
    }	
    hal_leave_critical();

    // default option is synchronous call. this function will return until all 
    // data being sent successfully.
    //
    if ((opt & 0x01) == 0)
    {
        // wait for sending complete
        while (uart->txlen > 0) {};

        // attention
        // if the parameter "len" is very large, then we "recursively" call uart_write()
        // to send all the data out.  
        //
        if (count < len)
        {
            uart_write( uart, (char *)buf+count, len-count, opt );
            while (uart->txlen > 0) {};
        }
        return len;
    }
    // if this is an asynchronous call, this function will return immediately 
    // and assume the "count" characters in the front of the "buf" has been 
    // sent successfully.
    else{
        return count;
    }
*/
    return 0;    
#endif

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
    intx count = len;
/*    
    while (count > 0)
    {
        if (uart_putchar(uart, buf[len-count]) <= 0)
            break;
        count --;
    }
*/    
    return len-count;
#endif
}

// @todo The following interrupt handler needs deep revised
/*
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void _uart_rx0_interrupt_handler( void * uartptr, TiEvent * e )
{ 
    TiUartAdapter * uart = (TiUartAdapter *)uartptr;
    if (CONFIG_UART_RXBUFFER_SIZE -  uart->rxlen> 0)
    {
        uart->rxbuf[uart->rxlen] = UDR0;
        uart->rxlen ++;
    } 
} 
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void _uart_tx0_interrupt_handler( void * uartptr, TiEvent * e )
{ 
    TiUartAdapter * uart = (TiUartAdapter *)uartptr;
    if (uart->txidx < uart->txlen)
    {
        UDR0 = uart->txbuf[uart->txidx];
        uart->txidx ++;
    }
    else{
        UCSR0B &= (~(1 << UDRIE));                 // disable sending interrupt. 
        uart->txlen = 0;
        uart->txidx = 0;
    }
} 
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void _uart_rx1_interrupt_handler( void * uartptr, TiEvent * e )
{ 
    TiUartAdapter * uart = (TiUartAdapter *)uartptr;
    if (CONFIG_UART_RXBUFFER_SIZE -  uart->rxlen> 0)
    {
        uart->rxbuf[uart->rxlen] = UDR1;
        uart->rxlen ++;
    } 
} 
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void _uart_tx1_interrupt_handler( void * uartptr, TiEvent * e )
{ 
    TiUartAdapter * uart = (TiUartAdapter *)uartptr;
    if (uart->txidx < uart->txlen)
    {
        UDR1 = uart->txbuf[uart->txidx];
        uart->txidx ++;
    }
    else{
        UCSR0B &= (~(1 << UDRIE));                 // disable sending interrupt. 
        uart->txlen = 0;
        uart->txidx = 0;
    }
} 
#endif
*/

TiBlockDeviceInterface * uart_get_blockinterface( TiUartAdapter * uart, TiBlockDeviceInterface * intf )
{
    hal_assert( intf != NULL );
    memset( intf, 0x00, sizeof(TiBlockDeviceInterface) );
    intf->provider = uart;
    intf->read = (TiFunBlockDeviceWrite)uart_read;
    intf->write = (TiFunBlockDeviceWrite)uart_write;
    intf->evolve = NULL;
    intf->switchtomode = NULL;
    return intf;
}



// @todo: to be merged with above 2012.04.16

/* @todo  to be deleted in May 2012

#define BUFFER_SIZE 32

typedef struct{
   UINT8 pointer;
   char text[BUFFER_SIZE];
} BUFFER;

BUFFER buffer;
*/

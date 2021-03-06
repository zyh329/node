/*******************************************************************************
 * osxdemo
 * osx is an component based, lightweight operating system kernel. 
 * this program demostrates how to develop an complete program and separate runnable services.
 *
 * @status
 *	- release
 *
 * @author zhangwei(TongJi University) on 20090706
 *	- first created
 * @modified by zhangwei(TongJi University) on 20091006
 *	- finished osx kernel, this demo program and compile them passed
 * @modified by yan shixing(TongJi University) on 20091112
 *	- tested success.
 ******************************************************************************/

#include "asv_configall.h"
#include <string.h>
#include "../../../common/openwsn/hal/hal_foundation.h"
#include "../../../common/openwsn/hal/hal_cpu.h"
#include "../../../common/openwsn/hal/hal_interrupt.h"
#include "../../../common/openwsn/hal/hal_targetboard.h"
#include "../../../common/openwsn/hal/hal_led.h"
#include "../../../common/openwsn/hal/hal_assert.h"
#include "../../../common/openwsn/hal/hal_timer.h"
#include "../../../common/openwsn/hal/hal_debugio.h"
#include "../../../common/openwsn/osx/osx_kernel.h"	
#include "../../../common/openwsn/hal/hal_event.h"
#include "asv_foundation.h"
#include "appsvc1.h"
#include "appsvc2.h"
#include "appsvc3.h"

/******************************************************************************* 
 * CONFIG_AUTO_STOP 
 * The timer will send event to the osx kernel. This macro decides whether the timer 
 * stops after some time.
 *
 * CONFIG_TIMER_DRIVE
 * Let the hardware timer to drive the kernel to run. If this macro is undefined, 
 * then the kernel is drived by an infinite loop.
 *
 * CONFIG_DISPATCHER_TEST_ENABLE
 * This macro is defined by default. If this macro is undefined, then the events 
 * generated are with NULL event handler. This will cause the osx kernel to searching 
 * for appropriate handler to process it. This is done by the dispatcher object 
 * inside the kernel.
 ******************************************************************************/

#define CONFIG_AUTO_STOP
#undef  CONFIG_AUTO_STOP

#undef  CONFIG_TIMER_DRIVE
//#define CONFIG_TIMER_DRIVE

#define CONFIG_DISPATCHER_TEST_ENABLE

#define CONFIG_UART_ID              0
#define CONFIG_TIMER_ID             2

TiAppService1                       m_svcmem1;
TiAppService2                       m_svcmem2;
TiAppService3                       m_svcmem3;
TiTimerAdapter                      m_timer;
volatile uint16                              g_count=0;

void on_timer_expired( void * object, TiEvent * e );

/******************************************************************************* 
 * main()
 ******************************************************************************/

int main()
{
	TiAppService1 * asv1;
	TiAppService2 * asv2;
	TiAppService3 * asv3;
    TiTimerAdapter * evt_timer;
	char * msg = "welcome to osxdemo...";

	target_init();
	//dbc_write( msg, strlen(msg) );	

	led_open(LED_RED);
	led_on( LED_RED );
	hal_delayms( 1000 );
	led_off( LED_RED );

	rtl_init( (void *)dbio_open(38400), (TiFunDebugIoPutChar)dbio_putchar, (TiFunDebugIoGetChar)dbio_getchar, hal_assert_report );

	g_count = 0;

	/* interrupt must keep disabled during the osx initializing process. It will 
	 * enabled until osx_execute(). */
	osx_init();

	/* @attention: The timer_setinterval() cannot accept large duration values because
	 * the hardware may not support it 
	 * 
	 * Q: what's the maximum value of timer_setinterval for each hardware timer?
	 * A: 1~8 (???)  */
	evt_timer = timer_construct( (void *)&m_timer, sizeof(TiTimerAdapter) );
	timer_open( evt_timer, CONFIG_TIMER_ID, on_timer_expired, (void*)g_osx, 0x01 );
	timer_setinterval( evt_timer, 20, 1 );


	/* create and initialize three runnable application services. the runnable service
	 * is quite similar to OS's process. however, the runnable service improves the 
	 * standard "process" with a data structure and event handler, which greatly simplied 
	 * the state machine pattern developing.  */
	
	asv1 = asv1_open( &m_svcmem1, sizeof(TiAppService1) );
	asv2 = asv2_open( &m_svcmem2, sizeof(TiAppService2) );
	asv3 = asv3_open( &m_svcmem3, sizeof(TiAppService3) );

	/* put the runnable application service into osx. then osx can dispatch events to 
	 * these services. the services only run when it receives an event, namely, the 
	 * events drives the service to forward according to the state machine.  */
	osx_attach( EVENT_WAKEUP, asv1_evolve, asv1 );
	//osx_attach( EVENT_WAKEUP, asv2_evolve, asv2 );
	//osx_postx(2,asv2_evolve,asv2,asv2);
	//osx_taskspawn(asv1_evolve, asv1, 1, 0, 0 );

	/* configure the listener relation between service 2 and service 3.
	 * you can also use
	 *		osx_attach( 3, asv3_evolve, asv3 );
	 * however, the following code demonstrates how to implement complex relations 
	 * among services.  */
	//asv2_setlistener( asv2, (TiFunEventHandler)asv3_evolve, (void *)asv3 );

	/* when the osx kernel really executed, it will enable the interrupts so that the 
	 * whole program can accept interrupt requests.
     * attention: osx kernel already support sleep/wakeup because the sleep/wakeup 
	 * handler have been registered inside the osx itself.  */
 	//dbc_putchar(0xf1);
 	timer_start( evt_timer );

	#ifndef CONFIG_TIMER_DRIVE
 	osx_execute();
	#endif

	#ifdef CONFIG_TIMER_DRIVE
	osx_hardexecute();
	#endif
}


/* This is the timer's listener function. Everytime the timer expired, this function
 * will be called. So we can generate the event and put it into the system in this 
 * function.
 *
 * In this case, we set newe.handler and newe.objectto to NULL. This will cause 
 * the kernel to use dispatcher to find appropriate service.
 */
void on_timer_expired( void * object, TiEvent * e )
{
	if((g_count%50 == 0) && (g_count<600))
	{
		osx_wakeup_request();
		led_toggle(LED_RED);
	 	dbc_putchar(g_count/10);
	}
	g_count++;
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}


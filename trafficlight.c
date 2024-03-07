
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include "stm32f4_discovery.h"
/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"



/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 100

#define green  	0
#define amber  	1
#define red  	2

#define shift_reg_GPIO_port 	GPIOC
#define shift_reg_DATA_pin 		GPIO_Pin_6
#define shift_reg_CLOCK_pin 	GPIO_Pin_7
#define shift_reg_RESET_pin 	GPIO_Pin_8

#define traffic_light_GPIO_port GPIOB
#define red_light_pin 			GPIO_Pin_4
#define amber_light_pin 		GPIO_Pin_5
#define green_light_pin 		GPIO_Pin_6

#define ADC_GPIO_port 			GPIOA
#define ADC_pin 				GPIO_Pin_1



static void prvSetupHardware( void );

/*
 * The queue send and receive tasks as described in the comments at the top of
 * this file.
 */




static void my_ADC_Init();
static void my_GPIO_Init();
static void Traffic_Flow_Adjustment_Task( void *pvParameters );
static void Traffic_Generator_Task( void *pvParameters );
//static void Traffic_Light_State_Task( void *pvParameters );
static void System_Display_Task( void *pvParameters );
static void update_traffic(int n, uint16_t car_positions[]);
static void green_timer_callback();
static void amber_timer_callback();
static void red_timer_callback();



xQueueHandle adc_light_queue = 0;
xQueueHandle adc_traffic_queue = 0;
xQueueHandle traffic_light_queue = 0;
xQueueHandle car_postion_queue = 0;

xTimerHandle green_light_timer;
xTimerHandle amber_light_timer;
xTimerHandle red_light_timer;

/*-----------------------------------------------------------*/

int main(void)
{

	my_GPIO_Init();
	my_ADC_Init();


	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();


	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */
	adc_light_queue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
							sizeof( uint16_t ) );	/* The size of each item the queue holds. */

	adc_traffic_queue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
								sizeof( uint16_t ) );	/* The size of each item the queue holds. */

	traffic_light_queue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
								sizeof( uint16_t ) );	/* The size of each item the queue holds. */

	car_postion_queue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
									sizeof( uint16_t ) );	/* The size of each item the queue holds. */

	green_light_timer = xTimerCreate("timer_green", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, green_timer_callback );
	amber_light_timer = xTimerCreate("timer_amber", pdMS_TO_TICKS(1500), pdFALSE, (void*)0, amber_timer_callback );
	red_light_timer = xTimerCreate("timer_red", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, red_timer_callback );

	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( adc_light_queue, "adc_light_queue" );
	vQueueAddToRegistry( adc_traffic_queue, "adc_traffic_queue" );
	vQueueAddToRegistry( traffic_light_queue, "traffic_light_queue" );
	vQueueAddToRegistry( car_postion_queue, "car_postion_queue" );

	//xTaskCreate( Manager_Task, "Manager", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	//xTaskCreate( Blue_LED_Controller_Task, "Blue_LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	//xTaskCreate( Red_LED_Controller_Task, "Red_LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	//xTaskCreate( Green_LED_Controller_Task, "Green_LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	//xTaskCreate( Amber_LED_Controller_Task, "Amber_LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	xTaskCreate( Traffic_Flow_Adjustment_Task, "Traffic_Flow", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
	xTaskCreate( Traffic_Generator_Task, "Traffic_Generator", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	//xTaskCreate( Traffic_Light_State_Task, "Traffic_Light_State", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate( System_Display_Task, "System_Display", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	//turn on green light, start green light timer
	GPIO_SetBits(traffic_light_GPIO_port, green_light_pin);
	xTimerStart(green_light_timer, 0);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();



	return 0;
}


/*-----------------------------------------------------------*/

static void my_GPIO_Init(){

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_InitTypeDef Shift_Init_Struct;

	Shift_Init_Struct.GPIO_Pin = shift_reg_DATA_pin | shift_reg_CLOCK_pin | shift_reg_RESET_pin;
	Shift_Init_Struct.GPIO_Mode = GPIO_Mode_OUT;
	Shift_Init_Struct.GPIO_OType = GPIO_OType_PP;
	Shift_Init_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	Shift_Init_Struct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &Shift_Init_Struct);
	GPIO_SetBits(shift_reg_GPIO_port, shift_reg_RESET_pin);

	GPIO_InitTypeDef Shift_Init_Struct_light;

	Shift_Init_Struct_light.GPIO_Pin = red_light_pin | amber_light_pin | green_light_pin;
	Shift_Init_Struct_light.GPIO_Mode = GPIO_Mode_OUT;
	Shift_Init_Struct_light.GPIO_OType = GPIO_OType_PP;
	Shift_Init_Struct_light.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &Shift_Init_Struct_light);

	GPIO_SetBits(traffic_light_GPIO_port, red_light_pin);
	GPIO_ResetBits(traffic_light_GPIO_port, red_light_pin);

	GPIO_SetBits(traffic_light_GPIO_port, amber_light_pin);
	GPIO_ResetBits(traffic_light_GPIO_port, amber_light_pin);

	GPIO_SetBits(traffic_light_GPIO_port, green_light_pin);
	GPIO_ResetBits(traffic_light_GPIO_port, green_light_pin);

}

static void my_ADC_Init(){
	GPIO_InitTypeDef ADC_GPIO_Init_Struct;
	ADC_InitTypeDef ADC_Init_Struct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_GPIO_Init_Struct.GPIO_Pin = ADC_pin;
	ADC_GPIO_Init_Struct.GPIO_Mode = GPIO_Mode_AN;
	ADC_GPIO_Init_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(ADC_GPIO_port, &ADC_GPIO_Init_Struct);

	ADC_Init_Struct.ADC_ScanConvMode = DISABLE;
	ADC_Init_Struct.ADC_Resolution = ADC_Resolution_12b;
	ADC_Init_Struct.ADC_ContinuousConvMode = ENABLE;
	ADC_Init_Struct.ADC_ExternalTrigConv = DISABLE;
	ADC_Init_Struct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_Init_Struct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init_Struct.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_Init_Struct);

	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_3Cycles);
}

static void green_timer_callback(){
	uint16_t traffic_light_val = amber;
	if( xQueueSend(traffic_light_queue, &traffic_light_val, 1000)){
		//printf("Changed to amber\n");
		GPIO_ResetBits(traffic_light_GPIO_port, green_light_pin);
		GPIO_SetBits(traffic_light_GPIO_port, amber_light_pin);
	}

	xTimerStart(amber_light_timer, 0);

}

static void amber_timer_callback(){
	uint16_t adc_val;
	uint16_t traffic_light_val = red;
	if( xQueueSend(traffic_light_queue, &traffic_light_val, 1000)){
		//printf("Changed to red\n");
		GPIO_ResetBits(traffic_light_GPIO_port, amber_light_pin);
		GPIO_SetBits(traffic_light_GPIO_port, red_light_pin);
	}
	if(xQueuePeek(adc_light_queue, &adc_val, 100)){
		int period = 1000*(8000/adc_val);
		if(period > 4000){
			period = 4000;
		}
		printf("adc_val: %d\n", adc_val);
		printf("Red light period: %d\n", period);
		xTimerChangePeriod(red_light_timer, pdMS_TO_TICKS(period), 0);
	}
	xTimerStart(red_light_timer, 0);
}

static void red_timer_callback(){
	uint16_t adc_val;
	uint16_t traffic_light_val = green;
	if( xQueueSend(traffic_light_queue, &traffic_light_val, 1000)){
		//printf("Changed to green\n");
		GPIO_ResetBits(traffic_light_GPIO_port, red_light_pin);
		GPIO_SetBits(traffic_light_GPIO_port, green_light_pin);
	}
	if(xQueuePeek(adc_light_queue, &adc_val, 500)){
		int period = adc_val + 1000;
		printf("Green light period: %d\n", period);
		xTimerChangePeriod(green_light_timer, pdMS_TO_TICKS(period), 0);
	}
	xTimerStart(green_light_timer, 0);
}

static void Traffic_Flow_Adjustment_Task( void *pvParameters ){

	uint16_t adc_val = 0;
	uint16_t new_adc_val;

	while(1){

		ADC_SoftwareStartConv(ADC1);
		while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

		new_adc_val = ADC_GetConversionValue(ADC1);
		//printf("adc_val = %d\n", new_adc_val);

		if(abs(new_adc_val - adc_val) > 100){

			xQueueReset(adc_light_queue);
			if( xQueueSend(adc_light_queue,&new_adc_val,1000))
			{
				//printf("ADC Light Queue : %u\n", new_adc_val);
			}
			else
			{
				printf("Traffic_Flow_Adjustment Failed!\n");
			}
		}



		if( xQueueSend(adc_traffic_queue,&new_adc_val,1000))
		{
			//printf("ADC Traffic Queue : %u\n", new_adc_val);
		}
		else
		{
			printf("Traffic_Flow_Adjustment Failed!\n");
		}
		vTaskDelay(1000);

	}


}

static void Traffic_Generator_Task( void *pvParameters )
{
	uint16_t adc_val;
	uint16_t i = 0;


	while(1)
	{
		uint16_t random = rand() % 4000; //generate random between 0 abd 5000
		printf("Random: %d\nadc_val + 1000: %d\n", random, adc_val + 1000);
		if(random < adc_val + 1000){
			printf("Sending car.\n");
			i = 1;
		}
		else{
			i = 0;
		}
		//TODO this is not receiving the right adc val for some reason.
		if(xQueueReceive(adc_traffic_queue, &adc_val, 500))
		{
			//printf("ADC val received: %u\n", adc_val);
			if( xQueueSend(car_postion_queue, &i, 1000)){
				//printf("car position queue: %u\n", i);
			}
			else{
				printf("car position failed\n");
			}
		}
		vTaskDelay(1000);

		//Need a queue to send 1 or 0 for car to system_display_task
	}
}



static void System_Display_Task( void *pvParameters )
{
	uint16_t car_positions[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //len = 19
	uint16_t traffic_light_val;
	uint16_t new_car_position;
	uint16_t before_light = 8;
	uint16_t total_light = 19;
	uint16_t space_in_traffic = 0;
	while(1)
	{
		space_in_traffic = 0;
		if(xQueueReceive(traffic_light_queue, &traffic_light_val, 500))
		{
			//printf("Traffic light received: %u\n", traffic_light_val);

		}

		if(xQueueReceive(car_postion_queue, &new_car_position, 500))
		{
			car_positions[0] = new_car_position;
			for(int i = total_light - 1; i > 0; i--){

				if(traffic_light_val == green){

					car_positions[i] = car_positions[i - 1];

				}else{
					if(i > before_light){
						//past traffic light
						if(i == 9){
							car_positions[i] = 0;
						}else{
							car_positions[i] = car_positions[i - 1];
						}

					}else if(i <= before_light){
						//before traffic light
						if(car_positions[i] == 0){
							space_in_traffic = 1;
						}
						if(space_in_traffic){
							car_positions[i] = car_positions[i - 1];
						}

					}

				}

			}
			for(int i = 0; i < total_light; i++){
				//printf("%u, ", car_positions[i]);
				if(i == before_light){
					//printf("||%u|| " ,traffic_light_val);
				}
			}
			//printf("\n");
			update_traffic(total_light, car_positions);

			vTaskDelay(1000);


		}

	}
}

static void update_traffic(int n, uint16_t car_positions[]){
	uint16_t position;
//	printf("output:\n");
//	for(int i = 0; i < n; i++){
//		printf("%u, ", car_positions[i]);
//	}
//	printf("\n");
	for (int i = n - 1; i > 0; i--){
		position = car_positions[i];
		if (position == 0){
			GPIO_ResetBits(shift_reg_GPIO_port, shift_reg_DATA_pin);
		}
		else{
			GPIO_SetBits(shift_reg_GPIO_port, shift_reg_DATA_pin);
		}
		GPIO_SetBits(shift_reg_GPIO_port,  shift_reg_CLOCK_pin); //clock
		GPIO_ResetBits(shift_reg_GPIO_port,  shift_reg_CLOCK_pin);
	}
}



/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software 
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.

	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	/* TODO: Setup the clocks, etc. here, if they were not configured before
	main() was called. */
}



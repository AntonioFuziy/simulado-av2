/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "arm_math.h"
#include "math.h"
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"

/************************************************************************/
/* STATIC                                                               */
/************************************************************************/

/*A static or global variable to store the buffers*/
static lv_disp_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_APS2_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_APS2_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
  printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
  for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
  configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* prototypes */
/************************************************************************/

static void send_package(char data[], char n);
void lv_debug(void);

/************************************************************************/
/* globas                                                                */
/************************************************************************/

lv_obj_t * labelDebug;
QueueHandle_t xQueueRx;

/************************************************************************/
/* handlers                                                             */
/************************************************************************/

void USART1_Handler(void){
  uint32_t ret = usart_get_status(CONSOLE_UART);

  BaseType_t xHigherPriorityTaskWoken = pdTRUE;
  char c;

  // Verifica por qual motivo entrou na interrup?cao
  //  - Dadodispon?vel para leitura
  if(ret & US_IER_RXRDY){
    usart_serial_getchar(CONSOLE_UART, &c);
    xQueueSendFromISR(xQueueRx, (void *) &c, &xHigherPriorityTaskWoken);

    // -  Transmissoa finalizada
    } else if(ret & US_IER_TXRDY){

  }
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
  lv_debug();
  for (;;)  {
    lv_tick_inc(50);
    lv_task_handler();
    vTaskDelay(50);
  }
}

static void task_receive(void *pvParameters) {
  char p_data[32];
  char p_cnt = 0;
  char p_do = 0;
  
  for (;;)  {    
    char c;
    if (xQueueReceive( xQueueRx, &c, 0 )) {
      p_data[p_cnt++] = c;
      if (p_data[p_cnt - 1] == 'X'){
        p_do = 1;
      }
    }
    
    if(p_do == 1) {
      lv_label_set_text_fmt(labelDebug, "%02X %02X %02X %02X",p_data[0], p_data[1], p_data[2], p_data[3] );
      p_cnt = 0;
      p_do = 0;
    }

  }
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void but_check(lv_obj_t * obj, lv_event_t event) {
  char p[] = {'U', 0, 0, 'X'};
  if(event == LV_EVENT_CLICKED)
  send_package(p, 4);
}

static void but_cobrar(lv_obj_t * obj, lv_event_t event) {
  char p[] = {'U', 1, 15, 'X'};
  if(event == LV_EVENT_CLICKED)
  send_package(p, 4);
}

static void but_verifica(lv_obj_t * obj, lv_event_t event) {
  char p[] = {'U', 2, 0, 'X'};
  if(event == LV_EVENT_CLICKED)
  send_package(p, 4);
}

void lv_debug(void) {
  lv_obj_t * label;
  lv_obj_t * btn1 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn1, but_check);
  lv_obj_align(btn1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  label = lv_label_create(btn1, NULL);
  lv_label_set_text(label, "Check");

  lv_obj_t * btn2 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn2, but_cobrar);
  lv_obj_align(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  label = lv_label_create(btn2, NULL);
  lv_label_set_text(label, "Cobrar");
  
  lv_obj_t * btn3 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn3, but_verifica);
  lv_obj_align(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  label = lv_label_create(btn3, NULL);
  lv_label_set_text(label, "Verifica");
  
  labelDebug = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_width(labelDebug, 150);
  lv_obj_align(labelDebug, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);
}

/************************************************************************/
/* funcs                                                               */
/************************************************************************/

static void send_package(char data[], char n) {
  for (int i = 0; i < n; i++){
    while(!usart_is_tx_ready(CONSOLE_UART)) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    usart_write(CONSOLE_UART, data[i]);
  }
}

static void configure_lcd(void) {
  /**LCD pin configure on SPI*/
  pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
  pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
  pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
  pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
  pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
  pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
  
}

static void configure_console(void) {
  const usart_serial_options_t uart_serial_options = {
    .baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
    .charlength = USART_SERIAL_CHAR_LENGTH,
    .paritytype = USART_SERIAL_PARITY,
    .stopbits = USART_SERIAL_STOP_BIT,
  };

  /* Configure console UART. */
  stdio_serial_init(CONSOLE_UART, &uart_serial_options);

  /* Specify that stdout should not be buffered. */
  setbuf(stdout, NULL);
  
  
  /* ativando interrupcao */
  usart_enable_interrupt(CONSOLE_UART, US_IER_RXRDY);
  NVIC_SetPriority(CONSOLE_UART_ID, 4);
  NVIC_EnableIRQ(CONSOLE_UART_ID);
}

static void USART1_init(void){
  /* Configura USART1 Pinos */
  sysclk_enable_peripheral_clock(ID_PIOB);
  sysclk_enable_peripheral_clock(ID_PIOA);
  pio_set_peripheral(PIOB, PIO_PERIPH_D, PIO_PB4); // RX
  pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA21); // TX
  MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;

  /* Configura opcoes USART */
  const sam_usart_opt_t usart_settings = {
    .baudrate       = 115200,
    .char_length    = US_MR_CHRL_8_BIT,
    .parity_type    = US_MR_PAR_NO,
    .stop_bits   	= US_MR_NBSTOP_1_BIT	,
    .channel_mode   = US_MR_CHMODE_NORMAL
  };

  /* Ativa Clock periferico USART0 */
  sysclk_enable_peripheral_clock(CONSOLE_UART_ID);

  /* Configura USART para operar em modo RS232 */
  usart_init_rs232(CONSOLE_UART, &usart_settings, sysclk_get_peripheral_hz());

  /* Enable the receiver and transmitter. */
  usart_enable_tx(CONSOLE_UART);
  usart_enable_rx(CONSOLE_UART);

  /* map printf to usart */
  ptr_put = (int (*)(void volatile*,char))&usart_serial_putchar;
  ptr_get = (void (*)(void volatile*,char*))&usart_serial_getchar;

  /* ativando interrupcao */
  usart_enable_interrupt(CONSOLE_UART, US_IER_RXRDY);
  NVIC_SetPriority(CONSOLE_UART_ID, 4);
  NVIC_EnableIRQ(CONSOLE_UART_ID);

}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
  ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
  ili9341_copy_pixels_to_screen(color_p,  (area->x2 - area->x1) * (area->y2 - area->y1));
  
  /* IMPORTANT!!!
  * Inform the graphics library that you are ready with the flushing*/
  lv_disp_flush_ready(disp_drv);
}

bool my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
  int px, py, pressed;
  
  if (readPoint(&px, &py)) {
    data->state = LV_INDEV_STATE_PR;
  }
  else {
    data->state = LV_INDEV_STATE_REL;
  }
  
  data->point.x = px;
  data->point.y = py;
  return false; /*No buffering now so no more data read*/
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
  
  xQueueRx = xQueueCreate(32, sizeof(char));
  
  /* board and sys init */
  board_init();
  sysclk_init();
  USART1_init();

  /* LCd int */
  configure_lcd();
  ili9341_init();
  configure_touch();
  ili9341_backlight_on();
  
  /*LittlevGL init*/
  lv_init();
  lv_disp_drv_t disp_drv;                 /*A variable to hold the drivers. Can be local variable*/
  lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
  lv_disp_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);  /*Initialize `disp_buf` with the buffer(s) */
  disp_drv.buffer = &disp_buf;            /*Set an initialized buffer*/
  disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
  lv_disp_t * disp;
  disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
  
  /* Init input on LVGL */
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_input_read;
  /*Register the driver in LVGL and save the created input device object*/
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
  

  if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create lcd task\r\n");
  }
  
  if (xTaskCreate(task_receive, "main", TASK_APS2_STACK_SIZE, NULL, TASK_APS2_STACK_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create Main task\r\n");
  }
  
  
  /* Start the scheduler. */
  vTaskStartScheduler();

  /* RTOS n?o deve chegar aqui !! */
  while(1){ }
}
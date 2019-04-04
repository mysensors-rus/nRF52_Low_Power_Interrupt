/**
Пример испльзования малопотребляющего компаратора (LPCOMP) в режиме сна

*/

#define MY_DEBUG
#define MY_RADIO_NRF5_ESB
#define MY_PASSIVE_NODE
#define MY_NODE_ID 9
#define MY_LED 8
#define MY_BUTTON_ADC_PIN 1 // номер пина АЦП смотреть в MyBoardNRF5.cpp Имеется в виду именно аналоговый пин!

#include <MySensors.h>
#define CHILD_ID 1   // Id of the sensor child
// Initialize motion message
MyMessage msg(CHILD_ID, V_TRIPPED);

volatile bool detection; // ставим в true при срабатывании прерывания

void preHwInit() {
pinMode(MY_LED, OUTPUT);  
digitalWrite(MY_LED, LOW);
}

void before()
{
NRF_POWER->DCDCEN = 1; // закоментировать если E73
//NRF_UART0->ENABLE = 0;  
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Motion Sensor (LPCOMP)", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID, S_MOTION);
}

void setup() {
  analogReference(AR_INTERNAL);
  lpComp_init();	// Настраиваем регистр LPCOMP, в т.ч. и прерывание
  led(2,1);
  detection = false;
  wait(5000);
}

void loop() {
  sleep(3600000);
  if (detection) {
    lpComp_irq_disable(); // Отключаем прерывание
	
	detection=false;
    send(msg.set(detection));  // Посылаем сообщение в MySensors
    led(10,3);
    //sNRF_LPCOMP->EVENTS_DOWN=0;

    lpComp_irq_enable(); // Включаем прерывание
  } else {
    led(1,1);
  }
}

// Мигалка
void led(uint8_t flash, uint8_t iteration) {
  for (int x = 0; x < iteration; x++) {
    for (int i=0; i < flash; i++) {
      digitalWrite(MY_LED, HIGH);
      wait(30);
      digitalWrite(MY_LED, LOW);
      wait(70);
    }    
      wait(500);
  }
}

void lpComp_init() {	// настраиваем прерывание для LPCOMP
  NRF_LPCOMP->PSEL=MY_BUTTON_ADC_PIN;
  NRF_LPCOMP->ANADETECT=2; // детектирование EVENTS_DOWN.
  //ANADETECT=0; // детектирование EVENTS_CROSS. Т.е. в обе стороны
  //ANADETECT=1; // детектирование EVENTS_UP.
  //ANADETECT=2; // детектирование EVENTS_DOWN.
  NRF_LPCOMP->INTENSET=B0010; // активация прерывания для EVENTS_DOWN
  //B1000; // прерываниt для EVENTS_CROSS Т.е. в обе стороны
  //B0100; // прерываниt для EVENTS_UP
  //B0010; // прерываниt для EVENTS_DOWN
  NRF_LPCOMP->ENABLE=1;
  NRF_LPCOMP->TASKS_START=1;
  NVIC_SetPriority(LPCOMP_IRQn, 15); 	// Уст. приоритет прерывания
  NVIC_ClearPendingIRQ(LPCOMP_IRQn);	// Чистим
  NVIC_EnableIRQ(LPCOMP_IRQn);			// включаем прерывание
}

void lpComp_irq_disable() {
  if ((NRF_LPCOMP->ENABLE) && (NRF_LPCOMP->EVENTS_READY)) {
    NRF_LPCOMP->INTENCLR=B0010; //деактивация прерывания для LPCOMP
  }
}

void lpComp_irq_enable() {
  NRF_LPCOMP->INTENSET=B0010; //активация прерывания для LPCOMP
}

// #if __CORTEX_M == 0x04
// #define NRF5_RESET_EVENT(event)                                                 \
//         event = 0;                                                                   \
//         (void)event
// #else
// #define NRF5_RESET_EVENT(event) event = 0
// #endif

//В одну строку..
extern "C" {
  void LPCOMP_IRQHandler(void) {
    detection=true; 
    //NRF5_RESET_EVENT(NRF_LPCOMP->EVENTS_DOWN);
    NRF_LPCOMP->EVENTS_DOWN=0;
    NRF_RTC2->CC[0]=(NRF_RTC2->COUNTER+2);	// Нужно по ходу для того чтобы вывалиться из sleep
	}
  }

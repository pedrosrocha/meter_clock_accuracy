#include <Wire.h>
#include <U8g2lib.h>
#include <esp8266_peri.h>
#include <eagle_soc.h>
#include <gpio.h> 
#include <ESP8266WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SDA_PIN 14
#define SCL_PIN 12

#define INTERRUPT_PIN 4

#define NUMBER_OF_CYCLES_EQUAL_US 80
#define DELAY_FOR_LED_READING 1000

#define TIMER_INTERVAL_MS 5

bool StartMicroSecondsCounter = false;
int Led_Triggered = 0;

char buffer_time[10][5];
char buffer[10];

uint64_t Microseconds = 0;
uint64_t total_ticks = 0;
uint32_t ticks_start_count = 0;
uint32_t ticks_end_count = 0;

struct TimeComponents {
  uint32_t hours;
  uint32_t minutes;
  uint32_t seconds;
  uint32_t milliseconds;
  uint32_t microseconds;
};

os_timer_t timer; // Timer object

int FirstTimeRisingEdgeInterruption = 1;
int enableTimerRead = 0;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL_PIN, /* data=*/ SDA_PIN, /* reset=*/ OLED_RESET);

static inline uint32_t read_ccount(void) {
    uint32_t ccount;
    asm volatile ("rsr %0, ccount" : "=a" (ccount));
    return ccount;
}

TimeComponents convertMicroseconds(uint64_t totalMicroseconds) {
    TimeComponents time;

    time.hours = totalMicroseconds / 3600000000ULL;

    totalMicroseconds %= 3600000000ULL;
    time.minutes = totalMicroseconds / 60000000ULL;

    totalMicroseconds %= 60000000ULL;
    time.seconds = totalMicroseconds / 1000000ULL;

    totalMicroseconds %= 1000000ULL;
    time.milliseconds = totalMicroseconds / 1000ULL;

    totalMicroseconds %= 1000ULL;
    time.microseconds = totalMicroseconds;

    return time;
}


void timerCallback(void *arg) { //Timer interruption 1ms
  if(enableTimerRead == 1){

    ticks_end_count = read_ccount();
    total_ticks = total_ticks + (ticks_end_count - ticks_start_count);
    ticks_start_count = ticks_end_count;

  }
}

void ICACHE_RAM_ATTR handleInterrupt() {//rising edge interruption on pin D2
  delayMicroseconds(DELAY_FOR_LED_READING);
  uint32_t pinState = (GPIO_REG_READ(GPIO_IN_ADDRESS) & (1 << INTERRUPT_PIN)) != 0;

  if(pinState == 0){
      if(FirstTimeRisingEdgeInterruption == 1 ){

      ticks_start_count = read_ccount();
      total_ticks = 0;
      enableTimerRead = 1;
      FirstTimeRisingEdgeInterruption = 0;

      GPIO_OUTPUT_SET(LED_BUILTIN, 0); 
    }else{;

      ticks_end_count = read_ccount();
      enableTimerRead = 0;

      total_ticks = total_ticks + (ticks_end_count - ticks_start_count);
      FirstTimeRisingEdgeInterruption = 1;
      GPIO_OUTPUT_SET(LED_BUILTIN, 1); 
    }
  }


}

void setup() {
  u8g2.begin();

  Serial.begin(115200);  // Start serial communication at 115200 baud

  pinMode(INTERRUPT_PIN, INPUT);  // Set the interrupt pin as input
  pinMode(INTERRUPT_PIN, INPUT_PULLUP); // Set pin mode with internal pull-up resistor
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, FALLING); // The ISR (handleInterrupt) will be called on a rising edge

  os_timer_setfn(&timer, timerCallback, NULL);
  os_timer_arm(&timer, TIMER_INTERVAL_MS, true); // Start the timer with the specified interval (in milliseconds)

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  GPIO_OUTPUT_SET(LED_BUILTIN, 1);  // Initially turn off the built-in LED
  

  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Eletra Energy Solutions");
  u8g2.sendBuffer();					// transfer internal memory to the display

  delay(3000);

}

void loop() {
  TimeComponents timeStruct;
  Microseconds = total_ticks/NUMBER_OF_CYCLES_EQUAL_US;

  if(enableTimerRead == 1){
    sprintf(buffer, "%d", Microseconds/(1000*1000));

    u8g2.clearBuffer();				// clear the internal memory
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawStr(0, 15, "Counter Started:");
    u8g2.drawStr(0, 30, buffer);
    u8g2.drawGlyph(75, 55, 0x2603); //snowman
    u8g2.drawGlyph(110, 30, 0x2600); //sun
    u8g2.drawGlyph(90, 40, 0x2601); //cloud
    u8g2.drawGlyph(90, 50, 0x2602); //Umbrella
    u8g2.sendBuffer();					// transfer internal memory to the display
  }else{

    
    timeStruct = convertMicroseconds(Microseconds); 
    sprintf(buffer_time[0], "%d", timeStruct.hours);
    sprintf(buffer_time[1], "%d", timeStruct.minutes);
    sprintf(buffer_time[2], "%d", timeStruct.seconds);
    sprintf(buffer_time[3], "%d", timeStruct.milliseconds);
    sprintf(buffer_time[4], "%d", timeStruct.microseconds);

    u8g2.clearBuffer();				// clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0,  14, "Finished:");
    u8g2.drawStr(0,  24, "Hours: ");
    u8g2.drawStr(89, 24, buffer_time[0]);
    u8g2.drawStr(0,  34, "Minutes: ");
    u8g2.drawStr(89, 34, buffer_time[1]);
    u8g2.drawStr(0,  44, "Seconds: ");
    u8g2.drawStr(89, 44, buffer_time[2]);
    u8g2.drawStr(0,  54, "Milliseconds: ");
    u8g2.drawStr(89, 54, buffer_time[3]);
    u8g2.drawStr(0,  64, "Microseconds: ");
    u8g2.drawStr(89, 64, buffer_time[4]);
    u8g2.sendBuffer();					// transfer internal memory to the display

  }

  delay(500);
}

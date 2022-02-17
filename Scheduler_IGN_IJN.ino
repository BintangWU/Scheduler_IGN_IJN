#define Fosc              16000000UL
#define PRESCALER         256

#define TIMER_COUNTER     TCNT3
#define TIMER_COMPARE     OCR3A
#define TIMER_ENABLE      TIMSK3 |= (1 << OCIE3A)
#define TIMER_DISABLE     TIMSK3 &= ~(1 << OCIE3A)

enum scheduleStatus {
  OFF,
  PENDING,
  RUNNING
};

struct Schedule {
  volatile unsigned long timeDelay;
  volatile unsigned long duration;
  scheduleStatus status;

  void (*startCallBack)();
  void (*endCallBack)();
};

struct Schedule ignSchedule;
struct Schedule ijnSchedule;

void setup() {
  Serial.begin(115200);
  pinMode(32, OUTPUT);
  ignCharger();

  noInterrupts();
  detachInterrupt(digitalPinToInterrupt(19));
  timerSetup();

  attachInterrupt(digitalPinToInterrupt(19), triggerISR, FALLING);
  interrupts();
}

void loop() {
  // put your main code here, to run repeatedly:

}

inline void ignCharger() {
  digitalWrite(32, LOW);
}

inline void ignDischarge() {
  digitalWrite(32, HIGH);
}

void timerSetup() {
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;
  TIFR3 = 0;
  TCCR3B |= (1 << CS12); //Prescaler 256
  
  ignSchedule.status = OFF;
}

/*
 * Time/tick = Prescale * (1/Fosc)
 * exp. 256 * (1/16000000) = 16us
 * Untuk Mendapatkan nilau durasi compare register = Nominal timer(ms) / TimePerTick
 * exp. OCRxn = Nominal Timer(ms) / TimePerTick
 */
void setIgnSchedule(void (*startCallBack)(), unsigned long timeDelay, unsigned long duration, void (*endCallBack)()) {
  if (ignSchedule.status != RUNNING) {
    ignSchedule.duration = duration;
    ignSchedule.startCallBack = startCallBack;
    ignSchedule.endCallBack = endCallBack;

    noInterrupts();
    TIMER_COMPARE = (uint16_t)TIMER_COUNTER + (timeDelay >> 4);
    ignSchedule.status = PENDING;
    interrupts();
    TIMER_ENABLE;
  }
}

void triggerISR() {
  setIgnSchedule(ignDischarge, 1000, 4000, ignCharger);
}

ISR(TIMER3_COMPA_vect) {
  if (ignSchedule.status == PENDING) {
    ignSchedule.startCallBack();
    ignSchedule.status = RUNNING;
    TIMER_COMPARE = (uint16_t)TIMER_COUNTER + (ignSchedule.duration >> 4);
  }
  else if (ignSchedule.status == RUNNING) {
    ignSchedule.endCallBack();
    ignSchedule.status = OFF;
    TIMER_DISABLE;
  }
}

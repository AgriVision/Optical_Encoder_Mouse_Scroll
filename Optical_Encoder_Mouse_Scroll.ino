#include <DigiMouse.h>
#include <avr/interrupt.h>

// ------------------------------------------------------------
// Digispark ATtiny85 + E38S6G5-600B-G24N optical rotary encoder
// For use with SDR software
// Gerrit Polder, PA3BYA
//
// Encoder A (white) -> D2 / PB2 / INT0
// Encoder B (green) -> D0 / PB0
//
// Encoder:
//   600 pulses/revolution
//   2x decoding = 1200 counts/revolution
//
// Outputs:
//   USB HID mouse wheel
//
// The initial version was developed with assistance from OpenAI ChatGPT.
// Subsequently adapted and tested for the Digispark ATtiny85.
// ------------------------------------------------------------

#define ENCODER_A 2       // PB2 / INT0
#define ENCODER_B 0       // PB0

// Encoder counts required for one mouse-wheel step.
//
// 1200 counts/revolution with the present 2x decoder.
// Start with 4.
// Lower value = more sensitive.
// Higher value = less sensitive.
#define COUNTS_PER_SCROLL 4

volatile int16_t encoderCount = 0;


// ------------------------------------------------------------
// Encoder interrupt
//
// Triggered on BOTH rising and falling edges of A.
//
// Looking at B tells us the direction.
//
// If direction is reversed, simply change the +1/-1 below.
// ------------------------------------------------------------

ISR(INT0_vect)
{
  if (PINB & _BV(PB2)) {
    // A is HIGH
    if (PINB & _BV(PB0))
      encoderCount--;
    else
      encoderCount++;
  }
  else {
    // A is LOW
    if (PINB & _BV(PB0))
      encoderCount++;
    else
      encoderCount--;
  }
}


// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup()
{
  // Encoder inputs.
  //
  // We use external 4.7k pull-up resistors because the encoder
  // has NPN open-collector outputs.
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);

  // Configure INT0 for any logical change.
  //
  // On ATtiny85:
  //   ISC01 = 0
  //   ISC00 = 1
  //
  // -> interrupt on any logical change.
  MCUCR &= ~_BV(ISC01);
  MCUCR |=  _BV(ISC00);

  // Clear a possible pending INT0 interrupt.
  GIFR |= _BV(INTF0);

  // Enable INT0.
  GIMSK |= _BV(INT0);

  // Start Digispark USB mouse.
  DigiMouse.begin();
}


// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------

void loop()
{
  int16_t count;

  // Take a snapshot of the encoder count.
  noInterrupts();

  count = encoderCount;

  // Remove only the amount we're going to process.
  if (count >= COUNTS_PER_SCROLL) {
    encoderCount -= COUNTS_PER_SCROLL;
    count = COUNTS_PER_SCROLL;
  }
  else if (count <= -COUNTS_PER_SCROLL) {
    encoderCount += COUNTS_PER_SCROLL;
    count = -COUNTS_PER_SCROLL;
  }
  else {
    count = 0;
  }

  interrupts();


  // Generate mouse-wheel event.
  if (count > 0) {
    DigiMouse.scroll(1);
  }
  else if (count < 0) {
    DigiMouse.scroll(-1);
  }

  // Keep the Digispark USB connection serviced.
  DigiMouse.update();

  // 5 ms gives USB plenty of attention while allowing
  // the encoder interrupt to operate continuously.
  DigiMouse.delay(5);
}

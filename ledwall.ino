#include <Adafruit_NeoPixel.h>

#define LED_PIN 6

#define TRIG_ONE 9
#define ECHO_ONE 10

#define TRIG_TWO 7
#define ECHO_TWO 8



#define NUM_LEDS 50

//led segments

Adafruit_NeoPixel strip(
    NUM_LEDS,
    LED_PIN,
    NEO_GRB + NEO_KHZ800
  );
struct Segment {
  int start;
  int end;
  };
  Segment segmentOne = {0,29};
  Segment segmentTwo = {30, 59};
 

// Sonic sensors

long getDistance(int trigPin, int echoPin){

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);

    if (duration == 0){
      return -1;
      }
      return duration * 0.0343 / 2;
  }

  void setSegment(Segment segment, uint32_t colour){
      for (int i = segment.start; i <= segment.end; i++) {
          strip.setPixelColor(i, colour);
          strip.show();
          delay(20);
        }
    }
void setup() {
  // put your setup code here, to run once:

  pinMode(TRIG_ONE, OUTPUT);
  pinMode(ECHO_ONE, INPUT);

  pinMode(TRIG_TWO, OUTPUT);
  pinMode(ECHO_TWO, INPUT);

  strip.begin();
  strip.clear();

  strip.show();

  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  long distanceOne = getDistance(TRIG_ONE, ECHO_ONE);

  delay(30);

  long distanceTwo = getDistance(TRIG_TWO, ECHO_TWO);

  Serial.print("Sensor one: ");
  Serial.print(distanceOne);
  Serial.println(" cm");

  Serial.print("Sensor two: ");
  Serial.print(distanceTwo);
  Serial.println(" cm");

  if (distanceOne < 20) {
    setSegment(segmentOne, strip.Color(0,255,0));
    delay(1000);
    if(distanceTwo< 20){
      setSegment(segmentTwo, strip.Color(0,255,0));
      delay(100);
      }
    }



}
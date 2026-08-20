#include <FastLED.h>

#define LED_PIN 6

#define NUM_LEDS 36

unsigned long lastStateChange;

CRGB colour = (255, 0, 0);

CRGB leds[NUM_LEDS];

struct Segment {
  int start;
  int end;
  int brightness;
  bool state;
};

struct Sensor {
  int trig;
  int echo;
  int sensorSeg[2];
};

const int sensorAmount = 2;
Sensor sensors[sensorAmount] = {
  {9,10,{0,1}},
  {7,8,{2,3}}
};

const int segmentAmount = 2;
Segment segments[segmentAmount] = {
  {0, 17, 0, false},
  {18, 35, 0, false}
};

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

void fillSegment(Segment segment, CRGB colour) {
    for (int i = segment.start; i <= segment.end; i++) {
        leds[i] = colour;
        leds[i].nscale8(segment.brightness);
    }
}

void changeStates(){
  if(millis() - 30 < lastStateChange){
    return;
  }

  for(int i = 0; i<segmentAmount; i++){
    if(segments[i].state && segments[i].brightness < 255){
      segments[i].brightness + 5;
      fillSegment(segments[i], colour);
    }
    if(!segments[i].state && segments[i].brightness > 0){
      segments[i].brightness - 5;
      fillSegment(segments[i], colour);
    }
  }
  lastStateChange = millis();
}

void setup() {
  pinMode(sensors[0].trig, OUTPUT);
  pinMode(sensors[0].echo, INPUT);

  pinMode(sensors[1].trig, OUTPUT);
  pinMode(sensors[1].echo, INPUT);
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  
  FastLED.clear();
  FastLED.show();

  Serial.begin(9600);
}

void loop() {
  changeStates();

  for(int i = 0; i<sensorAmount; i++){
    if (getDistance(sensors[i].trig, sensors[i].echo) < 20){
        segments[i].state = true;
    }
  }

  delay(10);
}
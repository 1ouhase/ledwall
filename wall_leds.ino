
#include <FastLED.h>


#define LED_PIN 6

#define TRIG_ONE 9
#define ECHO_ONE 10

#define TRIG_TWO 7
#define ECHO_TWO 8


#define NUM_LEDS 36


//led segments

CRGB leds[NUM_LEDS];

struct Segment {
  int start;
  int end;
  };
  Segment segmentOne = {0,17};
  Segment segmentTwo = {18, 35};
 

// Sonic sensors
//Fuctions
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


void setSegmentColour(Segment segment, CRGB colour) {
    for (int i = segment.start; i <= segment.end; i++) {
        leds[i] = colour;
    }
}


void setSegmentBrightness(Segment segment, CRGB colour, uint8_t brightness) {
    for (int i = segment.start; i <= segment.end; i++) {
        leds[i] = colour;
        leds[i].nscale8(brightness);
    }
}
void pulseSegments(Segment one, Segment two) {

    int brightnessOne = 0;
    int brightnessTwo = 0;

    // --------------------------------
    // Phase 1:
    // Segment 1 goes from 0 -> 50%
    // --------------------------------

    while (brightnessOne < 128) {

        brightnessOne += 5;

        setSegmentBrightness(one, CRGB(255, 0, 0), brightnessOne);
        setSegmentBrightness(two, CRGB(255, 0, 0), 0);

        FastLED.show();
        delay(30);
    }


    // --------------------------------
    // Phase 2:
    // Segment 2 goes 0 -> 50%
    // Segment 1 stays at 50%
    // --------------------------------

    while (brightnessTwo < 128) {

        brightnessTwo += 5;

        setSegmentBrightness(one, CRGB(255, 0, 0), 128);
        setSegmentBrightness(two, CRGB(255, 0, 0), brightnessTwo);

        FastLED.show();
        delay(30);
    }


    // --------------------------------
    // Phase 3:
    // Segment 2 continues up
    // Segment 1 goes down
    // --------------------------------

    while (brightnessTwo < 255) {

        brightnessTwo += 5;
        brightnessOne -= 5;

        if (brightnessTwo > 255)
            brightnessTwo = 255;

        if (brightnessOne < 0)
            brightnessOne = 0;

        setSegmentBrightness(one, CRGB(255, 0, 0), brightnessOne);
        setSegmentBrightness(two, CRGB(255, 0, 0), brightnessTwo);

        FastLED.show();
        delay(30);
    }
    while (brightnessTwo > 0) {
      brightnessTwo -= 5;
      if (brightnessTwo < 0)
        brightnessTwo = 0;
      
      setSegmentBrightness(two, CRGB(255,0,0), brightnessTwo);
      
      FastLED.show();
      delay(30);
    }
}
void setup() {
  // put your setup code here, to run once:

  pinMode(TRIG_ONE, OUTPUT);
  pinMode(ECHO_ONE, INPUT);

  pinMode(TRIG_TWO, OUTPUT);
  pinMode(ECHO_TWO, INPUT);
  

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  
  FastLED.clear();
  FastLED.show();

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
   
  int direction = 0;
  
  if (distanceOne > 0 && distanceOne < 20) {
    direction = 1;
  } 
  else if (distanceTwo > 0 && distanceTwo < 20) {
    direction = 2;
  } 
  else {
    direction = 0;
  }
    //1 = left to right 2 = right to left
  switch(direction) {
    case 1:
      setSegmentColour(segmentOne, CRGB(255,0,0));
      setSegmentColour(segmentTwo, CRGB(255,0,0));
      pulseSegments(segmentOne, segmentTwo);
      break;
    case 2:
      setSegmentColour(segmentOne, CRGB(255,0,0));
      setSegmentColour(segmentTwo, CRGB(255,0,0));
      pulseSegments(segmentTwo, segmentOne);
      break;
    default:
    FastLED.clear();
    FastLED.show();
    break;
  }
  //if (distanceOne < 20) {
    //setSegment(segmentOne, CRGB(0,255,0));
    //pulseSegment(segmentOne, CRGB(0,255,0));  
  //}

  //if (distanceTwo < 20) {
    //setSegment(segmentTwo, CRGB(0,255,0));
  //}
}


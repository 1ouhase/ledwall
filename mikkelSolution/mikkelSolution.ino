#include <FastLED.h>

 

#define LED_PIN 9

#define NUM_LEDS 192

#define NUM_SEGMENTS 32

#define NUM_ZONES 6

#define NUM_SENSORS 6

#define MAX_SEGS_PER_ZONE 8

 
CRGB colour = CRGB(255, 0, 0);

CRGB leds[NUM_LEDS];

 
struct Segment {

  int start;

  int end;

  uint8_t brightness;

  bool state;

};

 
struct Zone {

  int segIndex[MAX_SEGS_PER_ZONE];

  int segCount;

};

 
struct Sensor {

  int trig;

  int echo;

  int zone;

};

 
Segment segments[NUM_SEGMENTS] = {

  {0,5,0,false},

  {6,11,0,false},

  {12,17,0,false},

  {18,23,0,false},

  {24,29,0,false},

  {30,35,0,false},

  {36,41,0,false},

  {42,47,0,false},

  {48,53,0,false},

  {54,59,0,false},

  {60,65,0,false},

  {66,71,0,false},

  {72,77,0,false},

  {78,83,0,false},

  {84,89,0,false},

  {90,95,0,false},

  {96,101,0,false},

  {102,107,0,false},

  {108,113,0,false},

  {114,119,0,false},

  {120,125,0,false},

  {126,131,0,false},

  {132,137,0,false},

  {138,143,0,false},

  {144,149,0,false},

  {150,155,0,false},

  {156,161,0,false},

  {162,167,0,false},

  {168,173,0,false},

  {174,179,0,false},

  {180,185,0,false},

  {186,191,0,false},

};

 
Zone zones[NUM_ZONES] = {

  {{20, 21, 22, 19}, 4},

  {{ 15, 17, 18}, 3},

  {{ 12, 13, 14, 16, 23}, 5},

  {{11,25, 24, 9, 10, 8}, 6},

  {{6, 27, 30, 31, 26, 7}, 6},

  {{0, 1, 2, 3, 4, 5, 28, 29}, 8},

};

 
Sensor sensors[NUM_SENSORS] = {

  {2,  3,  0}, // Sensor 1

  {4,  5,  1}, // Sensor 2

  {6,  7,  2}, // Sensor 3

  {8, 10,  3}, // Sensor 4

  {11, 12, 4}, // Sensor 5

  {14, 15, 5}, // Sensor 6

};

 

// --------------------------------------------------

// Sensor-indstillinger

// --------------------------------------------------

 

// Vi læser sensorer, som fysisk ligger længere fra hinanden,

// efter hinanden for at reducere risikoen for crosstalk.

//

// Sensor 1 -> 4 -> 2 -> 5 -> 3 -> 6

const int sensorOrder[NUM_SENSORS] = {

  0, 3, 1, 4, 2, 5

};

 

int currentSensorIndex = 0;

 

unsigned long lastSensorRead = 0;

 

// Snorre oplever crosstalk under ca. 20 ms.

// Derfor starter vi med 20 ms.

const unsigned long sensorInterval = 15;

 

// --------------------------------------------------

// Registrering

// --------------------------------------------------

 

const int detectionDistance = 110;

 

// Gem tidspunktet hvor hver zone sidst så en person.

unsigned long lastDetected[NUM_ZONES] = {0};

 

// Lad zonen blive aktiv lidt efter sidste registrering.

// Det gør systemet mindre følsomt overfor enkelte fejlaflæsninger.

const unsigned long zoneHoldTime = 300;

 

// --------------------------------------------------

// LED-animation

// --------------------------------------------------

 

unsigned long lastFadeUpdate = 0;

const unsigned long fadeInterval = 15;

 

// Hurtigere oplysning end i den oprindelige kode.

const uint8_t fadeInStep = 25;

 

// Langsommere nedtoning.

const uint8_t fadeOutStep = 6;

 

// --------------------------------------------------

// Afstandsmåling

// --------------------------------------------------

 

long getDistance(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);

  delayMicroseconds(2);

 

  digitalWrite(trigPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

 

  /*

     Den oprindelige kode brugte 30000 µs.

 

     Vi er kun interesseret i ca. 110 cm,

     så der er ingen grund til at vente 30 ms.

 

     9000 µs giver stadig god margin.

 

  */

  long duration = pulseIn(echoPin, HIGH, 10000);

 

  if (duration == 0) {

    return -1;

  }



  return duration * 0.0343 / 2;

}

 

// --------------------------------------------------

// LED-segment

// --------------------------------------------------

 

void fillSegment(Segment &segment) {

 

  uint8_t eased = ease8InOutQuad(segment.brightness);

 

  for (int i = segment.start; i <= segment.end; i++) {

    leds[i] = colour;

    leds[i].nscale8(eased);

  }

}

 

// --------------------------------------------------

// Opdater LED brightness

// --------------------------------------------------

 

void changeStates() {

 

  bool anyChanged = false;

 

  for (int i = 0; i < NUM_SEGMENTS; i++) {

 

    uint8_t oldBrightness = segments[i].brightness;

 

    if (segments[i].state) {

 

      // qadd8 sørger for at vi aldrig kommer over 255.

      segments[i].brightness =

        qadd8(segments[i].brightness, fadeInStep);

 

    } else {

 

      // qsub8 sørger for at vi aldrig kommer under 0.

      segments[i].brightness =

        qsub8(segments[i].brightness, fadeOutStep);

    }

 

    if (oldBrightness != segments[i].brightness) {

 

      fillSegment(segments[i]);

      anyChanged = true;

    }

  }

 

  if (anyChanged) {

    FastLED.show();

  }

}

 

// --------------------------------------------------

// Sæt alle segmenter i en zone

// --------------------------------------------------

 

void setZoneState(int zoneNumber, bool state) {

 

  Zone &z = zones[zoneNumber];

 

  for (int i = 0; i < z.segCount; i++) {

    segments[z.segIndex[i]].state = state;

  }

}

 

// --------------------------------------------------

// Læs næste sensor

// --------------------------------------------------

 

void readNextSensor() {

 

  // Find næste sensor ud fra vores specielle rækkefølge.

  int sensorNumber = sensorOrder[currentSensorIndex];

 

  Sensor &s = sensors[sensorNumber];

 

  long distance = getDistance(s.trig, s.echo);

 

  Serial.print("Sensor ");

  Serial.print(sensorNumber + 1);

  Serial.print(": ");

  Serial.println(distance);

 

  // Gyldig registrering.

  if (distance > 0 && distance < detectionDistance) {

 

    lastDetected[s.zone] = millis();

 

   setZoneState(s.zone, true);

  }

 

 

  // Gå videre til næste sensor.

  currentSensorIndex++;

 

  if (currentSensorIndex >= NUM_SENSORS) {

    currentSensorIndex = 0;

  }

}

 

// --------------------------------------------------

// Sluk zoner der ikke længere registrerer personer

// --------------------------------------------------

 

void updateZones() {

 

  unsigned long now = millis();

 

  for (int zone = 0; zone < NUM_ZONES; zone++) {

 

    if (lastDetected[zone] == 0) {

      continue;

    }

 

    if (now - lastDetected[zone] > zoneHoldTime) {

 

      setZoneState(zone, false);

 

      lastDetected[zone] = 0;

    }

  }

}

 

// --------------------------------------------------

// Setup

// --------------------------------------------------

 

void setup() {

 

  for (int i = 0; i < NUM_SENSORS; i++) {

 

    pinMode(sensors[i].trig, OUTPUT);

    pinMode(sensors[i].echo, INPUT);

 

    digitalWrite(sensors[i].trig, LOW);

  }

 

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(

    leds,

    NUM_LEDS

  );

 

  FastLED.clear();

  FastLED.show();

 

  // Den oprindelige brugte 9600.

  // 115200 reducerer risikoen for at Serial-output

  // bliver endnu en flaskehals.

  Serial.begin(115200);

}

 

// --------------------------------------------------

// Loop

// --------------------------------------------------

 

void loop() {

 

  unsigned long now = millis();

 

  // Læs én sensor ad gangen.

  if (now - lastSensorRead >= sensorInterval) {

 

    lastSensorRead = now;

 

    readNextSensor();

  }

 

  // Kontroller om zoner skal slukkes.

  updateZones();

 

  // LED-animationen kører uafhængigt af sensorerne.

  if (now - lastFadeUpdate >= fadeInterval) {

 

    lastFadeUpdate = now;

 

    changeStates();

  }

}

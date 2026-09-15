#include <FastLED.h>

#define LED_PIN 9
#define NUM_LEDS 192          // 32 segments x 6 LEDs
#define NUM_SEGMENTS 32
#define NUM_ZONES 6
#define NUM_SENSORS 6
#define MAX_SEGS_PER_ZONE 8   // largest zone (Zone 6) has 8

CRGB colour = CRGB(255, 0, 0);
CRGB leds[NUM_LEDS];

struct Segment {
  int start;
  int end;
  uint8_t brightness; // linear tracker, 0-255
  bool state;
};

struct Zone {
  int segIndex[MAX_SEGS_PER_ZONE]; // indices into segments[]
  int segCount;
};

struct Sensor {
  int trig;
  int echo;
  int zone;
};

Segment segments[NUM_SEGMENTS] = {
  {0,5,0,false},        // Segment 1
  {6,11,0,false},       // Segment 2
  {12,17,0,false},      // Segment 3
  {18,23,0,false},      // Segment 4
  {24,29,0,false},      // Segment 5
  {30,35,0,false},      // Segment 6
  {36,41,0,false},      // Segment 7
  {42,47,0,false},      // Segment 8
  {48,53,0,false},      // Segment 9
  {54,59,0,false},      // Segment 10
  {60,65,0,false},      // Segment 11
  {66,71,0,false},      // Segment 12
  {72,77,0,false},      // Segment 13
  {78,83,0,false},      // Segment 14
  {84,89,0,false},      // Segment 15
  {90,95,0,false},      // Segment 16
  {96,101,0,false},     // Segment 17
  {102,107,0,false},    // Segment 18
  {108,113,0,false},    // Segment 19
  {114,119,0,false},    // Segment 20
  {120,125,0,false},    // Segment 21
  {126,131,0,false},    // Segment 22
  {132,137,0,false},    // Segment 23
  {138,143,0,false},    // Segment 24
  {144,149,0,false},    // Segment 25
  {150,155,0,false},    // Segment 26
  {156,161,0,false},    // Segment 27
  {162,167,0,false},    // Segment 28
  {168,173,0,false},    // Segment 29
  {174,179,0,false},    // Segment 30
  {180,185,0,false},    // Segment 31
  {186,191,0,false},    // Segment 32
};

Zone zones[NUM_ZONES] = {
  { {20, 21, 22, 15}, 4 },       // Zone 1
  { {19, 14, 16, 17, 18}, 5 },   // Zone 2
  { {11, 12, 13, 23}, 4 },       // Zone 3
  { {25, 26, 9, 10, 8}, 5 },     // Zone 4
  { {24, 27, 30, 31, 6, 7}, 6 }, // Zone 5
  { {0, 1, 2, 3, 4, 5, 28, 29}, 8 }, // Zone 6
};

Sensor sensors[NUM_SENSORS] = {
  {2,  3,  0}, // Sensor 1 -> Zone 1
  {4,  5,  1}, // Sensor 2 -> Zone 2
  {6,  7,  2}, // Sensor 3 -> Zone 3
  {8,  10, 3}, // Sensor 4 -> Zone 4
  {11, 12, 4}, // Sensor 5 -> Zone 5
  {14, 15, 5}, // Sensor 6 -> Zone 6
};

// --- non-blocking timing state ---
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 50; // ms between sensor reads (crosstalk spacing)
int currentSensor = 0;

unsigned long lastFadeUpdate = 0;
const unsigned long fadeInterval = 15; // ms between fade steps

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) {
    return -1;
  }
  return duration * 0.0343 / 2;
}

void fillSegment(Segment &segment) {
  uint8_t eased = ease8InOutQuad(segment.brightness);
  for (int i = segment.start; i <= segment.end; i++) {
    leds[i] = colour;
    leds[i].nscale8(eased);
  }
}

void changeStates() {
  bool anyChanged = false;

  for (int i = 0; i < NUM_SEGMENTS; i++) {
    uint8_t brightness = segments[i].brightness;
    bool changed = false;

    if (segments[i].state && brightness < 255) {
      segments[i].brightness = (brightness + 15 > 255) ? 255 : brightness + 10;
      changed = true;
    }
    if (!segments[i].state && brightness > 0) {
      segments[i].brightness = (brightness < 2) ? 0 : brightness - 2;
      changed = true;
    }

    if (changed) {
      fillSegment(segments[i]);
      anyChanged = true;
    }
  }

  if (anyChanged) {
    FastLED.show();
  }
}

void readNextSensor() {
  Sensor s = sensors[currentSensor];
  long distance = getDistance(s.trig, s.echo);

  Serial.print(currentSensor);
  Serial.print(": ");
  Serial.println(distance);

  if (distance >= 0) {
    bool occupied = (distance < 110);
    Zone &z = zones[s.zone];
    for (int i = 0; i < z.segCount; i++) {
      segments[z.segIndex[i]].state = occupied;
    }
  }

  currentSensor = (currentSensor + 1) % NUM_SENSORS;
}

void setup() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensors[i].trig, OUTPUT);
    pinMode(sensors[i].echo, INPUT);
  }

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();

  // Poll one sensor at a time, spaced out to avoid crosstalk
  if (now - lastSensorRead >= sensorInterval) {
    lastSensorRead = now;
    readNextSensor();
  }

  // Update LED fade independently of sensor timing
  if (now - lastFadeUpdate >= fadeInterval) {
    lastFadeUpdate = now;
    changeStates();
  }
}

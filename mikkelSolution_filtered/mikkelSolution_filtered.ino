#include <FastLED.h>

// This is a non-destructive filtered variant of mikkelSolution.ino
// Approach: per-sensor 3-sample median filter + short per-sensor neighbor cooldown

#define LED_PIN 9
#define NUM_LEDS 192
#define NUM_SEGMENTS 32
#define NUM_ZONES 6
#define NUM_SENSORS 6
#define MAX_SEGS_PER_ZONE 8

CRGB colour = CRGB(255, 0, 0);
CRGB leds[NUM_LEDS];

struct Segment { int start; int end; uint8_t brightness; bool state; };
struct Zone { int segIndex[MAX_SEGS_PER_ZONE]; int segCount; };
struct Sensor { int trig; int echo; int zone; };

// --- copy segments/zones/sensors from original (kept identical) ---
Segment segments[NUM_SEGMENTS] = {
  {0,5,0,false},{6,11,0,false},{12,17,0,false},{18,23,0,false},
  {24,29,0,false},{30,35,0,false},{36,41,0,false},{42,47,0,false},
  {48,53,0,false},{54,59,0,false},{60,65,0,false},{66,71,0,false},
  {72,77,0,false},{78,83,0,false},{84,89,0,false},{90,95,0,false},
  {96,101,0,false},{102,107,0,false},{108,113,0,false},{114,119,0,false},
  {120,125,0,false},{126,131,0,false},{132,137,0,false},{138,143,0,false},
  {144,149,0,false},{150,155,0,false},{156,161,0,false},{162,167,0,false},
  {168,173,0,false},{174,179,0,false},{180,185,0,false},{186,191,0,false},
};

Zone zones[NUM_ZONES] = {
  {{20,21,22,19},4},{{15,17,18},3},{{12,13,14,16,23},5},
  {{11,25,24,9,10,8},6},{{6,27,30,31,26,7},6},{{0,1,2,3,4,5,28,29},8}
};

Sensor sensors[NUM_SENSORS] = {
  {2,3,0},{4,5,1},{6,7,2},{8,10,3},{11,12,4},{14,15,5}
};

const int sensorOrder[NUM_SENSORS] = { 0, 3, 1, 4, 2, 5 };
int currentSensorIndex = 0;
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 15; // keep fast

// Filtering structures (non-destructive file)
int filtSamples[NUM_SENSORS][3];
int filtIndex[NUM_SENSORS];
unsigned long sensorCooldown[NUM_SENSORS];
const unsigned long neighborCooldown = 40; // ms

const int detectionDistance = 110;
unsigned long lastDetected[NUM_ZONES] = {0};
const unsigned long zoneHoldTime = 300;

unsigned long lastFadeUpdate = 0;
const unsigned long fadeInterval = 15;
const uint8_t fadeInStep = 25;
const uint8_t fadeOutStep = 6;

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 10000);
  if (duration == 0) return -1;
  return duration * 0.0343 / 2;
}

void fillSegment(Segment &segment) {
  uint8_t eased = ease8InOutQuad(segment.brightness);
  for (int i = segment.start; i <= segment.end; i++) { leds[i] = colour; leds[i].nscale8(eased); }
}

void changeStates() {
  bool anyChanged = false;
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    uint8_t oldBrightness = segments[i].brightness;
    if (segments[i].state) segments[i].brightness = qadd8(segments[i].brightness, fadeInStep);
    else segments[i].brightness = qsub8(segments[i].brightness, fadeOutStep);
    if (oldBrightness != segments[i].brightness) { fillSegment(segments[i]); anyChanged = true; }
  }
  if (anyChanged) FastLED.show();
}

void setZoneState(int zoneNumber, bool state) {
  Zone &z = zones[zoneNumber];
  for (int i = 0; i < z.segCount; i++) segments[z.segIndex[i]].state = state;
}

// Small helper: median of 3 ints
int median3(int a, int b, int c) {
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

void readNextSensor() {
  int sensorNumber = sensorOrder[currentSensorIndex];
  Sensor &s = sensors[sensorNumber];
  unsigned long now = millis();

  // If in cooldown, skip physical read and record -1
  if (now - sensorCooldown[sensorNumber] < neighborCooldown) {
    filtSamples[sensorNumber][filtIndex[sensorNumber]] = -1;
    filtIndex[sensorNumber] = (filtIndex[sensorNumber] + 1) % 3;
  } else {
    long distance = getDistance(s.trig, s.echo);
    filtSamples[sensorNumber][filtIndex[sensorNumber]] = (int)distance;
    filtIndex[sensorNumber] = (filtIndex[sensorNumber] + 1) % 3;

    // If this read is a detection, set short cooldown on neighboring sensors in the read order
    if (distance > 0 && distance < detectionDistance) {
      int pos = -1;
      for (int j = 0; j < NUM_SENSORS; j++) if (sensorOrder[j] == sensorNumber) { pos = j; break; }
      if (pos != -1) {
        int prev = (pos - 1 + NUM_SENSORS) % NUM_SENSORS;
        int next = (pos + 1) % NUM_SENSORS;
        sensorCooldown[sensorOrder[prev]] = now;
        sensorCooldown[sensorOrder[next]] = now;
      }
    }
  }

  int v0 = filtSamples[sensorNumber][0];
  int v1 = filtSamples[sensorNumber][1];
  int v2 = filtSamples[sensorNumber][2];
  int med = median3(v0, v1, v2);

  Serial.print("Filtered Sensor "); Serial.print(sensorNumber + 1);
  Serial.print(" median="); Serial.print(med);
  Serial.print(" samples=["); Serial.print(v0); Serial.print(","); Serial.print(v1); Serial.print(","); Serial.print(v2); Serial.print("]\n");

  if (med > 0 && med < detectionDistance) {
    lastDetected[s.zone] = now;
    setZoneState(s.zone, true);
  }

  currentSensorIndex++;
  if (currentSensorIndex >= NUM_SENSORS) currentSensorIndex = 0;
}

void updateZones() {
  unsigned long now = millis();
  for (int zone = 0; zone < NUM_ZONES; zone++) {
    if (lastDetected[zone] == 0) continue;
    if (now - lastDetected[zone] > zoneHoldTime) { setZoneState(zone, false); lastDetected[zone] = 0; }
  }
}

void setup() {
  for (int i = 0; i < NUM_SENSORS; i++) { pinMode(sensors[i].trig, OUTPUT); pinMode(sensors[i].echo, INPUT); digitalWrite(sensors[i].trig, LOW); }
  for (int i = 0; i < NUM_SENSORS; i++) { filtIndex[i] = 0; for (int j = 0; j < 3; j++) filtSamples[i][j] = -1; sensorCooldown[i] = 0; }
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS); FastLED.clear(); FastLED.show(); Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSensorRead >= sensorInterval) { lastSensorRead = now; readNextSensor(); }
  updateZones();
  if (now - lastFadeUpdate >= fadeInterval) { lastFadeUpdate = now; changeStates(); }
}

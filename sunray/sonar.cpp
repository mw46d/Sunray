// Ardumower Sunray
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)


#include "config.h"
#include "sonar.h"
#include "robot.h"
#include "RunningMedian.h"
#include <Arduino.h>


#define MAX_DURATION 4000
#define ROUNDING_ENABLED false
#define US_ROUNDTRIP_CM 57      // Microseconds (uS) it takes sound to travel round-trip 1cm (2cm total), uses integer to save compiled code space. Default=57

// Conversion from uS to distance (round result to nearest cm or inch).
#define NewPingConvert(echoTime, conversionFactor) (max(((unsigned int)echoTime + conversionFactor / 2) / conversionFactor, (echoTime ? 1 : 0)))

RunningMedian<unsigned int, 9> sonarLeftMeasurements;
RunningMedian<unsigned int, 9> sonarRightMeasurements;
RunningMedian<unsigned int, 9> sonarCenterMeasurements;

volatile unsigned long startTime = 0;
volatile unsigned long echoTime = 0;
volatile unsigned long echoDuration = 0;
volatile byte sonarIdx = 0;
bool added = false;
unsigned long timeoutTime = 0;
unsigned long nextEvalTime = 0;


#ifdef SONAR_INSTALLED

// HC-SR04 ultrasonic sensor driver (2cm - 400cm)
void startHCSR04(int triggerPin, int echoPin) {
  unsigned int uS;
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  /*// if there is no reflection, we will get 0  (NO_ECHO)
  uS = pulseIn(echoPin, HIGH, MAX_ECHO_TIME);
  //if (uS == MAX_ECHO_TIME) uS = NO_ECHO;
  //if (uS < MIN_ECHO_TIME) uS = NO_ECHO;
  return uS;*/
}

void echoHandler(unsigned char pin) {
  if (digitalRead(pin) == HIGH) {
    startTime = micros();
    echoTime = 0;
  } else {
    echoTime = micros();
    echoDuration = echoTime - startTime;
  }
}

void echoLeft() {
  if (sonarIdx != 0) return;
  echoHandler(pinSonarLeftEcho);
}

void echoCenter() {
  if (sonarIdx != 1) return;
  echoHandler(pinSonarCenterEcho);
}

void echoRight() {
  if (sonarIdx != 2) return;
  echoHandler(pinSonarRightEcho);
}

#endif


void Sonar::run() {
#ifdef SONAR_INSTALLED  
  if (!enabled) {
    distanceRight = distanceLeft = distanceCenter = 0;
    return;
  }
  if (echoDuration != 0) {
    added = true;
    unsigned long raw = echoDuration;
    if (raw > MAX_DURATION) raw = MAX_DURATION;
    if (sonarIdx == 0) sonarLeftMeasurements.add(raw);
    else if (sonarIdx == 1) sonarCenterMeasurements.add(raw);
    else sonarRightMeasurements.add(raw);
    echoDuration = 0;
  }
  if (millis() > timeoutTime) {
    if (!added) {
      if (sonarIdx == 0) sonarLeftMeasurements.add(MAX_DURATION);
      else if (sonarIdx == 1) sonarCenterMeasurements.add(MAX_DURATION);
      else sonarRightMeasurements.add(MAX_DURATION);
    }
    sonarIdx = (sonarIdx + 1) % 3;
    echoDuration = 0;
    if (sonarIdx == 0) startHCSR04(pinSonarLeftTrigger, pinSonarLeftEcho);
    else if (sonarIdx == 1) startHCSR04(pinSonarCenterTrigger, pinSonarCenterEcho);
    else startHCSR04(pinSonarRightTrigger, pinSonarRightEcho);
    timeoutTime = millis() + 50;           // 10
    added = false;
  }
  if (millis() > nextEvalTime) {
    nextEvalTime = millis() + 200;
    sonarLeftMeasurements.getMedian(distanceLeft);
    distanceLeft = convertCm(distanceLeft);
    sonarRightMeasurements.getMedian(distanceRight);
    distanceRight = convertCm(distanceRight);
    sonarCenterMeasurements.getMedian(distanceCenter);
    distanceCenter = convertCm(distanceCenter);
  }
#endif
}

void Sonar::begin()
{
#ifdef SONAR_INSTALLED
  enabled = SONAR_ENABLE;
  triggerLeftBelow = SONAR_LEFT_OBSTACLE_CM;
  triggerCenterBelow = SONAR_CENTER_OBSTACLE_CM;
  triggerRightBelow = SONAR_RIGHT_OBSTACLE_CM;
  pinMode(pinSonarLeftTrigger, OUTPUT);
  pinMode(pinSonarCenterTrigger, OUTPUT);
  pinMode(pinSonarRightTrigger, OUTPUT);

  pinMode(pinSonarLeftEcho, INPUT);
  pinMode(pinSonarCenterEcho, INPUT);
  pinMode(pinSonarRightEcho, INPUT);

  attachInterrupt(pinSonarLeftEcho, echoLeft, CHANGE);
  attachInterrupt(pinSonarCenterEcho, echoCenter, CHANGE);
  attachInterrupt(pinSonarRightEcho, echoRight, CHANGE);

  //pinMan.setDebounce(pinSonarCenterEcho, 100);  // reject spikes shorter than usecs on pin
  //pinMan.setDebounce(pinSonarRightEcho, 100);  // reject spikes shorter than usecs on pin
  //pinMan.setDebounce(pinSonarLeftEcho, 100);  // reject spikes shorter than usecs on pin
  nearObstacleTimeout = 0;
#endif
}


bool Sonar::obstacle()
{
#ifdef SONAR_INSTALLED
  if (!enabled) return false;
  bool res = ((distanceLeft < triggerLeftBelow) || (distanceCenter < triggerCenterBelow) || (distanceRight < triggerRightBelow));
  if (res) {
    CONSOLE.print("Sonar::obstacle() sensor true  ");
    if (distanceLeft < triggerLeftBelow) {
      CONSOLE.print("  distanceLeft= "); CONSOLE.print(distanceLeft);
    }
    if (distanceCenter < triggerCenterBelow) {
      CONSOLE.print("  distanceCenter= "); CONSOLE.print(distanceCenter);
    }
    if (distanceRight < triggerRightBelow) {
      CONSOLE.print("  distanceRight= "); CONSOLE.print(distanceRight);
    }
    CONSOLE.println("");
  }
  return res;
#else
  return false;
#endif
}

bool Sonar::nearObstacle()
{
#ifdef SONAR_INSTALLED
  if (!enabled) return false;
  int nearZone = 10; // marco 30; // cm
  if ((nearObstacleTimeout != 0) && (millis() < nearObstacleTimeout)) {
    CONSOLE.println("Sonar::nearObstacle() time true");
    return true;
  }
  nearObstacleTimeout = 0;
  bool res = ((distanceLeft < triggerLeftBelow + nearZone) || (distanceCenter < triggerCenterBelow + nearZone) || (distanceRight < triggerRightBelow + nearZone));
  if (res) {
    nearObstacleTimeout = millis() + 5000;
    CONSOLE.print("Sonar::nearObstacle() sensor true  ");
    if (distanceLeft < triggerLeftBelow + nearZone) {
      CONSOLE.print("  distanceLeft= "); CONSOLE.print(distanceLeft);
    }
    if (distanceCenter < triggerCenterBelow + nearZone) {
      CONSOLE.print("  distanceCenter= "); CONSOLE.print(distanceCenter);
    }
    if (distanceRight < triggerRightBelow + nearZone) {
      CONSOLE.print("  distanceRight= "); CONSOLE.print(distanceRight);
    }
    CONSOLE.println("");
  }
  return res;
#else
  return false;
#endif
}

unsigned int Sonar::convertCm(unsigned int echoTime) {
#if ROUNDING_ENABLED == false
  return (echoTime / US_ROUNDTRIP_CM);              // Convert uS to centimeters (no rounding).
#else
  return NewPingConvert(echoTime, US_ROUNDTRIP_CM); // Convert uS to centimeters.
#endif
}

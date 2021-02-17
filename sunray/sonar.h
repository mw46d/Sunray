// Ardumower Sunray 
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)

// HC-SR04 ultrasonic sensor driver (2cm - 400cm)
// for 3 sensors, optimized for speed: based on hardware interrupts (no polling)
// up to 100 Hz measurements tested

#ifndef SONAR_H
#define SONAR_H

class Sonar {
    public:
        unsigned int distanceLeft; // cm
        unsigned int distanceRight;
        unsigned int distanceCenter;
        bool enabled;

        void begin();
        void run();
        bool obstacle();
        bool nearObstacle();

    protected:

    private:
        unsigned int triggerLeftBelow;
        unsigned int triggerCenterBelow;
        unsigned int triggerRightBelow;
        unsigned long nearObstacleTimeout;

        unsigned int convertCm(unsigned int echoTime);
};

#endif

#include "sleepTimer.h"

sleepTimer::sleepTimer() {    
    // ESP.rtcUserMemoryWrite(64 + sizeof(uint32_t), &UINT32_MAX, sizeof(uint32_t));
}

void sleepTimer::setSleepTime(uint64_t timeToSleep) {
    this->convertMusToHrAndRemainingMus(timeToSleep,
                                        this -> updatedSleepTimeInHr,
                                        this -> updatedSleepTimeInMus);

    this->writeTimeRec();
}

void sleepTimer::startSleeping() {
    this -> readTimeRec(); // Filling both updatedSleepTimeInMus and updatedSleepTimeInHr
    // if (updatedSleepTimeInMus == UINT32_MAX) { //First Call
        // Doing nothing...
    //    return
    // }
    uint32_t init;
    ESP.rtcUserMemoryRead(64 + 2 * sizeof(uint32_t), &init, sizeof(uint32_t));
    Serial.println(init);
    if (init != 321) { // Fist call after power ON
        init = 321;
        ESP.rtcUserMemoryWrite(64 + 2 * sizeof(uint32_t), &init, sizeof(uint32_t));
        return;
    }

    if (updatedSleepTimeInHr > 0) {
        updatedSleepTimeInHr -= 1;
        this -> writeTimeRec();
        ESP.deepSleep(3600 * 1000 * 1000);
    } else if (updatedSleepTimeInMus > 0){
        uint32_t temp_updatedSleepTimeInMus = updatedSleepTimeInMus;
        updatedSleepTimeInMus = 0;
        this -> writeTimeRec();
        init = 123;
        ESP.rtcUserMemoryWrite(64 + 2 * sizeof(uint32_t), &init, sizeof(uint32_t));
        Serial.println(temp_updatedSleepTimeInMus);
        ESP.deepSleep(temp_updatedSleepTimeInMus);
    }
}

// Helper Functions

void sleepTimer::convertMusToHrAndRemainingMus(uint64_t timeInMus, uint32_t &timeInHr, uint32_t &timeInRemainingMus){
    timeInHr = timeInMus / (3600 * 1000 * 1000); // extract Hr
    timeInRemainingMus = timeInMus % (3600 * 1000 * 1000);
}


void sleepTimer::writeTimeRec() {
    Serial.println("hour");
    Serial.println(updatedSleepTimeInHr);
    Serial.println("rest");
    Serial.println(updatedSleepTimeInMus);
    ESP.rtcUserMemoryWrite(64, &this->updatedSleepTimeInHr, sizeof(uint32_t));
    ESP.rtcUserMemoryWrite(64 + sizeof(uint32_t), &this->updatedSleepTimeInMus, sizeof(uint32_t));
}

void sleepTimer::readTimeRec() {
    ESP.rtcUserMemoryRead(64, &this->updatedSleepTimeInHr, sizeof(uint32_t));
    ESP.rtcUserMemoryRead(64 + sizeof(uint32_t), &this->updatedSleepTimeInMus, sizeof(uint32_t));
}

#include "sleepTimer.h"

sleepTimer::sleepTimer() {    
    // ESP.rtcUserMemoryWrite(64 + sizeof(uint32_t), &UINT32_MAX, sizeof(uint32_t));
}

void sleepTimer::setSleepTime(uint64_t timeToSleep) {
    this->convertMusToHrAndRemainingMus(timeToSleep,
                                        this -> updatedSleepTimeInHr,
                                        this -> updatedSleepTimeInUs);

    this->writeTimeRec();

void sleepTimer::startSleeping() {
    this -> readTimeRec(); // Filling both updatedSleepTimeInMus and updatedSleepTimeInHr
    // if (updatedSleepTimeInMus == UINT32_MAX) { //First Call
        // Doing nothing...
    //    return
    // }
    uint32_t init;
    ESP.rtcUserMemoryRead(64 + 2 * sizeof(uint32_t), &init, sizeof(uint32_t));
    if (init != 123) { // Fist call after power ON
        ESP.rtcUserMemoryWrite(64 + 2 * sizeof(uint32_t), 123, sizeof(uint32_t));
        return;
    }

    if (updatedSleepTimeInHr > 0) {
        updatedSleepTimeInHr -= 1;
        this -> writeTimeRec();
        ESP.deepSleep(3600 * 1000 * 1000);
    } else {
        uint32_t temp_updatedSleepTimeInMus = updatedSleepTimeInMus;
        updatedSleepTimeInMus = 0;
        this -> writeTimeRec();
        ESP.deepSleep(temp_updatedSleepTimeInMus);
    }
}

// Helper Functions

void sleepTimer::convertMusToHrAndRemainingMus(uint64_t timeInMus, uint32_t &timeInHr, uint32_t &timeInRemainingMus){
    timeInHr = timeInMus / (3600 * 100 * 100); // extract Hr
    timeInRemainingMus = timeInMus % (3600 * 100 * 100);
}


void sleepTimer::writeTimeRec() {
    ESP.rtcUserMemoryWrite(64, &this->updatedSleepTimeInHr, sizeof(uint32_t));
    ESP.rtcUserMemoryWrite(64 + sizeof(uint32_t), &this->updatedSleepTimeInMus, sizeof(uint32_t));
}

void sleepTimer::readTimeRec() {
    ESP.rtcUserMemoryRead(64, &this->updatedSleepTimeInHr, sizeof(uint32_t));
    ESP.rtcUserMemoryRead(64 + sizeof(uint32_t), &this->updatedSleepTimeInMus, sizeof(uint32_t));
}
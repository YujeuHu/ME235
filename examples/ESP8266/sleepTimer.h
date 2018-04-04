#include "Arduino.h"

extern "C" {
  #include "user_interface.h"
}

class sleepTimer {
    public:
    sleepTimer();
    void setSleepTime(uint64_t timeToSleep);
    void startSleeping();

    private:
    void writeTimeRec(); // convert and write time to RTC Memery in unit of us
    void readTimeRec(); // read time record from RTC Memery in units of us
    void convertMusToHrAndRemainingMus(uint64_t timeInMus, uint32_t &timeInHr, uint32_t &timeInRemainingMus);
    uint32_t updatedSleepTimeInMus; // the portion of sleep time less than an hour in mu s
    uint32_t updatedSleepTimeInHr; // the portion of sleep time in hours
    // bool sleepTimeLessThan1Hr;
};
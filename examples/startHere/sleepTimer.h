
extern "C" {
  #include "user_interface.h"
}

class sleepTimer {
    public:
    sleepTimer();
    void sleep();
    unit32_t result;
    // void setSleepTime(uint64_t timeToSleep);
    // void startSleeping();
    private:
    // void writeTimeRec(uint64_t timeToWrite); // convert and write time to RTC Memery in unit of us
    // void readTimeRec(unit64_t &timeInMem); // read time record from RTC Memery in units of us
    // uint32_t updatedSleepTimeInUs; // the portion of sleep time less than an hour
    // uint32_t updatedSleepTimeInHr; // the portion of sleep time in hours
    // bool sleepTimeLessThan1Hr;
}
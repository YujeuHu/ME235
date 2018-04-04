#include "sleepTimer.h"

sleepTimer::sleepTimer() {
    uint32_t content = 12;
    
    ESP.rtcUserMemoryWrite(64, &content, sizeof(uint32_t));
    ESP.rtcUserMemoryRead(64, &result, sizeof(uint32_t));
}

void sleepTimer::sleepTest() {
    ESP.deepSleep(5*1000*1000);
}

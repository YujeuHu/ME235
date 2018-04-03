sleepTimer::sleepTimer() {
    unit32_t content = 12;
    
    ESP.rtcUserMemoryWrite(64, &content, sizeof(uint32_t));
    ESP.rtcUserMemoryRead(64, &result, sizeof(uint32_t));
}

void sleepTimer::sleep() {
    ESP.deepSleep(5*1000*1000);
}
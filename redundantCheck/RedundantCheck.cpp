#include "RedundantCheck.h"

RedundantChecker() {
    this.history[0] = 0;
}

bool reset() {
    int cunt = sizeof(history) / sizeof(history[0]);
    for (int i = 0; i < cunt; i++){
        this.history[i] = 0;
    }    
    return this.history[0] == 0;
}

bool check(String msg) {
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);

    if (this.history[0] == 0) { //First call
        history[0] = JSONRestored["DeviceID"];
        return false;
    } else { // History contains sth
        int cunt = sizeof(history) / sizeof(history[0]);
        for (int i = 0; i < cunt; i++) {
            if (history[i] == JSONRestored["DeviceID"]) {
                return true;
            }
        }
        return false;
    }
}
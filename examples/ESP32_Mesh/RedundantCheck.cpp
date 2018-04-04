#include "RedundantCheck.h"

RedundantChecker::RedundantChecker() {
    history[0] = 0;
    numOfRecord = 0;
}

bool RedundantChecker::reset() {
    int cunt = sizeof(history) / sizeof(history[0]);
    for (int i = 0; i < cunt; i++){
        history[i] = 0;
    }    
    numOfRecord = 0;
    return history[0] == 0;
}

bool RedundantChecker::check(String msg) {
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);

    if (history[0] == 0) { //First call
        history[0] = JSONRestored["DeviceID"];
        //Serial.println("First Call");
        return false;
    } else { // History contains sth
        int cunt = sizeof(history) / sizeof(history[0]);
        for (int i = 0; i < cunt; i++) { // treaverse the entire array to find same ID
            if (history[i] == JSONRestored["DeviceID"]) { // Redundant ID found
                return true;
            }
        }
        numOfRecord ++; // No Redundancy Found
        history[numOfRecord] = JSONRestored["DeviceID"];
        return false;
    }

bool RedundantChecker::check(String msg, unit16_t &lengthOfMsg) {
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);
    lengthOfMsg = msg.length();
    if (history[0] == 0) { //First call
        history[0] = JSONRestored["DeviceID"];
        //Serial.println("First Call");
        return false;
    } else { // History contains sth
        int cunt = sizeof(history) / sizeof(history[0]);
        for (int i = 0; i < cunt; i++) { // treaverse the entire array to find same ID
            if (history[i] == JSONRestored["DeviceID"]) { // Redundant ID found
                return true;
            }
        }
        numOfRecord ++; // No Redundancy Found
        history[numOfRecord] = JSONRestored["DeviceID"];
        return false;
    }
}

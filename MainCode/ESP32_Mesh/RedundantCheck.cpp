#include "RedundantCheck.h"

RedundantChecker::RedundantChecker() {
    history[0][0] = 0;
    numOfRecord = 0;
}

bool RedundantChecker::reset() {
    int cunt = sizeof(history) / (sizeof(history[0][0]) * 2);
    for (int i = 0; i < cunt; i++){
        history[i][0] = 0;
        history[i][1] = 0;
    }    
    numOfRecord = 0;
    return history[0][0] == 0;
}

bool RedundantChecker::reset(String msg) {
    int cunt = sizeof(history) / (sizeof(history[0][0]) * 2);
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);
    for (int i = 0; i < cunt; i++){
        if (history[i][0] == JSONRestored["DeviceID"]) {
            history[i][0] = 0;
            history[i][1] = 0;
            return true;
        }
    }
    return false;
}

bool RedundantChecker::check(String msg) {
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);

    if (history[0][0] == 0) { //First call
        history[0][0] = JSONRestored["DeviceID"];
        //Serial.println("First Call");
        return false;
    } else { // History contains sth
        int cunt = sizeof(history) / (sizeof(history[0][0]) * 2);
        for (int i = 0; i < cunt; i++) { // treaverse the entire array to find same ID
            if (history[i][0] == JSONRestored["DeviceID"]) { // Redundant ID found
                return true;
            }
        }
        numOfRecord ++; // No Redundancy Found
        history[numOfRecord][0] = JSONRestored["DeviceID"];
        return false;
    }
}

bool RedundantChecker::check(String msg, uint16_t &prevLength) {
    DynamicJsonBuffer restoreJsonBuffer;
    JsonObject& JSONRestored = restoreJsonBuffer.parseObject(msg);
    //lengthOfMsg = msg.length();
    if (history[0][0] == 0) { //First call
        history[0][0] = JSONRestored["DeviceID"];
        history[0][1] = msg.length();
        prevLength = 0;
        //Serial.println("First Call");
        return false;
    } else { // History contains sth
        int cunt = sizeof(history) / (sizeof(history[0][0]) * 2);
        for (int i = 0; i < cunt; i++) { // treaverse the entire array to find same ID
            if (history[i][0] == JSONRestored["DeviceID"]) { // Redundant ID found
                prevLength = history[i][1];
                return true;
            }
        }
        numOfRecord ++; // No Redundancy Found
        history[numOfRecord][0] = JSONRestored["DeviceID"];
        history[numOfRecord][1] = msg.length();
        prevLength = 0;
        return false;
    }
}

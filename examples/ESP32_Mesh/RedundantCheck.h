#include <ArduinoJson.h>

#define MAX_NUM_OF_NODE 5
class RedundantChecker {
    public:
    RedundantChecker();
    bool check(String msg);
    // bool check(String msg, unit16_t &lengthOfMsg);
    bool reset();

    private:
    int history[MAX_NUM_OF_NODE][2];
    int numOfRecord;
};

#include <ArduinoJson.h>


class RedundantChecker {
    public:
    RedundantChecker();
    bool check(String msg);
    bool check(String msg, unit16_t &lengthOfMsg);
    bool reset();

    private:
    int history[5];
    int numOfRecord;
};

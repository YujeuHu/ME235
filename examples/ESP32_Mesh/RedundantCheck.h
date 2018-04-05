#include <ArduinoJson.h>

#define MAX_NUM_OF_CLIENT 5
class RedundantChecker {
    public:
    RedundantChecker();
    bool check(String msg);
    bool check(String msg, uint16_t &prevLength);
    bool reset();
    bool reset(String msg);

    private:
    int history[MAX_NUM_OF_CLIENT][2];

    int numOfRecord;
};

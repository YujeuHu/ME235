#include <ArduinoJson.h>


class RedundantChecker {
    public:
    RedundantChecker();
    bool check(String msg);
    bool reset();

    private:
    int history[5];
    int numOfRecord;
};

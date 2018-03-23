#include <ArduinoJson.h>
#include "RedundantCheck.h"

RedundantChecker checker = RedundantChecker();

void setup() {
  bool isRedundant = checker.check( /* Some JSON String*/);
  checker.reset();
  
}


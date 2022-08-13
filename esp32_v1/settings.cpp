#include <string>
#include "settings.hpp"

using namespace std;

// Stored as ints ONLY
unordered_map<string, int> settings = {
  { "version", 1 },
  { "debounce", 50 },
  { "inputDelay", 50 },
  // Output ON time
  { "relayZero", 1000 },
  { "relayOne", 1000 },
  { "relayTwo", 1000 },
  { "relayThree", 1000 },
  { "relayFour", 1000 },
  { "relayFive", 1000 },
  { "transZero", 1000 },
  { "transOne", 1000 },
  // Timer delay time
  { "timerZero", 1000 },
  { "timerOne", 1000 },
  { "timerTwo", 1000 },
  { "timerThree", 1000 },
  { "timerFour", 1000 },
};

//// other current values, not saved
//unordered_map<string, float> currentValues = {
//  { "debounceTime", 3 },
//};

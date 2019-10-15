#include "Clock.hpp"
#include <chrono>


uint32_t Clock::getTime() {
   auto t1 = std::chrono::system_clock::now();
   std::time_t t2 = std::chrono::system_clock::to_time_t(t1);
   std::tm *t = std::localtime(&t2);

   int weekday = t->tm_wday == 0 ? 6 : t->tm_wday - 1;

   return time(t->tm_hour, t->tm_min, t->tm_sec) | weekday << WEEKDAY_SHIFT;
}

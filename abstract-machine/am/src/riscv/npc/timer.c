#include <am.h>
#include "../riscv.h"

#define TIMER_LO_ADDR 0x20000000
#define TIMER_HI_ADDR 0x20000004

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t lo = inl(TIMER_LO_ADDR);
  uint32_t hi = inl(TIMER_HI_ADDR);
  uptime->us = ((uint64_t)hi << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

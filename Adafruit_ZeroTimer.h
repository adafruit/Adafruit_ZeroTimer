#include "Arduino.h"

#if defined(__SAMD21G18A__)
  #define SAMD21G
  #include "samd21/include/samd21g18a.h"
#elif defined(__SAMD21E18A__)
  #define SAMD21E
  #include "samd21/include/samd21e18a.h"
#endif

#include "tc.h"
#include "tc_interrupt.h"

class Adafruit_ZeroTimer {
 public:
  Adafruit_ZeroTimer(uint8_t tn);

  boolean PWMout(boolean pwmout, uint8_t channum, uint8_t pin);
  void setPeriodMatch(uint32_t period, uint32_t match, uint8_t channum = 1);
  void enable(boolean en);

  void configure(tc_clock_prescaler prescale, tc_counter_size countersize, tc_wave_generation wavegen, tc_count_direction countdir = TC_COUNT_DIRECTION_UP);
  void setCompare(uint8_t channum, uint32_t compare);
  void setTop(uint32_t top);
  void invertWave(tc_waveform_invert_output inv);
  void setCallback(boolean enable, tc_callback cb_type, tc_callback_t callback_func = NULL);

 protected:
  uint8_t _timernum;
  tc_counter_size _countersize;

  struct tc_config config_tc;
  struct tc_module tc_instance;
};

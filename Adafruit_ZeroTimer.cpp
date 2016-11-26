#include <Adafruit_ZeroTimer.h>

// mostly from asfdoc_sam0_tc_basic_use_case.html

Adafruit_ZeroTimer::Adafruit_ZeroTimer(uint8_t timernum) {
  _timernum = timernum;
}


boolean Adafruit_ZeroTimer::PWMout(boolean pwmout, uint8_t channum, uint8_t pin) {
  Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;

  if (channum > 1) return false; // we only support pwm #0 or #1

  if (! pwmout) {
    config_tc.pwm_channel[channum].enabled = false;
  } else {
    uint32_t pinout = 0xFFFF;
    uint32_t pinmux = 0xFFFF;

    if (_timernum == 3) {
      if (channum == 0) {
	if (pin == 10) {
	  pinout = PIN_PA18E_TC3_WO0; pinmux = MUX_PA18E_TC3_WO0;
	}
	if (pin == 2) {
	  pinout = PIN_PA14E_TC3_WO0; pinmux = MUX_PA14E_TC3_WO0;
	}
      } 
      if (channum == 1) {
	if (pin == 12) {
	  pinout = PIN_PA19E_TC3_WO1; pinmux = MUX_PA19E_TC3_WO1;
	}
	if (pin == 5) {
	  pinout = PIN_PA15E_TC3_WO1; pinmux = MUX_PA15E_TC3_WO1;
	}
      } 
    }

    if (_timernum == 4) {
      if (channum == 0) {
	if (pin == 20) { // a.k.a SDA
	  pinout = PIN_PA22E_TC4_WO0; pinmux = MUX_PA22E_TC4_WO0;
	}
#if defined(__SAMD21G18A__)
	if (pin == A1) {
	  pinout = PIN_PB08E_TC4_WO0; pinmux = MUX_PB08E_TC4_WO0;
	}
#endif
      } 
      if (channum == 1) {
	if (pin == 21) {  // a.k.a SCL
	  pinout = PIN_PA23E_TC4_WO1; pinmux = MUX_PA23E_TC4_WO1;
	}
#if defined(__SAMD21G18A__)
	if (pin == A2) {
	  pinout = PIN_PB09E_TC4_WO1; pinmux = MUX_PB09E_TC4_WO1;
	}
#endif
      } 
    }

    if (_timernum == 5) {
      if (channum == 0) {
#if defined(__SAMD21G18A__)
	if (pin == MOSI) {
	  pinout = PIN_PB10E_TC5_WO0; pinmux = MUX_PB10E_TC5_WO0;
	}
#endif
	// only other option is D-, skip it!
      } 
      if (channum == 1) {
#if defined(__SAMD21G18A__)
	if (pin == SCK) {
	  pinout = PIN_PB11E_TC5_WO1; pinmux = MUX_PB11E_TC5_WO1;
	}
#endif
	// only other option is D+, skip it!
      }
    }



    if (pinout == 0xFFFF) 
      return false;

    config_tc.pwm_channel[channum].enabled = true;
    config_tc.pwm_channel[channum].pin_out = pinout;
    config_tc.pwm_channel[channum].pin_mux = pinmux;
  }

  // re-init
  tc_init(&tc_instance, tc_modules[_timernum - TC_INSTANCE_OFFSET], &config_tc);
  return true;
}

void Adafruit_ZeroTimer::invertWave(tc_waveform_invert_output invert) {
  Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;

  config_tc.waveform_invert_output = invert;
  tc_init(&tc_instance, tc_modules[_timernum - TC_INSTANCE_OFFSET], &config_tc);
}

void Adafruit_ZeroTimer::configure(tc_clock_prescaler prescale, tc_counter_size countersize, tc_wave_generation wavegen, tc_count_direction countdir)
{
  if (_timernum > TC_INST_MAX_ID) return;
  Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;

  _countersize = countersize;

  tc_get_config_defaults(&config_tc);
  if (countersize == TC_COUNTER_SIZE_8BIT) {
    config_tc.counter_8_bit.period = 0xFF;
  }

  config_tc.clock_prescaler = prescale;
  config_tc.counter_size    = countersize;
  config_tc.wave_generation = wavegen;
  config_tc.count_direction = countdir;

  // initialize
  tc_init(&tc_instance, tc_modules[_timernum - TC_INSTANCE_OFFSET], &config_tc);
}

void Adafruit_ZeroTimer::setPeriodMatch(uint32_t period, uint32_t match, uint8_t channum) {
  if (_countersize == TC_COUNTER_SIZE_8BIT) {
    config_tc.counter_8_bit.period = period;
    tc_set_top_value(&tc_instance, period);
    setCompare(channum, match);
  }
  else if (_countersize == TC_COUNTER_SIZE_16BIT) {
    setCompare(0, period);
    setCompare(1, match);
  }
  else if (_countersize == TC_COUNTER_SIZE_32BIT) {
    setCompare(0, period);
    setCompare(1, match);
  }
}


void Adafruit_ZeroTimer::setCallback(boolean enable, tc_callback cb_type, tc_callback_t callback_func) {
  if (enable) {
    tc_register_callback(&tc_instance, callback_func, cb_type);
    tc_enable_callback(&tc_instance, cb_type);
  } else {
    tc_disable_callback(&tc_instance, cb_type);
  }
}

void Adafruit_ZeroTimer::setCompare(uint8_t channum, uint32_t compare) {
  if (channum > 1) return; 

  // set the configuration 
  if (_countersize == TC_COUNTER_SIZE_8BIT) {
    config_tc.counter_8_bit.compare_capture_channel[channum] = compare;
  }
  else if (_countersize == TC_COUNTER_SIZE_16BIT) {
    config_tc.counter_16_bit.compare_capture_channel[channum] = compare;
  }
  else if (_countersize == TC_COUNTER_SIZE_32BIT) {
    config_tc.counter_32_bit.compare_capture_channel[channum] = compare;
  }

  // set it live
  tc_set_compare_value(&tc_instance, (tc_compare_capture_channel)channum, compare);  
}

void Adafruit_ZeroTimer::enable(boolean en) {
  if (en) {
    tc_enable(&tc_instance);
  } else {
    tc_disable(&tc_instance);
  }
}

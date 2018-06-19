#include <Adafruit_ZeroTimer.h>
#include "wiring_private.h"

// mostly from asfdoc_sam0_tc_basic_use_case.html

#ifndef TC_INST_MAX_ID
#define TC_INST_MAX_ID 5
#endif

#ifndef TC_INSTANCE_OFFSET
#if defined(__SAMD51__)
#define TC_INSTANCE_OFFSET 3
#else
#define TC_INSTANCE_OFFSET 3
#endif
#endif

#ifndef TC_ASYNC
#define TC_ASYNC true
#endif

static inline bool tc_is_syncing(Tc *const hw)
{
  return (hw->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY);
}

Adafruit_ZeroTimer::Adafruit_ZeroTimer(uint8_t timernum)
{
  _timernum = timernum;
}

bool Adafruit_ZeroTimer::tc_init()
{
  /* Temporary variable to hold all updates to the CTRLA
	 * register before they are written to it */
  uint16_t ctrla_tmp = 0;
  /* Temporary variable to hold all updates to the CTRLBSET
	 * register before they are written to it */
  uint8_t ctrlbset_tmp = 0;
  /* Temporary variable to hold all updates to the CTRLC
	 * register before they are written to it */
  uint8_t ctrlc_tmp = 0;
  /* Temporary variable to hold TC instance number */
  uint8_t instance = _timernum - TC_INSTANCE_OFFSET;

  /* Array of GLCK ID for different TC instances */
  uint32_t inst_gclk_id[] = {GCLK_CLKCTRL_ID(GCM_TCC2_TC3), GCLK_CLKCTRL_ID(GCM_TC4_TC5), GCLK_CLKCTRL_ID(GCM_TC4_TC5)};
  /* Array of PM APBC mask bit position for different TC instances */
  uint32_t inst_pm_apbmask[] = {PM_APBCMASK_TC3, PM_APBCMASK_TC4, PM_APBCMASK_TC5};

#if TC_ASYNC == true
  /* Initialize parameters */
  for (uint8_t i = 0; i < TC_CALLBACK_N; i++)
  {
    _callback[i] = NULL;
  }
  _register_callback_mask = 0x00;
  _enable_callback_mask = 0x00;
#endif

  /* Check if odd numbered TC modules are being configured in 32-bit
	 * counter size. Only even numbered counters are allowed to be
	 * configured in 32-bit counter size.
	 */
  if ((_counter_size == TC_COUNTER_SIZE_32BIT) &&
      ((instance + TC_INSTANCE_OFFSET) & 0x01))
  {
    return false;
  }

  if (_hw->COUNT8.CTRLA.reg & TC_CTRLA_SWRST)
  {
    /* We are in the middle of a reset. Abort. */
    return false;
  }

  if (_hw->COUNT8.STATUS.reg & TC_STATUS_SLAVE)
  {
    /* Module is used as a slave */
    return false;
  }

  if (_hw->COUNT8.CTRLA.reg & TC_CTRLA_ENABLE)
  {
    /* Module must be disabled before initialization. Abort. */
    return false;
  }

  /* Set up the TC PWM out pin for channel 0 */
  if (_pwm_channel[0].enabled)
  {
    pinPeripheral(_pwm_channel[0].pin_out, (EPioType)_pwm_channel[0].pin_mux);
  }

  /* Set up the TC PWM out pin for channel 1 */
  if (_pwm_channel[1].enabled)
  {
    pinPeripheral(_pwm_channel[1].pin_out, (EPioType)_pwm_channel[1].pin_mux);
  }

  /* Enable the user interface clock in the PM */
  PM->APBCMASK.reg |= inst_pm_apbmask[instance];

  /* Enable the slave counter if counter_size is 32-bit */
  if ((_counter_size == TC_COUNTER_SIZE_32BIT))
  {
    /* Enable the user interface clock in the PM */
    PM->APBCMASK.reg |= inst_pm_apbmask[instance + 1];
  }

  /* Setup clock for module */
  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | inst_gclk_id[instance]);
  while (GCLK->STATUS.bit.SYNCBUSY == 1);

  /* Set ctrla register */
  ctrla_tmp =
      (uint32_t)_counter_size |
      (uint32_t)_wave_generation |
      (uint32_t)_clock_prescaler;

  /* Write configuration to register */
  while (tc_is_syncing(_hw));
  _hw->COUNT8.CTRLA.reg = ctrla_tmp;

  if (_count_direction)
  {
    ctrlbset_tmp |= TC_CTRLBSET_DIR;
  }

  /* Clear old ctrlb configuration */
  while (tc_is_syncing(_hw));
  _hw->COUNT8.CTRLBCLR.reg = 0xFF;

  /* Check if we actually need to go into a wait state. */
  if (ctrlbset_tmp)
  {
    while (tc_is_syncing(_hw));
    /* Write configuration to register */
    _hw->COUNT8.CTRLBSET.reg = ctrlbset_tmp;
  }

  /* Set ctrlc register*/
  ctrlc_tmp = _waveform_invert_output;

  /* Write configuration to register */
  while (tc_is_syncing(_hw));
  _hw->COUNT8.CTRLC.reg = ctrlc_tmp;

  /* Write configuration to register */
  while (tc_is_syncing(_hw));

  /* Switch for TC counter size  */
  switch (_counter_size)
  {
  case TC_COUNTER_SIZE_8BIT:
    while (tc_is_syncing(_hw));

    _hw->COUNT8.COUNT.reg =
        _counter_8_bit.value;

    while (tc_is_syncing(_hw));

    _hw->COUNT8.PER.reg =
        _counter_8_bit.period;

    while (tc_is_syncing(_hw));

    _hw->COUNT8.CC[0].reg =
        _counter_8_bit.compare_capture_channel[0];

    while (tc_is_syncing(_hw));

    _hw->COUNT8.CC[1].reg =
        _counter_8_bit.compare_capture_channel[1];

    return true;

  case TC_COUNTER_SIZE_16BIT:
    while (tc_is_syncing(_hw));

    _hw->COUNT16.COUNT.reg = _counter_16_bit.value;

    while (tc_is_syncing(_hw));

    _hw->COUNT16.CC[0].reg =
        _counter_16_bit.compare_capture_channel[0];

    while (tc_is_syncing(_hw));

    _hw->COUNT16.CC[1].reg =
        _counter_16_bit.compare_capture_channel[1];

    return true;

  case TC_COUNTER_SIZE_32BIT:
    while (tc_is_syncing(_hw));

    _hw->COUNT32.COUNT.reg = _counter_32_bit.value;

    while (tc_is_syncing(_hw));

    _hw->COUNT32.CC[0].reg =
        _counter_32_bit.compare_capture_channel[0];

    while (tc_is_syncing(_hw));

    _hw->COUNT32.CC[1].reg =
        _counter_32_bit.compare_capture_channel[1];

    return true;
  }

  return false;
}

boolean Adafruit_ZeroTimer::PWMout(boolean pwmout, uint8_t channum, uint8_t pin)
{
  Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;

  if (channum > 1)
    return false; // we only support pwm #0 or #1

  if (!pwmout)
  {
    _pwm_channel[channum].enabled = false;
  }
  else
  {
    uint32_t pinout = 0xFFFF;
    uint32_t pinmux = 0xFFFF;

    if (_timernum == 3)
    {
      if (channum == 0)
      {
        if (pin == 10)
        {
          pinout = PIN_PA18E_TC3_WO0;
          pinmux = MUX_PA18E_TC3_WO0;
        }
        if (pin == 2)
        {
          pinout = PIN_PA14E_TC3_WO0;
          pinmux = MUX_PA14E_TC3_WO0;
        }
      }
      if (channum == 1)
      {
        if (pin == 12)
        {
          pinout = PIN_PA19E_TC3_WO1;
          pinmux = MUX_PA19E_TC3_WO1;
        }
        if (pin == 5)
        {
          pinout = PIN_PA15E_TC3_WO1;
          pinmux = MUX_PA15E_TC3_WO1;
        }
      }
    }

    if (_timernum == 4)
    {
      if (channum == 0)
      {
        if (pin == 20)
        { // a.k.a SDA
          pinout = PIN_PA22E_TC4_WO0;
          pinmux = MUX_PA22E_TC4_WO0;
        }
#if defined(__SAMD21G18A__)
        if (pin == A1)
        {
          pinout = PIN_PB08E_TC4_WO0;
          pinmux = MUX_PB08E_TC4_WO0;
        }
#endif
      }
      if (channum == 1)
      {
        if (pin == 21)
        { // a.k.a SCL
          pinout = PIN_PA23E_TC4_WO1;
          pinmux = MUX_PA23E_TC4_WO1;
        }
#if defined(__SAMD21G18A__)
        if (pin == A2)
        {
          pinout = PIN_PB09E_TC4_WO1;
          pinmux = MUX_PB09E_TC4_WO1;
        }
#endif
      }
    }

    if (_timernum == 5)
    {
      if (channum == 0)
      {
#if defined(__SAMD21G18A__)
        if (pin == MOSI)
        {
          pinout = PIN_PB10E_TC5_WO0;
          pinmux = MUX_PB10E_TC5_WO0;
        }
#endif
        // only other option is D-, skip it!
      }
      if (channum == 1)
      {
#if defined(__SAMD21G18A__)
        if (pin == SCK)
        {
          pinout = PIN_PB11E_TC5_WO1;
          pinmux = MUX_PB11E_TC5_WO1;
        }
#endif
        // only other option is D+, skip it!
      }
    }

    if (pinout == 0xFFFF)
      return false;

    _pwm_channel[channum].enabled = true;
    _pwm_channel[channum].pin_out = pin;
    _pwm_channel[channum].pin_mux = pinmux;
  }

  // re-init
  _hw = tc_modules[_timernum - TC_INSTANCE_OFFSET];
  return tc_init();
}

void Adafruit_ZeroTimer::invertWave(uint8_t invert)
{
  _waveform_invert_output = invert;

  tc_init();
}

void Adafruit_ZeroTimer::configure(tc_clock_prescaler prescale, tc_counter_size countersize, tc_wave_generation wavegen, tc_count_direction countdir)
{
  Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;

  if (_timernum > TC_INST_MAX_ID)
    return;

  _waveform_invert_output = false;

  _pwm_channel[0].enabled = false;
  _pwm_channel[0].pin_out = 0;
  _pwm_channel[0].pin_mux = 0;

  _pwm_channel[1].enabled = false;
  _pwm_channel[1].pin_out = 0;
  _pwm_channel[1].pin_mux = 0;

  _counter_16_bit.value = 0x0000;
  _counter_16_bit.compare_capture_channel[0] = 0x0000;
  _counter_16_bit.compare_capture_channel[1] = 0x0000;

  if (countersize == TC_COUNTER_SIZE_8BIT)
  {
    _counter_8_bit.period = 0xFF;
  }

  _clock_prescaler = prescale;
  _counter_size = countersize;
  _wave_generation = wavegen;
  _count_direction = countdir;

  _hw = tc_modules[_timernum - TC_INSTANCE_OFFSET];
  tc_init();
}

void Adafruit_ZeroTimer::setPeriodMatch(uint32_t period, uint32_t match, uint8_t channum)
{
  if (_counter_size == TC_COUNTER_SIZE_8BIT)
  {
    while (tc_is_syncing(_hw))
      ;
    _counter_8_bit.period = period;
    _hw->COUNT8.PER.reg = (uint8_t)period;
    setCompare(channum, match);
  }
  else if (_counter_size == TC_COUNTER_SIZE_16BIT)
  {
    setCompare(0, period);
    setCompare(1, match);
  }
  else if (_counter_size == TC_COUNTER_SIZE_32BIT)
  {
    setCompare(0, period);
    setCompare(1, match);
  }
}

void Adafruit_ZeroTimer::setCallback(boolean enable, tc_callback cb_type, void (*callback_func)(void))
{
  if (enable)
  {

    /* Register callback function */
    _callback[cb_type] = callback_func;

    /* Set the bit corresponding to the callback_type */
    if (cb_type == TC_CALLBACK_CC_CHANNEL0)
    {
      _register_callback_mask |= TC_INTFLAG_MC(1);
    }
    else if (cb_type == TC_CALLBACK_CC_CHANNEL1)
    {
      _register_callback_mask |= TC_INTFLAG_MC(2);
    }
    else
    {
      _register_callback_mask |= (1 << cb_type);
    }

    //TODO: enable interrupt

    /* Enable callback */
    if (cb_type == TC_CALLBACK_CC_CHANNEL0)
    {
      _enable_callback_mask |= TC_INTFLAG_MC(1);
      _hw->COUNT8.INTENSET.reg = TC_INTFLAG_MC(1);
    }
    else if (cb_type == TC_CALLBACK_CC_CHANNEL1)
    {
      _enable_callback_mask |= TC_INTFLAG_MC(2);
      _hw->COUNT8.INTENSET.reg = TC_INTFLAG_MC(2);
    }
    else
    {
      _enable_callback_mask |= (1 << cb_type);
      _hw->COUNT8.INTENSET.reg = (1 << cb_type);
    }
  }
  else
  {

    /* Disable callback */
    if (cb_type == TC_CALLBACK_CC_CHANNEL0)
    {
      _hw->COUNT8.INTENCLR.reg = TC_INTFLAG_MC(1);
      _enable_callback_mask &= ~TC_INTFLAG_MC(1);
    }
    else if (cb_type == TC_CALLBACK_CC_CHANNEL1)
    {
      _hw->COUNT8.INTENCLR.reg = TC_INTFLAG_MC(2);
      _enable_callback_mask &= ~TC_INTFLAG_MC(2);
    }
    else
    {
      _hw->COUNT8.INTENCLR.reg = (1 << cb_type);
      _enable_callback_mask &= ~(1 << cb_type);
    }
  }
}

void Adafruit_ZeroTimer::setCompare(uint8_t channum, uint32_t compare)
{
  if (channum > 1)
    return;

  // set the configuration
  if (_counter_size == TC_COUNTER_SIZE_8BIT)
  {
    _counter_8_bit.compare_capture_channel[channum] = compare;
  }
  else if (_counter_size == TC_COUNTER_SIZE_16BIT)
  {
    _counter_16_bit.compare_capture_channel[channum] = compare;
  }
  else if (_counter_size == TC_COUNTER_SIZE_32BIT)
  {
    _counter_32_bit.compare_capture_channel[channum] = compare;
  }

  // set it live
  while (tc_is_syncing(_hw))
  {
    /* Wait for sync */
  }

  /* Read out based on the TC counter size */
  switch (_counter_size)
  {
  case TC_COUNTER_SIZE_8BIT:
    _hw->COUNT8.CC[channum].reg = (uint8_t)compare;
    break;

  case TC_COUNTER_SIZE_16BIT:
    _hw->COUNT16.CC[channum].reg = (uint16_t)compare;
    break;

  case TC_COUNTER_SIZE_32BIT:
    _hw->COUNT32.CC[channum].reg = (uint32_t)compare;
    break;
  }
}

void Adafruit_ZeroTimer::enable(boolean en)
{
  if (en)
  {
    while (tc_is_syncing(_hw))
      ;

    /* Enable TC module */
    _hw->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE;
  }
  else
  {
    while (tc_is_syncing(_hw))
      ;
    /* Disbale interrupt */
    _hw->COUNT8.INTENCLR.reg = TC_INTENCLR_MASK;
    /* Clear interrupt flag */
    _hw->COUNT8.INTFLAG.reg = TC_INTFLAG_MASK;

    /* Disable TC module */
    _hw->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  }
}

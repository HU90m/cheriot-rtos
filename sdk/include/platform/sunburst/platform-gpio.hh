#pragma once
#include <cdefs.h>
#include <stdint.h>
#include <utils.hh>

/**
 * A simple driver for the Sonata's GPIO. This struct represents a single
 * GPIO instance, and the methods available to interact with that GPIO.
 *
 * GPIO instances can be implemented via subclass structs which optionally
 * override the three static mask attributes and/or add their own instance-
 * specific functionality. The subclass itself should be provided as a
 * template class parameter when declaring inheritance.
 *
 * Documentation source can be found at:
 * https://github.com/lowRISC/sonata-system/blob/9f794fe3bd4eec8d1a01ee81da97a7f2cec0b452/doc/ip/gpio.md
 *
 * Rendered documentation is served from:
 * https://lowrisc.org/sonata-system/doc/ip/gpio.html
 */
template<uint32_t OutputMask = 0xffff'ffff, uint32_t InputMask = 0xffff'ffff>
struct SonataGpio : private utils::NoCopyNoMove
{
	uint32_t output;
	uint32_t input;
	uint32_t debouncedInput;
	uint32_t outputEnable;

	/**
	 * @returns the bit corresponding to a given GPIO index. Will mask out
	 * the bit if this is outside of the provided mask.
	 */
	inline constexpr static uint32_t gpio_bit(uint8_t index, uint32_t mask)
	{
		return (1 << index) & mask;
	}

	/**
	 * Set the output bit for a given GPIO pin index to a specified value.
	 * This will only have an effect if the corresponding bit is first set
	 * to `0` (i.e. output) in the `output_enable` register, and if the pin
	 * is a valid output pin.
	 *
	 * @returns true if the output was set successfully
	 */
	bool output_set(uint8_t index, bool value) volatile
	{
		const uint32_t Bit = gpio_bit(index, OutputMask);
		if (value)
		{
			output = output | Bit;
		}
		else
		{
			output = output & ~Bit;
		}
		return Bit != 0;
	}

	/**
	 * Set the output enable bit for a given GPIO pin index. If `enable` is
	 * true, the GPIO pin is set to output. If false, it is instead set to
	 * input mode.
	 *
	 * @returns true if the output was enabled successfully
	 */
	bool output_enable(uint32_t index, bool enable = true) volatile
	{
		const uint32_t Bit = gpio_bit(index, OutputMask);
		if (enable)
		{
			outputEnable = outputEnable | Bit;
		}
		else
		{
			outputEnable = outputEnable & ~Bit;
		}
		return Bit != 0;
	}

	template<uint32_t Index>
	inline void output_enable(bool enable = true) volatile
	{
		static_assert(gpio_bit(Index, OutputMask),
		              "GPIO at given index unavailable");
		output_enable(Index, enable);
	}

	/**
	 * Read the input value for a given GPIO pin index. For this to be
	 * meaningful, the corresponding pin must be configured to be an input
	 * first (set output enable to false for the given index). If given an
	 * invalid GPIO pin (outside the input mask), then this value will
	 * always be false.
	 */
	bool input_get(uint32_t index) volatile
	{
		return (input & gpio_bit(index, InputMask)) > 0;
	}

	template<uint32_t Index>
	bool input_get() volatile
	{
		static_assert(gpio_bit(Index, InputMask),
		              "GPIO at given index unavailable");
		return input_get(Index);
	}

	/**
	 * Read the debounced input value for a given GPIO pin index. FOr this
	 * to be meaningful, the corresponding pin must be configured to be an
	 * input first (set output enable to false for the given index). If
	 * given an invalid GPIO pin (outside the input mask), then this value
	 * will always be false.
	 */
	bool input_debounced_get(uint32_t index) volatile
	{
		return (debouncedInput & gpio_bit(index, InputMask)) > 0;
	}
};

/**
 * A driver for Sonata's Board GPIO (instance 0).
 *
 * Documentation source:
 * https://lowrisc.org/sonata-system/doc/ip/gpio.html
 */
struct SonataGpioBoard : SonataGpio<0x0000'00FF, 0x0001'FFFF>
{
	/**
	 * The bit mappings of the output GPIO pins available in Sonata's General
	 * GPIO.
	 *
	 * Source: https://lowrisc.github.io/sonata-system/doc/ip/gpio.html
	 */
	enum class Outputs : uint32_t
	{
		Leds = (0xFFu << 0),
	};

	/**
	 * The bit mappings of the input GPIO pins available in Sonata's General
	 * GPIO.
	 *
	 * Source: https://lowrisc.github.io/sonata-system/doc/ip/gpio.html
	 */
	enum class Inputs : uint32_t
	{
		DipSwitches            = (0xFFu << 0),
		Joystick               = (0x1Fu << 8),
		SoftwareSelectSwitches = (0x7u << 13),
		MicroSdCardDetection   = (0x1u << 16),
	};

	/**
	 * Represents the state of Sonata's joystick, where each possible input
	 * corresponds to a given bit in the General GPIO's input register.
	 *
	 * Note that up to 3 of these bits may be asserted at any given time: pressing
	 * down the joystick whilst pushing it in a diagonal direction (i.e. 2 cardinal
	 * directions).
	 *
	 */
	enum class Joystick : uint16_t
	{
		Left    = 1u << 8,
		Up      = 1u << 9,
		Pressed = 1u << 10,
		Down    = 1u << 11,
		Right   = 1u << 12,
	};

	/**
	 * The bit index of the first GPIO pin connected to a user LED.
	 */
	static constexpr uint32_t FirstLED = 0;
	/**
	 * The bit index of the last GPIO pin connected to a user LED.
	 */
	static constexpr uint32_t LastLED = 7;
	/**
	 * The number of user LEDs.
	 */
	static constexpr uint32_t LEDCount = LastLED - FirstLED + 1;
	/**
	 * The mask covering the GPIO pins used for user LEDs.
	 */
	static constexpr uint32_t LEDMask = uint32_t(Outputs::Leds);

	/**
	 * The output bit mask for a given user LED index
	 */
	constexpr static uint32_t led_bit(uint32_t index)
	{
		return gpio_bit(index + FirstLED, LEDMask);
	}

	/**
	 * Switches off the LED at the given user LED index
	 */
	void led_on(uint32_t index) volatile
	{
		output = output | led_bit(index);
	}

	/**
	 * Switches on the LED at the given user LED index
	 */
	void led_off(uint32_t index) volatile
	{
		output = output & ~led_bit(index);
	}

	/**
	 * Toggles the LED at the given user LED index
	 */
	void led_toggle(uint32_t index) volatile
	{
		output = output ^ led_bit(index);
	}

	/**
	 * The bit index of the first GPIO pin connected to a user switch.
	 */
	static constexpr uint32_t FirstSwitch = 0;
	/**
	 * The bit index of the last GPIO pin connected to a user switch.
	 */
	static constexpr uint32_t LastSwitch = 7;
	/**
	 * The number of user switches.
	 */
	static constexpr uint32_t SwitchCount = LastSwitch - FirstSwitch + 1;
	/**
	 * The mask covering the GPIO pins used for user switches.
	 */
	static constexpr uint32_t SwitchMask = uint32_t(Inputs::DipSwitches);

	/**
	 * The input bit mask for a given user switch index
	 */
	constexpr static uint32_t switch_bit(uint32_t index)
	{
		return gpio_bit(index + FirstSwitch, SwitchMask);
	}

	/**
	 * Returns the value of the switch at the given user switch index.
	 */
	bool read_switch(uint32_t index) volatile
	{
		return (input & switch_bit(index)) > 0;
	}

	/**
	 * Returns the state of the joystick.
	 */
	Joystick read_joystick() volatile
	{
		return Joystick(input & uint32_t(Inputs::Joystick));
	}
};

/**
 * A driver for Sonata's Raspberry Pi HAT Header GPIO.
 */
using SonataGpioRaspberryPiHat = SonataGpio<0x0FFF'FFFF, 0x0FFF'FFFF>;
using SonataGpioArduinoShield  = SonataGpio<0x0000'3FFF, 0x0000'3FFF>;
using SonataGpioPmod           = SonataGpio<0x0000'00FF, 0x0000'00FF>;
using SonataGpioPmod0          = SonataGpioPmod;
using SonataGpioPmod1          = SonataGpioPmod;
using SonataGpioPmodC          = SonataGpio<0x0000'003F, 0x0000'003F>;


using SonataJoystick = SonataGpioBoard::Joystick;

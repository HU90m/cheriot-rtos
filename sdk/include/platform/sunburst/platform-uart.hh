#pragma once
#pragma push_macro("CHERIOT_PLATFORM_CUSTOM_UART")
#define CHERIOT_PLATFORM_CUSTOM_UART
#include_next <platform-uart.hh>
#pragma pop_macro("CHERIOT_PLATFORM_CUSTOM_UART")

/**
 * OpenTitan UART
 */
template<unsigned DefaultBaudRate = 115'200>
class OpenTitanUart
{
	public:
	uint32_t intrState;
	uint32_t intrEnable;
	uint32_t intrTest;
	uint32_t alertTest;
	uint32_t ctrl;
	uint32_t status;
	uint32_t rData;
	uint32_t wData;
	uint32_t fifoCtrl;
	uint32_t fifoStatus;
	uint32_t ovrd;
	uint32_t val;
	uint32_t timeoutCtrl;

	void init(unsigned baudRate = DefaultBaudRate) volatile
	{
		// NCO = 2^20 * baud rate / cpu frequency
		const uint32_t NCO =
		  ((static_cast<uint64_t>(baudRate) << 20) / CPU_TIMER_HZ);
		// Set the baud rate and enable transmit & receive
		ctrl = (NCO << 16) | 0b11;
	};

	bool can_write() volatile
	{
		return (fifoStatus & 0xff) < 32;
	};

	bool can_read() volatile
	{
		// Not efficient, but explanatory
		return ((fifoStatus >> 16) & 0xff) > 0;
	};

	/**
	 * Write one byte, blocking until the byte is written.
	 */
	void blocking_write(uint8_t byte) volatile
	{
		while (!can_write()) {}
		wData = byte;
	}

	/**
	 * Read one byte, blocking until a byte is available.
	 */
	uint8_t blocking_read() volatile
	{
		while (!can_read()) {}
		return rData;
	}
};

#ifndef CHERIOT_PLATFORM_CUSTOM_UART
using Uart = OpenTitanUart<>;
static_assert(IsUart<Uart>);
#endif

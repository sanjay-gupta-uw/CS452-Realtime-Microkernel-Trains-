#include "../include/state_machine.h"
#include "../include/uassert.h"
#include "../include/io.h"
#include "../../shared_constants.h"
#include "../../rpi.h"

#define MMIO_BASE 0xFE000000
static char *const UART0_BASE = (char *)(MMIO_BASE + 0x201000);
static char *const UART3_BASE = (char *)(MMIO_BASE + 0x201600);

static char *const line_uarts[] = {nullptr, UART0_BASE, UART3_BASE};
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))
#define UART_FR 0x18

// masks for flag register
#define UART_FR_TXFE (1 << 7)
#define UART_FR_CTS (1 << 0)

TransmitMachine::TransmitMachine()
{
    isTransmitting = false;
    hasCTSGoneDown = false;
}

TransmitMachine::~TransmitMachine()
{
}

void TransmitMachine::update_state(STATES state)
{
    uassert(int(state) < NUM_STATES && "TransmitMachine::update_state: Invalid state (too large)");
    switch (state)
    {
    case STATES::CTS_LOW:
    {
        hasCTSGoneDown = true;
        break;
    }
    case STATES::CTS_HIGH:
    {
        if (hasCTSGoneDown)
        {
            reset();
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void TransmitMachine::reset()
{
    // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "TransmitMachine::reset\r\n", TRANSMIT_LOCATION + 6, 1);
    isTransmitting = false;
    hasCTSGoneDown = false;
}

bool TransmitMachine::isReady()
{
    // check flag register
    uint32_t FR = UART_REG(2, UART_FR);
    if (!isTransmitting && (FR & UART_FR_TXFE) && (FR & UART_FR_CTS))
    {
        return true;
    }

    return false;
}

void TransmitMachine::begin_transmission()
{
    isTransmitting = true;
}
// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>

#define LED_PIN 1

// custom boards may override default pin definitions with BLEPeripheral(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheral blePeripheral = BLEPeripheral();

// create service
BLEService strobeService = BLEService("19b10000e8f2537e4f6cd104768a1214");

// create switch characteristic
BLEIntCharacteristic rpmCharacteristic = BLEIntCharacteristic("19b10001e8f2537e4f6cd104768a1214", BLERead | BLEWrite);

// create switch characteristic
BLEIntCharacteristic dutyCharacteristic = BLEIntCharacteristic("19b10002e8f2537e4f6cd104768a1214", BLERead | BLEWrite);

int rpm;
int duty;

void start_timer(void)
{
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->PRESCALER = 7;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER2->CC[0] = 1000;
    NRF_TIMER2->CC[1] = 500;
    NRF_TIMER2->SHORTS = 1 << 0;

    // Enable interrupt on Timer 2, both for CC[0] and CC[1] compare match events
    NRF_TIMER2->INTENSET =
        (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos) |
        (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
    NVIC_EnableIRQ(TIMER2_IRQn);

    NRF_TIMER2->TASKS_START = 1; // Start TIMER2
}

void timer_freq(int freq)
{
    int cnt = 125000 / freq;
    NRF_TIMER2->CC[0] = cnt;
    NRF_TIMER2->CC[1] = (cnt * duty) / 1000;
}

void timer_rpm(int rpm)
{
    int cnt = 125000 * 60 / rpm;
    NRF_TIMER2->CC[0] = cnt;
    NRF_TIMER2->CC[1] = (cnt * duty) / 1000;
}

extern "C"
{

    void TIMER2_IRQHandler(void)
    {
        if ((NRF_TIMER2->EVENTS_COMPARE[0] != 0) and ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk) != 0))
        {
            NRF_TIMER2->EVENTS_COMPARE[0] = 0; // Clear compare register 0 event
            NRF_GPIO->OUTSET = 1 << LED_PIN;
        }

        if ((NRF_TIMER2->EVENTS_COMPARE[1] != 0) and ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE1_Msk) != 0))
        {
            NRF_TIMER2->EVENTS_COMPARE[1] = 0; // Clear compare register 1 event
            NRF_GPIO->OUTCLR = 1 << LED_PIN;
        }
    }
}

void setup()
{
    // set LED pin to output mode
    pinMode(LED_PIN, OUTPUT);

    // set advertised local name and service UUID
    blePeripheral.setLocalName("StrobeGadget");
    blePeripheral.setAdvertisedServiceUuid(strobeService.uuid());

    // add service and characteristic
    blePeripheral.addAttribute(strobeService);
    blePeripheral.addAttribute(rpmCharacteristic);
    blePeripheral.addAttribute(dutyCharacteristic);

    // begin initialization
    blePeripheral.begin();

    start_timer();
    rpm = 1000;
    duty = 1;
    timer_rpm(rpm);
}

void loop()
{
    BLECentral central = blePeripheral.central();

    if (central)
    {
        while (central.connected())
        {
            if (rpmCharacteristic.written())
            {
                int new_rpm = rpmCharacteristic.value();
                if (new_rpm >= 1 and new_rpm <= 50000)
                {
                    rpm = new_rpm;
                    timer_rpm(rpm);
                }
            }

            if (dutyCharacteristic.written())
            {
                int new_duty = dutyCharacteristic.value();
                if (new_duty >= 0 and new_duty <= 1000)
                {
                    duty = new_duty;
                    timer_rpm(rpm);
                }
            }
        }
    }
}
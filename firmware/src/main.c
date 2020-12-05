/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stc15.h>
#include <stdint.h>

inline void sleep() {
    // put to sleep mode
    PCON = 0x02;

    // Waked up
    __asm
        nop
        nop
        nop
        nop
    __endasm;
}

static void delay_ms(unsigned int ms)
{
    do {
        unsigned int i = F_CPU / 17000;
        while (--i);
    } while (--ms);
}

static uint16_t random65535() {
    static uint16_t ctx = 0;

    for (uint8_t i = 0; i < 2; i ++) {
        uint8_t lsb = ctx & 1;
        ctx >>= 1;
        if (lsb || !ctx) {
            ctx ^= 0xB400;
        }
    }

    return ctx;
}

static uint8_t random8() {
    uint16_t r = 0;

    while (r < 8) {
        r = random65535();
    } // get a randome number from 8 to 65535

    return r & 0x07;
}

static uint8_t random6() {
    uint8_t r = 7;

    while (r > 5) {
        r = random8();
    }

    return r;
}

const uint16_t read_button_count = 60;

__sbit __at (0xCC) P5_4;

static uint8_t use_p5_4 = 0;

static uint8_t button_down() {
    if (use_p5_4) return P5_4 == 0;
    else return P3_4 == 0;
}

static void output(uint8_t v) {
    P3 = 0x10 | v;
}

static void animate_output(uint8_t r) {
    uint16_t i;
    for (i = 100; i < 300; i += 50) {
        output(0x0E); // 0b00001110
        delay_ms(i);
        output(0x0C); // 0b00001101
        delay_ms(i);
        output(0x0A); // 0b00001011
        delay_ms(i);
        output(0x07); // 0b00000111;
        delay_ms(i);
    }

    uint8_t v = 0x0F;

    switch (r) {
        case 0:
            v = 0x0E; // 0b00001110
            break;
        case 1:
            v = 0x0B; // 0b00001011
            break;
        case 2:
            v = 0x06; // 0b00000110
            break;
        case 3:
            v = 0x03; // 0b00000011
            break;
        case 4:
            v = 0x02; // 0b00000010
            break;
        case 5:
            v = 0x01; // 0b00000001
            break;
    }

    for (i = 0; i < 4; i ++) {
        output(0x0F);
        delay_ms(200);
        output(v);
        delay_ms(700);
    }
}

void main() {
    P3M0 = 0;
    P3M1 = 0;

    output(0x0F);

    P5M0 = 0;
    P5M1 = 0;

    P5_4 = 1;

    __asm
        nop
        nop
        nop
        nop
    __endasm;

    for (;;) {
        if (P5_4 != 0) {
            use_p5_4 = 1;
            break;
        }

        if (P3_4 != 0) {
            use_p5_4 = 0;
            break;
        }
    }

    // enable interrupts
    INT_CLKO |= 0x10; // INT2
    EA = 1;

    uint8_t should_sleep = 1;

    while (1) {
        output(0x0F);

        if (should_sleep) {
            sleep();
        }

        uint16_t i;
        uint8_t r = random6();

        // debounce while button is down
        for (i = 0; i < read_button_count; i ++) {
            delay_ms(1);
            if (button_down()) {
                r = random6();
            }
        }

        while (button_down()) {
            delay_ms(1);
            r = random6();
        }

        // debounce while button is up
        for (i = 0; i < read_button_count; i ++) {
            delay_ms(1);
            if (button_down()) {
                r = random6();
            }
        }

        animate_output(r);

        // wait about 10 seconds
        should_sleep = 1;
        for (i = 0; i < 1000; i ++) {
            if (button_down()) {
                should_sleep = 0;
                break;
            }

            delay_ms(10);
        }
    }
}

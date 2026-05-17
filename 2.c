#include <xc.h>
#include <string.h>

#define _XTAL_FREQ 20000000

// CONFIG
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

// LCD
#define RS RB0
#define EN RB1

char password[] = "1234";
char entered[5];

unsigned char index = 0;
unsigned char wrong_count = 0;

// ---------------- LCD ----------------

void pulse_enable()
{
    EN = 1;
    __delay_ms(2);
    EN = 0;
}

void lcd_cmd(unsigned char cmd)
{
    RS = 0;

    PORTB &= 0x0F;
    PORTB |= (cmd & 0xF0);

    pulse_enable();

    PORTB &= 0x0F;
    PORTB |= ((cmd << 4) & 0xF0);

    pulse_enable();

    __delay_ms(2);
}

void lcd_data(unsigned char data)
{
    RS = 1;

    PORTB &= 0x0F;
    PORTB |= (data & 0xF0);

    pulse_enable();

    PORTB &= 0x0F;
    PORTB |= ((data << 4) & 0xF0);

    pulse_enable();

    __delay_ms(2);
}

void lcd_init()
{
    __delay_ms(20);

    lcd_cmd(0x02);
    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
}

void lcd_string(const char *str)
{
    while(*str)
    {
        lcd_data(*str++);
    }
}

// ---------------- PWM SERVO ----------------

void PWM_Init()
{
    TRISC2 = 0;

    PR2 = 249;

    CCP1CON = 0x0C;

    T2CON = 0x07;
}

void Servo_Angle(unsigned int angle)
{
    unsigned int duty;

    duty = ((angle / 18.0) + 12);

    CCPR1L = duty;
}

// ---------------- KEYPAD ----------------

char keypad_scan()
{
    // ROW 1
    RD0 = 0; RD1 = 1; RD2 = 1; RD3 = 1;

    if(RD4 == 0) { __delay_ms(2); return '1'; }
    if(RD5 == 0) { __delay_ms(2); return '2'; }
    if(RD6 == 0) { __delay_ms(2); return '3'; }

    // ROW 2
    RD0 = 1; RD1 = 0; RD2 = 1; RD3 = 1;

    if(RD4 == 0) { __delay_ms(2); return '4'; }
    if(RD5 == 0) { __delay_ms(2); return '5'; }
    if(RD6 == 0) { __delay_ms(2); return '6'; }

    // ROW 3
    RD0 = 1; RD1 = 1; RD2 = 0; RD3 = 1;

    if(RD4 == 0) { __delay_ms(2); return '7'; }
    if(RD5 == 0) { __delay_ms(2); return '8'; }
    if(RD6 == 0) { __delay_ms(2); return '9'; }

    // ROW 4
    RD0 = 1; RD1 = 1; RD2 = 1; RD3 = 0;

    if(RD4 == 0) { __delay_ms(2); return '*'; }
    if(RD5 == 0) { __delay_ms(2); return '0'; }
    if(RD6 == 0) { __delay_ms(2); return '#'; }

    return 0;
}

// ---------------- MAIN ----------------

void main()
{
    char key;

    TRISB = 0x00;

    // ROWS OUTPUT
    TRISD0 = 0;
    TRISD1 = 0;
    TRISD2 = 0;
    TRISD3 = 0;

    // COLUMNS INPUT
    TRISD4 = 1;
    TRISD5 = 1;
    TRISD6 = 1;

    // BUZZER
    TRISC0 = 0;

    // WHITE LED
    TRISC3 = 0;

    // GREEN LED
    TRISC4 = 0;

    // RED LED
    TRISC5 = 0;

    PORTB = 0x00;

    // STARTUP STATES
    RC0 = 0;

    RC3 = 0;
    RC4 = 0;
    RC5 = 0;

    lcd_init();

    PWM_Init();

    Servo_Angle(0);

    lcd_cmd(0x80);

    lcd_string("ENTER PASSWORD");

    lcd_cmd(0xC0);

    while(1)
    {
        key = keypad_scan();

        if(key != 0)
        {
            __delay_ms(5);

            // NUMBER
            if(key >= '0' && key <= '9')
            {
                if(index < 4)
                {
                    entered[index] = key;

                    lcd_data('*');

                    // WHITE LED
                    RC3 = 1;

                    __delay_ms(50);

                    RC3 = 0;

                    index++;
                }
            }

            // ENTER
            if(key == '#')
            {
                entered[4] = '\0';

                lcd_cmd(0x01);

                // CORRECT PASSWORD
                if(strcmp(entered, password) == 0)
                {
                    wrong_count = 0;

                    // GREEN LED
                    RC4 = 1;

                    lcd_string("ACCESS OK");

                    Servo_Angle(90);

                    RC0 = 1;

                    __delay_ms(300);

                    RC0 = 0;

                    __delay_ms(1000);

                    RC4 = 0;
                }

                // WRONG PASSWORD
                else
                {
                    wrong_count++;

                    // RED LED
                    RC5 = 1;

                    lcd_string("WRONG PASS");

                    RC0 = 1;

                    __delay_ms(300);

                    RC0 = 0;

                    __delay_ms(1000);

                    RC5 = 0;

                    // SYSTEM LOCK
                    if(wrong_count >= 3)
                    {
                        lcd_cmd(0x01);

                        lcd_string("SYSTEM LOCKED");

                        RC5 = 1;

                        RC0 = 1;

                        while(1);
                    }
                }

                __delay_ms(300);

                lcd_cmd(0x01);

                lcd_string("ENTER PASSWORD");

                lcd_cmd(0xC0);

                index = 0;

                memset(entered, 0, sizeof(entered));
            }

            // CLEAR
            if(key == '*')
            {
                lcd_cmd(0x01);

                lcd_string("ENTER PASSWORD");

                lcd_cmd(0xC0);

                index = 0;

                memset(entered, 0, sizeof(entered));
            }
        }
    }
}
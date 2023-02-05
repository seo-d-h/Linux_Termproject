#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <wiringSerial.h>
#include <wiringPi.h>
#include <softPwm.h>

#define PIN 1
#define CDS 0

// Define some device parameters
#define I2C_ADDR   0x27 // I2C device address

// Define some device constants
#define LCD_CHR  1 // Mode - Sending data
#define LCD_CMD  0 // Mode - Sending command

#define LINE1  0x80 // 1st line
#define LINE2  0xC0 // 2nd line 

#define LCD_BACKLIGHT   0x08  // On
// LCD_BACKLIGHT = 0x00  # Off

#define ENABLE  0b00000100 // Enable bit

// lcd 함수 호출부
void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);

// added by Lewis
void typeInt(int i);
void typeFloat(float myFloat);
void lcdLoc(int line); //move cursor
void ClrLcd(void); // clr LCD return home
void typeln(const char* s);
void typeChar(char val);
int fd_lcd;  // seen by all subroutines


int main(void) {
    if (wiringPiSetup() == -1)
        return 1;

    time_t timer;
    struct tm* t;
    timer = time(NULL);
    t = localtime(&timer);

    char buff[200];     // 시간을 저장하기 위한 버퍼

    FILE* fp;
    fp = fopen("Time_Save.txt", "a");

    if (fp == NULL)
        printf("cannot open file\n");
    else
        printf("success opening file!\n");


    pinMode(PIN, SOFT_PWM_OUTPUT);      // 서보모터 pwm 출력
    pinMode(CDS, INPUT);        // 조도센서 입력

    softPwmCreate(PIN, 0, 200);

    fd_lcd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init(); // setup LCD

    int fd;    // 블루투스로 받는
    int data;   // 블루투스로 받는 data

    char array1[] = "Light ON";
    char array2[] = "Light OFF";

    fd = serialOpen("/dev/ttyS0", 9600);
    if (fd < 0) {
        fprintf(stderr, "Failed to open serial device: %s\n", strerror(errno));
    }

    printf("\nRaspberry Pi UART Test\n");
    while (1) {
        // 날짜, 시간 저장
        sprintf(buff, "%d / %d / %d  %d : %d : %d", t->tm_year+1900, 
            t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        fputs(buff, fp);
        data = serialGetchar(fd);
        printf("\nRecv Data: %c", data);
        serialPutchar(fd, (char)data);
        serialPuts(fd, "\n");
        fflush(stdout);

        fcloseall();
        switch ((char)data) {
        case '0':       // 조도센서 모드
            if (digitalRead(CDS) == HIGH) {
                softPwmWrite(PIN, 5);
                ClrLcd();
                lcdLoc(LINE1);
                typeln("Light Sensor Mode");
                lcdLoc(LINE2);
                typeln("Light ON");
                delay(500);
            }
            else {
                softPwmWrite(PIN, 12);
                ClrLcd();
                lcdLoc(LINE1);
                typeln("Light Sensor Mode");
                lcdLoc(LINE2);
                typeln("Light OFF");
                delay(500);
            }
            break;
        case '1':       // 블루투스 모드, 조명 ON
            softPwmWrite(PIN, 5);
            ClrLcd();
            lcdLoc(LINE1);
            typeln("Bluetooth Mode");
            lcdLoc(LINE2);
            typeln("Light ON");
            delay(500);
            break;

        case '2':       // 블루투스모드, 조명 off
            softPwmWrite(PIN, 12);
            ClrLcd();
            lcdLoc(LINE1);
            typeln("Bluetooth Mode");
            lcdLoc(LINE2);
            typeln("Light OFF");
            delay(500);
            break;
        }
    }
    return 0;
}


// float to string
void typeFloat(float myFloat) {
    char buffer[20];
    sprintf(buffer, "%4.2f", myFloat);
    typeln(buffer);
}

// int to string
void typeInt(int i) {
    char array1[20];
    sprintf(array1, "%d", i);
    typeln(array1);
}

// clr lcd go home loc 0x80
void ClrLcd(void) {
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}

// go to location on LCD
void lcdLoc(int line) {
    lcd_byte(line, LCD_CMD);
}

// out char to LCD at current position
void typeChar(char val) {

    lcd_byte(val, LCD_CHR);
}


// this allows use of any size string
void typeln(const char* s) {

    while (*s) lcd_byte(*(s++), LCD_CHR);

}

void lcd_byte(int bits, int mode) {

    //Send byte to data pins
    // bits = the data
    // mode = 1 for data, 0 for command
    int bits_high;
    int bits_low;
    // uses the two half byte writes to LCD
    bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    // High bits
    wiringPiI2CReadReg8(fd_lcd, bits_high);
    lcd_toggle_enable(bits_high);

    // Low bits
    wiringPiI2CReadReg8(fd_lcd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits) {
    // Toggle enable pin on LCD display
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd_lcd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd_lcd, (bits & ~ENABLE));
    delayMicroseconds(500);
}


void lcd_init() {
    // Initialise display
    lcd_byte(0x33, LCD_CMD); // Initialise
    lcd_byte(0x32, LCD_CMD); // Initialise
    lcd_byte(0x06, LCD_CMD); // Cursor move direction
    lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
    lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
    lcd_byte(0x01, LCD_CMD); // Clear display
    delayMicroseconds(500);
}
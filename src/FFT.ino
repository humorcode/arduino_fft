/**
* 基于https://github.com/bjmashibing/InternetArchitect
**/
#include <arduinoFFT.h>
#include <HT1632_3208.h>
#include <SPI.h>

#define SAMPLES 64 //Must be a power of 2
#define xres 32    // Total number of  columns in the display, must be <= SAMPLES/2
#define yres 8     // Total number of  rows in the display

int MY_ARRAY[] = {0, 128, 192, 224, 240, 248, 252, 254, 255};  // default = standard pattern
int MY_MODE_1[] = {0, 128, 192, 224, 240, 248, 252, 254, 255}; // standard pattern
int MY_MODE_2[] = {0, 128, 64, 32, 16, 8, 4, 2, 1};            // only peak pattern
int MY_MODE_3[] = {0, 128, 192, 160, 144, 136, 132, 130, 129}; // only peak +  bottom point
int MY_MODE_4[] = {0, 128, 192, 160, 208, 232, 244, 250, 253}; // one gap in the top , 3rd light onwards
int MY_MODE_5[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};          // standard pattern, mirrored vertically

double vReal[SAMPLES];
double vImag[SAMPLES];
char data_avgs[xres];

int yvalue;
int displaycolumn, displayvalue;
int peaks[xres];
const int buttonPin = 6; // the number of the pushbutton pin
// int state = HIGH;        // the current reading from the input pin
int previousState = LOW; // the previous reading from the input pin
int displaymode = 1;
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers

QXC_HT1632_3208LEDMatrix LED3208display = QXC_HT1632_3208LEDMatrix(11, 13, 4);
arduinoFFT FFT = arduinoFFT(); // FFT object

void setup()
{
    // Serial.begin(9600);
    // ADCSRA = 0b11100101; // set ADC to free running mode and set pre-scalar to 32 (0xe5)
    // ADMUX = 0b00000000;  // use pin A0 and external voltage reference
    pinMode(buttonPin, INPUT);
    pinMode(A0, INPUT);
    LED3208display.begin(); // initialize display
    delay(50);              // wait to get reference voltage stabilized
}

void loop()
{
    // delay(10);
    // ++ Sampling
    for (int i = 0; i < SAMPLES; i++)
    {
        // while (!(ADCSRA & 0x10))
        // ;                // wait for ADC to complete current conversion ie ADIF bit set
        // ADCSRA = 0b11110101; // clear ADIF bit so that ADC can do next operation (0xf5)
        // int value = ADC - 512; // Read from ADC and subtract DC offset caused value
        // int value = ADC - 512;
        int value = analogRead(A0);
        // while (value < 100)
        // {
        value = analogRead(A0);
        // }

        // vReal[i] = value / 8; // Copy to bins after compressing
        vReal[i] = value / 3; // Copy to bins after compressing
        vImag[i] = 0;
    }
    // -- Sampling

    // ++ FFT
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    // -- FFT

    // ++ re-arrange FFT result to match with no. of columns on display ( xres )
    int step = (SAMPLES / 2) / xres;
    int c = 0;
    for (int i = 0; i < (SAMPLES / 2); i += step)
    {
        data_avgs[c] = 0;
        for (int k = 0; k < step; k++)
        {
            data_avgs[c] = data_avgs[c] + vReal[i + k];
        }
        data_avgs[c] = data_avgs[c] / step;
        c++;
    }
    // -- re-arrange FFT result to match with no. of columns on display ( xres )

    // ++ send to display according measured value
    for (int i = 0; i < xres; i++)
    {
        data_avgs[i] = constrain(data_avgs[i], 0, 80);    // set max & min values for buckets
        data_avgs[i] = map(data_avgs[i], 0, 80, 0, yres); // remap averaged values to yres
        yvalue = data_avgs[i];

        peaks[i] = peaks[i] - 1; // decay by one light
        if (yvalue > peaks[i])
            peaks[i] = yvalue;
        yvalue = peaks[i];
        displayvalue = MY_ARRAY[yvalue];
        // displaycolumn = 31 - i;
        displaycolumn = i;
        for (int nn = 0; nn < 8; nn++)
        {
            LED3208display.drawPixel(displaycolumn, nn, bitRead(displayvalue, nn));
        }
        //mx.setColumn(displaycolumn, displayvalue); // for left to right
    }
    LED3208display.writeScreen();
    // -- send to display according measured value

    displayModeChange(); // check if button pressed to change display mode
}

void displayModeChange()
{
    int reading = digitalRead(buttonPin);
    if (reading == HIGH && previousState == LOW && millis() - lastDebounceTime > debounceDelay) // works only when pressed

    {

        switch (displaymode)
        {
        case 1: //       move from mode 1 to 2
            displaymode = 2;
            for (int i = 0; i <= 8; i++)
            {
                MY_ARRAY[i] = MY_MODE_2[i];
            }
            break;
        case 2: //       move from mode 2 to 3
            displaymode = 3;
            for (int i = 0; i <= 8; i++)
            {
                MY_ARRAY[i] = MY_MODE_3[i];
            }
            break;
        case 3: //     move from mode 3 to 4
            displaymode = 4;
            for (int i = 0; i <= 8; i++)
            {
                MY_ARRAY[i] = MY_MODE_4[i];
            }
            break;
        case 4: //     move from mode 4 to 5
            displaymode = 5;
            for (int i = 0; i <= 8; i++)
            {
                MY_ARRAY[i] = MY_MODE_5[i];
            }
            break;
        case 5: //      move from mode 5 to 1
            displaymode = 1;
            for (int i = 0; i <= 8; i++)
            {
                MY_ARRAY[i] = MY_MODE_1[i];
            }
            break;
        }

        lastDebounceTime = millis();
    }
    previousState = reading;
}
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// The TinySPI lib is used to communicate with the DAC
#include <tinySPI.h>  

// The frequency lookup table. For every value coming
// from the ADC (1024 in total), there is a corresponding 
// value to set the Frequency of the PWM using the ocr_step_value.
// These values are precalculated to improve speed.
#include <clock_frq_lookup.h>

// The DAC lookup table. For every value coming
// from the ADC (1024 in total), there is a corresponding 
// value to set the DAC to generate the Input Voltage.
// These values are precalculated to improve speed.
#include <dac_voltage_lookup.h>

// define the SPI latch pin
#define SPI_LATCH PA3

// function declarations
void setDACVoltage(uint16_t value);
uint16_t getADCSample(void);

// declare variable for ocr_step_value
// 45455 is just a default value
volatile uint16_t ocr_step_value = 5252; // 238hz

// declare variable for the unput_voltage read by the ADC
volatile uint16_t input_voltage = 0;
volatile uint16_t current_input_voltage = 0;

int main(void)
{
    /*
     * DAC Config
     */

    // set pin config
    DDRA |= (1 << SPI_LATCH);

    // start SPI library
    SPI.begin(); 

    // SPI_LATCH goes high 
    PORTA |= (1 << SPI_LATCH); 

    /*
     * Timer1 Config
     */

    // set pin to pwm output mode 
    DDRA |= (1 << DDA6); 

    // clear Timer1 Registers
    TCCR1A = 0;
    TCCR1B = 0;

    // enable 'Compare Match Toggle' on OC1A
    TCCR1A |= (1 << COM1A0);

    // set Prescaler to /8
    TCCR1B |= (1 << CS11)| (0 << CS10);

    // Output Compare Match A Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);
    
    /*
     * ADC Config
     */

    // Set ADC prescaler to 128 - 125KHz sample rate @ 16MHz
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  
    // set ADC1 as the input channel and enable external voltage reference (4.0V)
    ADMUX |= (1 << MUX0) | (1 << REFS0);

    // Set ADC to Free-Running Mode
    ADCSRA |= (1 << ADATE);  
    
    // Enable ADC
    ADCSRA |= (1 << ADEN);

    // Enable ADC Interrupt
    ADCSRA |= (1 << ADIE);  

    // Enable Global Interrupts
    sei(); 

    // Start A2D Conversions    
    ADCSRA |= (1 << ADSC);  

    //setDACVoltage(80);

    // Loop Forever
    for(;;)  
	{
	}  
}

/*
 * setDACVoltage
 */
void setDACVoltage(uint16_t value)
{
    // Set the config bits
    uint8_t config_bits = 0 << 3 | 0 << 2 | 1 << 1 | 1;

    // Compose the first byte to send to the DAC:
    // the 4 control bits, and the 4 most significant bits of the value
    uint8_t first_byte = config_bits << 4 | (value & 0xF00) >> 8;

    // Second byte is the lower 8 bits of the value
    uint8_t second_byte = value & 0xFF;

    // SPI_LATCH goes low
    PORTA &= ~(1 << SPI_LATCH); 

    SPI.transfer(first_byte);
    SPI.transfer(second_byte); 

    // SPI_LATCH goes high 
    PORTA |= (1 << SPI_LATCH); 
}

/*
 * The interrupt handler for the ADC 
 */
ISR(ADC_vect)
{
    // get the sample value from ADCL
    uint8_t adc_lobyte = ADCL;

    // add lobyte and hibyte to combine new input_voltage
    input_voltage = ADCH << 8 | adc_lobyte;

    if(input_voltage != current_input_voltage)
    {
        // set the Freqency
        // use pgm_read_word_near to get the value from flash memory
        ocr_step_value = pgm_read_word_near(frq_lookup + input_voltage);       

        // set the DAC
        // use pgm_read_word_near to get the value from flash memory
        setDACVoltage(pgm_read_word_near(dac_lookup + input_voltage));

        // store the new into the current
        current_input_voltage = input_voltage;
    }
}

/*
 * The interrupt handler for Timer1 
 */
ISR(TIM1_COMPA_vect) 
{
    // By continually adding a value to the OCR1A value we can
    // create a PWM signal with a variable Frequency which is
    // not bound by the prescaler and has a 50% duty cycle.
    OCR1A = OCR1A + ocr_step_value;
}
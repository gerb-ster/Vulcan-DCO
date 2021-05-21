#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <tinySPI.h>  

// define some pins
#define CLOCK_OUT_PIN PB2
#define SPI_LATCH PA3

// function declarations
void setPWMFrequency(unsigned int frequency);
void setDACVoltage(int value);
uint16_t getADCSample(void);

int main(void)
{
    /*
     * Pin configuration
     */
    DDRB |= (1 << CLOCK_OUT_PIN);
    DDRA |= (1 << SPI_LATCH);
    
    // Disable digital input on ADC0 pin. Not strictly required
	// but reduces power consumption a bit.
	DIDR0 = 0x01;

    /*
     * DAC Config
     */
    SPI.begin(); 

    // SPI_LATCH goes high 
    PORTA |= (1 << SPI_LATCH); 
    
    /*
     * ADC Config
     *
     * Enable ADC with 10bit resolution
     */
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0);
    
    /*
     * Timer0 Config
     */
    TCCR1A = 0;
    TCCR1B = 0;
    //TCNT1 = 0;

    // CTC
    TCCR1B |= (1 << WGM12);
    // Prescaler 1
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
    // Output Compare Match A Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);
    
    // set the Clock Frequency
    setPWMFrequency(880);

    // enable global interrupts
    sei();

    // declare inputVoltage
    uint16_t inputVoltage = 0;
    uint16_t currentInputVoltage = 0;

    // the main loop
    while(1)
    {
        // read voltage input
        inputVoltage = getADCSample();
    
        // only set the dac when something changed
        if(inputVoltage != currentInputVoltage)
        {
            // set the DAC
            setDACVoltage(inputVoltage * 4);

            // store the new into the current
            currentInputVoltage =  inputVoltage;
        }            
    }
}

/*
 * setPWMFrequency
 */
void setPWMFrequency(unsigned int frequency)
{
    //int tValue = (F_CPU / 8 / (value * 2));
    int ocr = (F_CPU / frequency / 2 / 8) - 2;

    OCR1A = ocr;
}

/*
 * setDACVoltage
 */
void setDACVoltage(int value)
{
    // Set the config bits
    uint8_t configBits = 0 << 3 | 0 << 2 | 1 << 1 | 1;

    // Compose the first byte to send to the DAC:
    // the 4 control bits, and the 4 most significant bits of the value
    uint8_t firstByte = configBits << 4 | (value & 0xF00) >> 8;

    // Second byte is the lower 8 bits of the value
    uint8_t secondByte = value & 0xFF;

    // SPI_LATCH goes low
    PORTA &= ~(1 << SPI_LATCH); 

    SPI.transfer(firstByte);
    SPI.transfer(secondByte); 

    // SPI_LATCH goes high 
    PORTA |= (1 << SPI_LATCH); 
}

/*
 * getADCSample
 */
uint16_t getADCSample(void)
{
	// start a conversion by setting the ADSC bit in ADCSRA
	ADCSRA |= (1 << ADSC); 
	
    // wait for the conversion to complete
	while (ADCSRA & (1 << ADSC)) ;              

    // get the sample value from ADCL
    uint8_t adc_lobyte = ADCL;

    // add lobyte and hibyte   
    uint16_t raw_adc = ADCH << 8 | adc_lobyte; 

	// return the ADC value
	return(raw_adc);
}

ISR(TIM1_COMPA_vect) {
    // toggle the PWM output port
    PORTB ^= _BV(CLOCK_OUT_PIN); 
}
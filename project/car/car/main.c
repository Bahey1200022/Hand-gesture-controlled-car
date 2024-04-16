/*
* car.c
*
* Created: 4/5/2024 7:16:57 PM
* Author : moham
*/

#include <avr/io.h> // AVR header
#include <stdio.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#define BIT_IS_SET(byte, bit) (byte & (1 << bit))
#define BIT_IS_CLEAR(byte, bit) (!(byte & (1 << bit)))
#define LAST_3_BITS_MASK 0x07 // Mask for the last 3 bits

#include <avr/delay.h>
#define LCD_DATA PORTB // port B is selected as LCD data port

#define en PD7 // enable signal is connected to port D pin 7
#define rw PD6 // read/write signal is connected to port D pin 6
#define rs PD4 // register select signal is connected to port D pin 5
void LCD_cmd(unsigned char cmd);
void init_LCD(void);
void LCD_write(unsigned char data);
void Cursor_pos(unsigned char x_pos, unsigned char y_pos);



void pwm_init(void);
int main(void)
{
	DDRC |= (1<<DDC0) | (1<<DDC1); // outputs for motor driver
	DDRB = 0xFF; // set LCD data port as output
	DDRD = 0b11111000;// set LCD signals (RS, RW, E) as out put, pwm :pd3  , servo control :pd5
	pwm_init();
	init_LCD(); // initialize LCD
	_delay_ms(10); // delay of 100 Milli seconds

	LCD_cmd(0x0C); // display on, cursor off
	_delay_ms(10);
	LCD_cmd(0x01); // clear

	unsigned char ch[4]={' '};
	uint8_t middle=0;
	uint8_t index=0;
	uint8_t thumb=0;
	uint8_t val=0;
	while (1)
	{
		LCD_cmd(0x01);
		
		ADMUX = 0b01100100;
		ADCSRA = 0b10000011; ///pin4
		for (int j =0;j<4;j++){
			ch[j]=' ';///////////CLEARING CHAR ARRAY
		}
		////////////WRITING
		Cursor_pos(0,7);
		ADCSRA |= (1 << ADSC);					// start ADC conversion
		while(BIT_IS_SET(ADCSRA, ADSC)) {}
		index=ADCH;
		ADMUX = 0b01100011;// pin3
		ADCSRA |= (1 << ADSC);					// start ADC conversion
		while(BIT_IS_SET(ADCSRA, ADSC)) {}
		thumb=ADCH;
		ADMUX = 0b01100101;// pin5
		ADCSRA |= (1 << ADSC);					// start ADC conversion
		while(BIT_IS_SET(ADCSRA, ADSC)) {}
		middle=ADCH;
		itoa(ADCH,ch,8);  ////convert int to string

		for (int j=0;j<4;j++){
			if (ch[j]<'0'||ch[j]>'9')
			LCD_write(' ');
			else
			LCD_write(ch[j]);
		}
		_delay_ms(10);
		if (middle>200 && thumb>200 && index>200){//stop
			PORTD= (1<<PIND5);
			_delay_us(150);
			PORTD = 0x00;
			_delay_ms(200);
			PORTC &= ~(1<<PINC0) ;
			PORTC &= ~(1<<PINC1);
			
		}
		else if (middle<200 &&thumb<200 && index<200){//forward
			OCR2B=375-(middle +thumb+index)/3;

			PORTC |= (1<<PINC0) ;
			PORTC &= ~(1<<PINC1);
			PORTD= (1<<PIND5);
			_delay_us(150);
			PORTD = 0x00;
			_delay_ms(200);
		}
		else if (middle<200 && thumb>200 && index<200){//back
			OCR2B=375-(middle +index)/3;

			PORTC |= (1<<PINC1) ;
			PORTC &= ~(1<<PINC0);
			PORTD= (1<<PIND5);
			_delay_us(150);
			PORTD = 0x00;
			_delay_ms(200);
		}
		else if(thumb<200 && index >200 &&middle>200){
			//go left
			PORTD = (1<<PIND5);
			_delay_us(100);
			PORTD = 0x00;

			_delay_ms(200);
			

		}
		else if(thumb>200 && index <200 && middle>200){
			//go right
			PORTD =(1<<PIND5);
			_delay_ms(200);
			PORTD = 0x00;
			_delay_ms(200);
			

		}
		LCD_cmd(0x01);
		
		
	}
	return 0;
}

void pwm_init(void){
	///datasheet
	
	TCCR2A |=(1<<COM2B1) |(1<< WGM21) |(1<<WGM20) ; ///OC2B FAST PWM;
	TCCR2B |= (1<<CS20) | (1<<WGM22);//no prescaler and wgm2
	
	OCR2A=800; //frequency
	OCR2B=100; //duty cycle 0->255
}





void init_LCD(void)
{
	LCD_cmd(0x38); // initialization in 8bit mode of 16X2 LCD
	_delay_ms(1);

	LCD_cmd(0x01); // make clear LCD
	_delay_ms(1);

	LCD_cmd(0x02); // return home
	_delay_ms(1);

	LCD_cmd(0x06); // make increment in cursor
	_delay_ms(1);

	LCD_cmd(0x80); // "8" go to first line and "0" is for 0th position
	_delay_ms(1);

	return;
}

//*****sending command on LCD******//

void LCD_cmd(unsigned char cmd)
{
	LCD_DATA = cmd; // data lines are set to send command
	
	PORTD &= ~(1 << rs); // RS sets 0, for command data
	PORTD &= ~(1 << rw); // RW sets 0, to write data
	PORTD |= (1 << en); // make enable from high to low
	
	_delay_ms(100);
	PORTD &= ~(1 << en); // make enable low

	return;
}

//******write data on LCD******//

void LCD_write(unsigned char data)
{
	LCD_DATA = data; // data lines are set to send command
	PORTD |= (1 << rs); // RS sets 1, for command data
	PORTD &= ~(1 << rw); // RW sets 0, to write data
	PORTD |= (1 << en); // make enable from high to low

	_delay_ms(2);
	PORTD &= ~(1 << en); // make enable low

	return;
}



void Cursor_pos(unsigned char x_pos, unsigned char y_pos) //x awel aw tani row (0->1) el y column (0->15)
{
	uint8_t the_address=0;
	if (x_pos==0)
	the_address=0x80;
	else if(x_pos==1)
	the_address=0xC0;
	if(y_pos<16)
	the_address+=y_pos;
	LCD_cmd(the_address);
	
}

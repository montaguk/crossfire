#include <avr/io.h>
#include <util/delay.h>

uint8_t switch_state_0 = 0;
uint8_t switch_state_2 = 0;
uint8_t switch_state_7 = 0;
uint8_t old_switch_state_0 = 0;
uint8_t old_switch_state_2 = 0;
uint8_t old_switch_state_7 = 0;

void chk_buttons(){
	switch_state_7 = bit_is_set(PINA, 7); //takes in current state of the switch
	switch_state_0 = bit_is_set(PINA, 0); //takes in current state of the switch
}

void tcnt3_init(){
	TCCR3A |= (1<<WGM31) | (1<<COM3A1);//phase correct PWM with ICR3A, set on down count, clear on up count
	TCCR3B |= (1<<WGM33) | (1<<CS30);//no prescaling
	ICR3 = 10000;	//1MHZ/(2*10000) = 50Hz to get a 20ms period
	OCR3A = 300;	//0 degree state, 600us duty cycle
}

uint8_t main(){
	DDRA &= 0b00000000; //set PA7-PA0 as inputs for buttons 
	DDRC |= 0x40; //set PC6 as output, PC6 is PWM output  
	DDRD |= 0xF0; //set PD4-PD7 as outputs to solenoids
	PORTD &= 0x0F; //set outputs for solenoid initially low

	tcnt3_init(); //initialize pwm
	
	old_switch_state_0 = 0;//save state in old state to compare to in loop
	old_switch_state_7 = 0;	//save state in old state to compare to in loop

	while(1){
		//chk_buttons();		//check the current state
		uint8_t buttons = PINA;
		PORTC = buttons;
		if(buttons & 0x80){
				PORTD |= 0b10010000;
		}
		else{
				PORTD &= 0b01101111;
		}
			//PORTD &= 0b00001111;	//set outputs low to stop moving right
		if(buttons & 0x01){
				PORTD |= 0b01100000;	//set outputs high to move right
		}else{
				PORTD &= 0b11011111;	//set outputs high to move right
		}		

		if(buttons & 0x02){
			//PORTD &= 0b11011111;	//set outputs high to move right
			PORTD |= 0b00010000;
			_delay_ms(300);
			PORTD &= 0x00;
		}
	}
}
	

#include <avr/io.h>

uint8_t switch_state_7 = 0;
uint8_t old_switch_state_7 = 0;

void chk_switches(){
	switch_state_7 = bit_is_set(PINA, 7);
}

void tcnt3_init(){
	TCCR3A |= (1<<WGM31) | (1<<COM3A1);//phase correct PWM with ICR3A, set on down count, clear on up count
	TCCR3B |= (1<<WGM33) | (1<<CS30);//no prescaling
	ICR3 = 10000;	//to get a 20ms period
	OCR3A = 300;	//0 degree state
}

uint8_t main(){
	DDRA &= 0x1F; //set pins to inputs for switches on PA7-PA5
	DDRC |= 0x70; //set PC4-PC6 as outputs, PC6 is PWM output  
	PORTC &= 0xCF; //set outputs for solenoid initially low

	tcnt3_init();
	switch_state_7 = bit_is_set(PINA,7);
	if(switch_state_7 != 0){
		OCR3A = 1200;
	}
	old_switch_state_7 = switch_state_7;
	while(1){
		chk_switches();
		if(switch_state_7 != old_switch_state_7){
			if(switch_state_7 == 0){
				OCR3A = 300;
			}else{
				OCR3A = 1200;
			}
			old_switch_state_7 = switch_state_7;
		}
	}
}
	

#include <avr/io.h>

uint8_t switch_state_5 = 0;
uint8_t switch_state_6 = 0;
uint8_t switch_state_7 = 0;
uint8_t old_switch_state_5 = 0;
uint8_t old_switch_state_6 = 0;
uint8_t old_switch_state_7 = 0;

void chk_switches(){
	switch_state_5 = bit_is_set(PINA, 5); //takes in current state of the switch
	switch_state_6 = bit_is_set(PINA, 6); //takes in current state of the switch
	switch_state_7 = bit_is_set(PINA, 7); //takes in current state of the switch
}

void tcnt3_init(){
	TCCR3A |= (1<<WGM31) | (1<<COM3A1);//phase correct PWM with ICR3A, set on down count, clear on up count
	TCCR3B |= (1<<WGM33) | (1<<CS30);//no prescaling
	ICR3 = 10000;	//1MHZ/(2*10000) = 50Hz to get a 20ms period
	OCR3A = 300;	//0 degree state, 600us duty cycle
}

uint8_t main(){
	DDRA &= 0x1F; //set PA5-PA7 as inputs for switches
	DDRC |= 0x70; //set PC4-PC6 as outputs, PC6 is PWM output  
	PORTC &= 0xCF; //set outputs for solenoid initially low

	tcnt3_init(); //initialize pwm
	switch_state_7 = bit_is_set(PINA,7); //get initial state of the servo switch
	if(switch_state_7 != 0){
		//OCR3A = 1200;			//if state is high, set servo to 180 degrees
		OCR3A = 700;			//if state is high, set servo to 180 degrees
	}

	PORTC |= (bit_is_set(PINA,6)<<PC5);	//set solenoid outputs equal depending on initial switch state
	PORTC |= (bit_is_set(PINA,5)<<PC4);	//" "
	
	old_switch_state_7 = switch_state_7;	//save state in old state to compare to in loop
	old_switch_state_6 = switch_state_6;	//save state in old state to compare to in loop
	old_switch_state_5 = switch_state_5;	//save state in old state to compare to in loop

	while(1){
		chk_switches();		//check the current state
		if(switch_state_7 != old_switch_state_7){//if the switch has been flipped
			if(switch_state_7 == 0){	//set servo position appropriately
				OCR3A = 300;
			}else{
				OCR3A = 700;
			}
			old_switch_state_7 = switch_state_7;
		}
		if(switch_state_6 != old_switch_state_6){//if the switch has been flipped
			if(switch_state_6 == 0){
				PORTC &= 0xDF;		//set output low
			}else{
				PORTC |= 0x20;		//set output high
			}
			old_switch_state_6 = switch_state_6;
		}
		if(switch_state_5 != old_switch_state_5){//if the switch has been flipped
			if(switch_state_5 == 0){
				PORTC &= 0xEF;		//set output low
			}else{
				PORTC |= 0x10;		//set output high
			}
			old_switch_state_5 = switch_state_5;
		}	
	}
}
	

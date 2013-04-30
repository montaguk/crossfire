#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

uint8_t enc1_state;
uint8_t enc2_state;
uint8_t prev_state1;
uint8_t position = 128; //halfway point of position 
uint8_t enc_freq = 8; //determines how often the encoders are checked by tcnt0
uint8_t buttons = 0;
uint8_t servo_dir = 1;

uint8_t switch_state_servo = 0;
uint8_t old_switch_state_servo = 0;

void chk_buttons(void){
	switch_state_servo = (! bit_is_clear(PINA, 5)); //takes in current state of the button
}

void chk_encoder(void){
  	enc1_state = (! bit_is_clear(PINE,0));//get encoder one status
	enc2_state = (! bit_is_clear(PINE,1));//get encoder two status
	
	if(enc1_state != prev_state1){//if an edge is detected
	  switch(enc1_state){//check the state of the encoder
		case 0://if first encoder is low
		  switch(enc2_state){
			case 0://and the second is low
			  position = position - 1; //the second encoder read the black strip first, it is going left
			  break;
			case 1://and the second is still high
			  position = position + 1; //first encoder read black strip first, it is going right
			  break;
		  }
		  break;
		case 1://if first encoder is high
		  switch(enc2_state){
			case 0://and the second is still low
			  position = position + 1; //the first encoder read the clear strip first, it is going right
			  break;
			case 1://and the second is high
			  position = position - 1; //second encoder read clear strip first, it is going left
			  break;
		  }
		  break;	
	  }		
	  prev_state1 = enc1_state;//set current encoder state to be previous state for next run
	  PORTC = position;
	}
}

/***********************************************************************/
//                              tcnt0_init                             
//Initalizes timer/counter0 (TCNT0). TCNT0 runs with no prescaling.
//The interrupt for a compare match with OCR0A is enabled, which determines
//the frequency at which our encoders will be checked.
/*************************************************************************/
void tcnt0_init(void){
  TIFR0 |= (1<<OCF0A);	//enable timer/counter0 compA flag to be set
  TIMSK0  |=  (1<<OCIE0A); //enable timer/counter0 output compare match A interrupt
  TCCR0B  |=  (1<<CS00);  //no prescale
  OCR0A  = enc_freq;          //set when to first check encoders
}//tcnt0_init

/*************************************************************************/
//			     time/counter0 compare ISR
//Clk is at 1 MHz. Interrupt is set to run every enc_freq * (1/1MHz) seconds
//enc_freq must be a multiple of 8 so that when it reaches the limit of the 
//eight-bit integer, it overflows to zero to start the count over again. 
//************************************************************************/
ISR(TIMER0_COMPA_vect){
  chk_encoder();//check encoders for user input
  OCR0A += enc_freq;//setup next interrupt to check the encoders
}//TIMER_COMP_vect



void tcnt1_init(){
	TCCR1A |= (1<<WGM11) | (1<<COM1A1);//phase correct PWM with ICR3A, set on down count, clear on up count
	TCCR1B |= (1<<WGM13) | (1<<CS10);//no prescaling
	ICR1 = 10000;	//1MHZ/(2*10000) = 50Hz to get a 20ms period
	OCR1A = 300;	//0 degree state, 600us duty cycle
}

uint8_t main(){
  DDRA &= 0x00;//set to take input from buttons
  DDRB |= 0x20; //set PB5 as output, PB5 is PWM output  
  DDRC |= 0xFF;//setup LED's for position output
  DDRD |= 0xF0; //set PD4-PD7 as outputs to solenoids
  DDRE &= 0xFC;//PORTE PIN0 and PIN1 are the encoder inputs

  PORTC = position;
  PORTD &= 0x0F; //set outputs for solenoid initially low

  tcnt0_init();//setup encoder reading interrupt
  tcnt1_init(); //initialize pwm
  sei();//enable interrupts

  enc1_state = (! bit_is_clear(PINE,0));//read in initial states of encoders
  enc2_state = (! bit_is_clear(PINE,1));
  prev_state1 = enc1_state;//set current state as first previous state for comparison

  switch_state_servo = bit_is_set(PINA,5); //get initial state of the servo switch
  if(switch_state_servo != 0){
	//OCR3A = 1200;			//if state is high, set servo to 180 degrees
	OCR1A = 700;			//if state is high, set servo to 180 degrees
  }

  old_switch_state_servo = switch_state_servo;	//save state in old state to compare to in loop

  while(1){
		buttons = PINA;
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

	chk_buttons();		//check the current state
	if(switch_state_servo){//if the button is pressed
		if(servo_dir == 1){//if servo is going to the right
			OCR1A += 50;
			_delay_ms(10);
			if(OCR1A == 700){//if max rightward angle is reached
				servo_dir = 0;//rotate to the left next time
			}
		}else{
			OCR1A -= 50;
			_delay_ms(10);
			if(OCR1A == 300){//if max left angle is reached
				servo_dir = 1;//rotate to the right next time
			}
		}	
	}
  }
}

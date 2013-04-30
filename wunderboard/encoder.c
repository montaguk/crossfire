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


uint8_t main(){
  DDRA &= 0x00;
  DDRC |= 0xFF;//light LED's with position info for testing
  DDRE &= 0xFC;//PORTE PIN0 and PIN1 are the encoder inputs
  DDRD |= 0xFF;
  PORTD = PINA;

  tcnt0_init();//setup encoder reading interrupt
  sei();//enable interrupts
  enc1_state = (! bit_is_clear(PINE,0));//read in initial states of encoders
  enc2_state = (! bit_is_clear(PINE,1));
  prev_state1 = enc1_state;//set current state as first previous state for comparison
  PORTC = position;
  while(1){	
   	PORTD = PINA;
  }
}



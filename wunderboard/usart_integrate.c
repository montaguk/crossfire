
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#define RIGHT 1;
#define LEFT 0;

#define RIGHT_TOLERANCE 15
#define LEFT_TOLERANCE 15

uint8_t enc1_state;
uint8_t enc2_state;
uint8_t prev_state1;
volatile int16_t position = 7; //halfway point of position 
uint8_t enc_freq = 1; //determines how often the encoders are checked by tcnt0
uint8_t buttons = 0;

uint8_t switch_state_servo = 0;
uint8_t old_switch_state_servo = 0;
uint8_t switch_2 = 0;
uint8_t old_switch_2 = 0;
uint8_t switch_6 = 0;
uint8_t old_switch_6 = 0;
uint8_t direction = 0;
uint8_t moving = 0;
uint8_t pulse_move_right = 0;
uint8_t pulse_stop_right = 0;
volatile uint8_t tar_position = 7;
volatile uint8_t old_tar_position = 7;
volatile uint8_t pos_reached = 0;
uint8_t angle = 90;
uint8_t tar_angle = 90;
uint8_t old_angle = 90;
uint8_t temp_position = 7;
uint8_t rec_sig = '@';

/***********************************************************************/
//			chk_buttons
//Checks the button input from the buttons on PORTA
/***********************************************************************/
void chk_buttons(void){
	switch_state_servo = (! bit_is_clear(PINA, 5)); //takes in current state of the button
}//chk buttons

/***********************************************************************/
//			chk_encoder
//This function looks for a rising or falling edge from the first encoder.
//If an edge is found, then the position count will be increased or decreased 
//depending on the state of the second encoder.
/***********************************************************************/
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
//	  PORTC = position;
	  if(position == tar_position){
		pos_reached = 1;
	  }
	}
}//chk_encoder


/***********************************************************************/
//			move_right
//Moves the slide to the right.
/***********************************************************************/
void move_right(void){
  PORTD |= 0b01100000;	//set left pressure and right vent outputs high
  moving = 1;		//slide is now moving
  direction = 1;  	//"1" corresponds to moving right
  pulse_move_right = 1; //pulse off the valve in main function
}//move_right 

/***********************************************************************/
//			stop_right
//Stops the slide from moving to the right.
/***********************************************************************/
void stop_right(void){
  PORTD |= 0b00010000; //set right vent valve high
  moving = 0;	       //slide is no longer moving
  pulse_stop_right = 1;//pulse off the valve in the main function
}//move_right 

/***********************************************************************/
//			move_left
//Moves the slide to the left.
/***********************************************************************/
void move_left(void){
  PORTD |= 0b10010000;//set left pressure high, righ vent high
  moving = 1;	      //slide is now moving
  direction = 0;      //"0" corresponds to moving left
}//move_left 

/***********************************************************************/
//			stop_left
//Stop the slide from moving to the left.
/***********************************************************************/
void stop_left(void){
  PORTD &= 0b01101111;//turn off left pressure and right vent
  moving = 0;	      //slide is no longer moving
}//move_left 

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


/***********************************************************************/
//			tcnt1_init
//Initializes timer/counter 1 to be used to control the aiming servo.
//the clock is prescaled by 8 to run at 1MHz. The timer/counter is used in 
//phase correct PWM mode to output a duty cycle, within a constant clock cycle,
//relating to the angle the servo should be at.
/***********************************************************************/

void tcnt1_init(void){
	TCCR1A |= (1<<WGM11) | (1<<COM1A1);//phase correct PWM with ICR3A, set on down count, clear on up count
	TCCR1B |= (1<<WGM13) | (1<<CS10);//no prescale
	ICR1 = 10000;	//8MHZ/(2*8*10000) = 50Hz to get a 20ms period
	OCR1A = (angle*5 + 300);	//90 degree state, 1500us pulse width
}//tcnt1_init

//#if DEBUG == 1
/***********************************************************************/
/** This function needs to setup the variables used by the UART to enable the UART and tramsmit at 9600bps. This 
function should always return 0. Remember, by defualt the Wunderboard runs at 1mHz for its system clock.*/
/***********************************************************************/
unsigned char initializeUART(void)
{
	/* Set baud rate */
	UBRR1H = 0;
	UBRR1L = 12;
	/* Set the U2X1 bit */
	UCSR1A = (1 << U2X1);
	/* Enable transmitter */
	UCSR1B = (1 << TXEN1) | (1<<RXEN1);
	/* Set frame format: 8data, 1stop bit */
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
	return 0;
}
/***********************************************************************/
/** This function needs to write a single byte to the UART. It must check that the UART is ready for a new byte 
and return a 1 if the byte was not sent. 
@param [in] data This is the data byte to be sent.
@return The function returns a 1 or error and 0 on successful completion.*/
/***********************************************************************/
unsigned char sendByteUART(uint8_t data)
{
	while (!(UCSR1A & (1 << UDRE1)))
	{
	}
	UDR1 = data;
	return 0;
}

/***********************************************************************/
/** This function needs to writes a string to the UART. It must check that the UART is ready for a new byte and 
return a 1 if the string was not sent. 
@param [in] str This is a pointer to the data to be sent.
@return The function returns a 1 or error and 0 on successful completion.*/
/***********************************************************************/
unsigned char sendStringUART(char* str)
{
	while (*str)
	{
		sendByteUART(*str);
		str++;
	}
	return 0;
}

/*
#else
#define initializeUART()
#define sendByteUART( data)
#define sendStringUART(str)
#define printErrorUART(err)
#endif // DEBUG

*/
/***********************************************************************/
//			getByteUART
//Gets a byte from the computer through the UART.
/***********************************************************************/
unsigned char getByteUART(){
    if(UCSR1A & (1<<RXC1)){
	return(UDR1);
    } else {
	return(0);
    }
}//getByteUART

/***********************************************************************/
uint8_t main(void){
  DDRA &= 0x00;//set to take input from buttons
  DDRB |= 0x20; //set PB5 as output, PB5 is PWM output  
  DDRC |= 0xFF;//setup LED's for position output
  DDRD |= 0xF0; //set PD4-PD7 as outputs to solenoids
  DDRE &= 0xFC;//PORTE PIN0 and PIN1 are the encoder inputs

  //PORTC = position;//show position on LED's
  PORTD &= 0x0F; //set outputs for solenoid initially low

  tcnt0_init();//setup encoder reading interrupt
  tcnt1_init(); //initialize pwm
  initializeUART();//initialize UART connection
  sei();//enable interrupts

  enc1_state = (! bit_is_clear(PINE,0));//read in initial states of encoders
  enc2_state = (! bit_is_clear(PINE,1));
  prev_state1 = enc1_state;//set current state as first previous state for comparison


  while(1){
 	if(getByteUART() == rec_sig){//if new data has been sent from the computer
		while(!(temp_position = getByteUART())); //get the new target position
		while(!(tar_angle = getByteUART()));	 //get the new target angle
		if(moving == 0){tar_position = temp_position;}//if the slide is not moving, update the new target position
		//PORTC = tar_position;
	}
	if(pulse_move_right == 1){	//if the move right pressure valve has been turned on
		_delay_ms(1);
		PORTD &= 0b11011111;	//end the pulse to the valve pushes right
		pulse_move_right = 0;	//reset pulse indicator
	}
	if(pulse_stop_right == 1){	//if the valve to stop motion right is on
		_delay_ms(1);
		PORTD &= 0x00;		//end the pulse to the valve
		pulse_stop_right = 0;	//reset pulse indicator
	}			

	if(old_tar_position != tar_position){//if the target position has changed
		if(tar_position > position + RIGHT_TOLERANCE){//if the target position is to the right
			move_right();	    //move the slide right
		}else if (tar_position < position - LEFT_TOLERANCE){
			move_left(); 	    //otherwise, move the slide to the left
		} else {
			// Within tolerance, just sit
		}
		old_tar_position = tar_position;
	}

	if(tar_angle != angle){//if the angle has changed
		//map angle to PWM value: 0 = 300, 180 = 1200: OCR1A value = (angle*5)+300
		old_angle = angle;//set old_angle to current for comparison
		angle = tar_angle;
		OCR1A = (angle*5) + 300;
	}

	if((pos_reached == 1) && (moving == 1)){//if the position has been reached and the slide is moving
		if(direction == 1){//if the slide is moving to the right
			stop_right();  //stop the slide from moving right
		}else{
			stop_left();   //otherwise, stop it from moving left
		}
		pos_reached = 0;       //reset position reached variable
		//tar_position = position;//set target position to current position for overdamping
		old_tar_position = tar_position;//set old target equal to new target so next change can be seen
	}

	if (!bit_is_clear(PINA, 0)) {
		PORTC ^= 0x40;
	}

	char buf[10] = {0};
	
	sendStringUART(itoa(position, buf, 10)); //send current position to GUI
	sendByteUART(',');
	sendStringUART(itoa(angle, buf, 10));    //send current angle to GUI
	sendByteUART(',');
	sendStringUART(itoa(tar_position, buf, 10));
	sendStringUART("\r\n");	//send newline char to protect from data mixup
  }
}

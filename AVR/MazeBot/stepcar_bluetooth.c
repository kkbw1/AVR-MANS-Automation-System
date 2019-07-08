//#define _PRINT_

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef _PRINT_
#include <stdio.h>
#endif

#include "define.h"
#include "uart2.h"
#include "stepmotor.h"
#include "note.h"

#define STEP_STRAIGHT	346	
#define STEP_TURNR		241		// good
#define STEP_TURNL		241		// good

#define STEP_SPEED		2

#define STOP	255
#define TEMPO	900			

/*********************** Bluetooth ***********************/ 
const char* BTID_BOARD = "00190137A856";	
const char* BT_PINCODE = "MANS";
uchar flag_con;								// 연결 상태
uchar flag_chk;								// 수신 완료 상태
uchar flag_resp;							// 수신 진행 상태

uchar count_resp;							// 수신 메시지의 크기
char buff_resp[64];							// 수신 메시지 버퍼
/********************************************************/
/************************ MOTOR *************************/ 
uchar step1;
uchar step2;
/********************************************************/
/*********************** SPEKAER ************************/
uint num_note;
/********************************************************/

void init_var(void);
void init_port(void);
void init_timer(void);

void send_recvOK(void);

void set_level(unsigned int level);

ISR(USART1_RX_vect);
ISR(TIMER3_COMPA_vect);

ISR(USART1_RX_vect)
{
	char udr_temp = UDR1;
	
	if((udr_temp == CR) && (flag_resp == 0))			// 1st => First Start Character Check
	{
		flag_resp = 1;
	}
	else if((udr_temp == LF) && (flag_resp == 1))		// 2nd => Second Start Character Check
	{
		flag_resp = 2;
		count_resp = 0;
	}
	else if((udr_temp != CR) && (flag_resp == 2))		// 3rd => Get Receive Data
	{
		buff_resp[count_resp] = udr_temp;
		count_resp++;
	}
	else if((udr_temp == CR) && (flag_resp == 2))		// 4th => First End Character Check
	{
		flag_resp = 3;
	}
	else if((udr_temp == LF) && (flag_resp == 3))		// 5th => Second End Character Check
	{
		flag_resp = 4;
		buff_resp[count_resp] = '#';

		flag_chk = TRUE;
	}
}

ISR(TIMER3_COMPA_vect)
{
	TCNT3 = 0;

	TCCR1B &= ~(0x07);									// Timer1 Stop
	if(note[num_note] != STOP)
	{
		set_level(tone[note[num_note]]);				// Timer1 Start
		OCR3A = (unsigned int)TEMPO * note[num_note+1];
		num_note += 2;	
	}
	else
	{
		num_note = 0;
	}
}

int main(void)
{
	init_var();
	init_port();
	init_timer();
	#ifdef _PRINT_
	init_uart0();		// SCI to COM
	#endif
	init_uart1();		// SCI to BLUETOOTH
	
	sei();

	while(1)
	{
		if(flag_chk == TRUE)
		{
			if(flag_con == TRUE)
			{
				PORTA = 0x01;
				// STRAIGHT
				if(buff_resp[0] == 'S')
				{
					#ifdef _PRINT_
					printf("STRAIGHT\r\n");
					#endif
					
					if(step1 == 0)
					{
						step1 = STEP12_A;
					}
					if(step2 == 0)
					{
						step2 = STEP12_A;
					}
					PORTF = (step2 << 4) | step1;
					delay_ms(100);
					move_pulse12(DIR0, STEP_STRAIGHT, STEP_SPEED);
	
					delay_ms(100);
					step1 = PORTF & 0x0F;
					step2 = PORTF >> 4;
					PORTF = 0x00;
						
					PORTA = 0x00;

					send_recvOK();
				}
				// LEFT
				else if(buff_resp[0] == 'L')
				{
					#ifdef _PRINT_
					printf("LEFT TURN\r\n");
					#endif
					
					if(step1 == 0)
					{
						step1 = STEP12_A;
					}
					if(step2 == 0)
					{
						step2 = STEP12_A;
					}
					PORTF = (step2 << 4) | step1;
					delay_ms(100);
					move_pulse12(DIR2, STEP_TURNL, STEP_SPEED);
	
					delay_ms(100);
					step1 = PORTF & 0x0F;
					step2 = PORTF >> 4;
					PORTF = 0x00;
					
					PORTA = 0x00;
					
					send_recvOK();
				}
				// RIGHT
				else if(buff_resp[0] == 'R')
				{
					#ifdef _PRINT_
					printf("RIGHT TURN\r\n");
					#endif
					
					if(step1 == 0)
					{
						step1 = STEP12_A;
					}
					if(step2 == 0)
					{
						step2 = STEP12_A;
					}
					PORTF = (step2 << 4) | step1;
					delay_ms(100);
					move_pulse12(DIR3, STEP_TURNR, STEP_SPEED);
	
					delay_ms(100);
					step1 = PORTF & 0x0F;
					step2 = PORTF >> 4;
					PORTF = 0x00;
					
					PORTA = 0x00;

					send_recvOK();
				}
				// IR LED ON, Begin Detect Position
				else if((buff_resp[0] == 'I') && (buff_resp[1] == 'R') && (buff_resp[2] == 'O') && (buff_resp[3] == 'N'))
				{
					#ifdef _PRINT_
					printf("IR LED ON\r\n");
					#endif		
					
					PORTC = 0x07;				// IR LED ON	
					
					send_recvOK();
				}
				// IR LED OFF, End Detect Position
				else if((buff_resp[0] == 'I') && (buff_resp[1] == 'R') && (buff_resp[2] == 'O') && (buff_resp[3] == 'F'))
				{
					#ifdef _PRINT_
					printf("Detect Start Position\r\n");
					#endif
					
					PORTC = 0x00;				// IR LED OFF	
					
					send_recvOK();
				}
				// MUSIC ON
				else if((buff_resp[0] == 'M') && (buff_resp[1] == 'S') && (buff_resp[2] == 'O') && (buff_resp[3] == 'N'))
				{
					num_note = 0;

					OCR3A = TEMPO * note[1];
					TCCR3B |= 0x05;

					send_recvOK();
				}
				// MUSIC OFF
				else if((buff_resp[0] == 'M') && (buff_resp[1] == 'S') && (buff_resp[2] == 'O') && (buff_resp[3] == 'F'))
				{
					TCCR1B &= ~(0x07);				// Timer1 Stop
					TCCR3B &= ~(0x07);				// Timer3 Stop

					send_recvOK();
				}
				// STEPMOTOR BRAKE(No Freerunning)
				else if((buff_resp[0] == 'M') && (buff_resp[1] == 'T') && (buff_resp[2] == 'S') && (buff_resp[3] == 'T'))
				{
					if(step1 == 0)
					{
						step1 = STEP12_A;
					}
					if(step2 == 0)
					{
						step2 = STEP12_A;
					}
					PORTF = (step2 << 4) | step1;	

					send_recvOK();
				}
				// DISCONNECT
				else if((buff_resp[0] == 'D') && (buff_resp[1] == 'I') && (buff_resp[2] == 'S') && (buff_resp[3] == 'C'))
				{
					#ifdef _PRINT_
					printf("BoardBT Disconnected\r\n");
					#endif

					PORTA = 0x00;

					flag_con = FALSE;
				}
			}
			else if(flag_con == FALSE)
			{
				PORTA = 0x00;
				// CONNECT
				if((buff_resp[0] == 'C') && (buff_resp[1] == 'O') && (buff_resp[2] == 'N') && (buff_resp[3] == 'N'))
				{
					#ifdef _PRINT_
					printf("BoardBT Connected\r\n");
					#endif

					PORTA = 0x01;
					flag_con = TRUE;
				} 
			}

			flag_chk = FALSE;
			flag_resp = 0;
		}
	}

	return 0;
}

void send_recvOK(void)
{
	uart1_putch(CR);
	uart1_putch(LF);
	uart1_puts("GET");
	uart1_putch(CR);
	uart1_putch(LF);
}

void init_var(void)
{
	step1 = 0;
	step2 = 0;

	flag_con = FALSE;
	flag_chk = FALSE;
	flag_resp = 0;
	
	count_resp = 0;
}

void init_port(void)
{
	DDRA = 0xFF;		// LED
	DDRB = 0x20;		// OC1A Output
	DDRC = 0x0F;		// IR LED PC0,1,2
	DDRF = 0xFF;		// STEPMOTOR

	PORTA = 0x00;		
	PORTC = 0x00;		// IR LED PC0,1,2
	PORTF = 0x00;
}

void init_timer(void)
{
	TCCR1A = (0x02 << 6) | 0x02;	// Fast PWM Mode, OC1A핀 비교시 Low, Top시 High
	TCCR1B = (0x03 << 3);			// |= 0x02 // Fcpu/8 => 2MHz, 1count => 0.5usec

	TCCR3A = 0x00;					// Normal Mode, 핀 출력 사용X
	TCCR3B = 0x00;					// |= 0x05 // Fcpu/1024 => 15625Hz, 1count => 64usec

	ETIMSK = 0x10;					// Timer3 Compare A Enable
}

void set_level(unsigned int level)
{
	ICR1 = level;					// Sound Level 
	OCR1A = (unsigned int)level/2;	// Volume Maximum
	TCCR1B |= 0x02;					// Fcpu / 8, Start Timer

	PORTA = level;
}

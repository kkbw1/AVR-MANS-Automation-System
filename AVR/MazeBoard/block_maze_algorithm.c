#define _PRINT_
#define _LCD_
//#define 	_SCIRX_
#define _SCITX_
#define	_BLUETOOTH_

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef _PRINT_
#include <stdio.h>
#endif

#include "define.h"
#include "uart2.h"

#ifdef _LCD_
#include "clcd.h"
#endif

// LCD
#define CLR		"                    "	// 20 spaces

// Bluetooth Signal
#define CR		0x0D					// Carriage return	'\r'
#define LF		0x0A					// Line feed		'\n'

// ADC
#define THRESHOLD	150					//  Threshold of IR LED Sensing Data

// Map
#define NSTEP	8
#define ROW		8						// outer wall 2 blocks + actual 6 blocks, �ܺ� 2 + ��� 6 = 8
#define COL		10						// outer wall 2 blocks + actual 6 blocks + start line 1 blocks + finish line 1blocks, 
										// �ܺ� 2 + ��� 6 + ��ߺκ� 1 + ���κ� 1  = 10

#define ROAD	0						// Value of Road, �� 
#define WALL	99						// Value of Wall, �� 

// Road
#define FORWARD	0						// Driving Forward way, 	������ ���
#define REVERSE	1						// Driving Reversed way, 	������ ���

// Print
#define REF		0
#define OLD		1
#define NOW		2
#define FIN		3

/*-------------------------------------------------------*/
/*----------------------- VARIABLE ----------------------*/
/*-------------------------------------------------------*/
/*********************************************************/
/*********************** Bluetooth ***********************/ 
const char* BTID_CAR = "00190137A4C8";	// BTID of the maze robot, 		�������� ������ �������ID

uchar flag_con;							// Status of Connectino, 		���� ����
uchar flag_chk;							// Status of receive complete, 	���� �Ϸ� ����
uchar flag_resp;						// Status of receive progress,	���� ���� ����
uchar flag_confirm;						// Status of receive Command,	Ŀ��Ʈ ���� Ȯ�� ����

uchar count_resp;						// Current size of received message,	���� �޽����� ũ��
char buff_resp[64];						// Receive message buffer,		���� �޽��� ����
/*********************************************************/
/*********************** IR SENSOR ***********************/
uchar flag_adc;							// ���� ��ġ ���� ����
uchar flag_detect;						// ���� ���� ����
uchar count_sum;						// �հ� Ƚ��
uchar adc_chan;							// ADC ä��

uint  sum_adc[6];						// ä�δ� ADC �հ� ��, ���� => 0 ~ 2550
uchar data_adc[6];						// ä�δ� ADC �հ� ����� ��, ���� => 0 ~ 255

uchar x_start;							// ���� ��ġ x��ǥ
/*********************************************************/
/********************** Block Maze ***********************/
uchar map_ref[ROW][COL];				// ���۷��� ��
uchar map_prev[NSTEP][ROW][COL];		// ���� ��
uchar map_now[NSTEP][ROW][COL];			// ���� ��
uchar map_fin[4][ROW][COL];				// ���� ��

char carPath[64];						// ���� �ʿ� ���� ������ ������ �̵� ��� �迭
uchar carPath_length;					// �̵� ��� �迭�� ũ��

uchar flag_calc;						// ��ã�� �˰��� ���� ���� ����(?)
uchar flag_dst;							// �������������� ���� ���� ����
uchar x;								// �������� x��ǥ
uchar y;								// �������� y��ǥ
uchar step;						
uchar road_prev;						// map_prev
uchar road_now;							// map_now
uchar nStep_prev;						// map_now		���� ������ ����


/*----------------------------------------------------------------*/
/*---------------------------- FUNCTION --------------------------*/
/*----------------------------------------------------------------*/
void init_var(void);					// Initialize Variables, 	���� �ʱ�ȭ
void init_port(void);					// Initialize GPIO,			GPIO �ʱ�ȭ
void init_adc(void);					// Initialize ADC, 			ADC �ʱ�ȭ

void send_Data(char* str);				// Sending Bluetooth data,	��������� �����͸� ����
uchar recv_sigOK(void);					// Receiving ACK,			���� Ȯ�� ��ȣ�� ����
void wait_bluetooth(void);				// Waiting BT Connection,	���� Bluetooth ��ġ ���� ���
void detect_startPoint(void);			// Detect robot start pt,	������ ���� ��ġ ����
void create_map(void);					// Create map,				�� �ʱ�ȭ, ���� ���� ����.
void send_refMap(void);
void send_finalMap(void);
/******************** Functions in the main loop *****************/
void process_step(void);
void change_wall(void);
void copy_map (void);

void move_map(void);
void select_finalMap(void);				// select final path to drive,	

/************* Functions after calculation of paths **************/
void save_finalMap(void);				// Save the map that has final path,	�ִ� ��ΰ� ǥ�õ� �� ����
void make_carMovement(void);			// Create robot movement cmd,			�ִ� ��θ� ������ �ڵ��� �̵� ��� ����
void send_carMovement(uchar dir);		// Send robot movement cmd,				�ڵ��� �̵� ��� ����

/************************ Printing Fucntions *********************/
void printMap(uchar select);			// Print specific map
void printMapAll(uchar select);			// Print all maps
void printCarMovement(void);			// Print robot movement cmd in order	

ISR(USART1_RX_vect);
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// main function ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	#ifdef _PRINT_
	printf("AVR Start\r\n");
	#endif	// _PRINT_

	////////////////////////////////////////////////////
	// Initialization
	////////////////////////////////////////////////////	
	init_var();							// Variables Initialize
	init_port();						// GP I/O Initialize
	init_adc();							// AD Converter Initialize

	init_LCD();							// 4x20 Character LCD Initialize
	init_uart0();						// SCI to COM
	init_uart1();						// SCI to Bluetooth

	sei();
	////////////////////////////////////////////////////

	#ifdef _LCD_
	write_string(0, 0, "Team MANS");
	#endif	// _LCD_

	////////////////////////////////////////////////////
	// Waiting BT Connection
	////////////////////////////////////////////////////
	#ifdef _BLUETOOTH_
	#ifdef _LCD_
	write_string(0, 1, CLR);
	write_string(0, 2, CLR);
	write_string(0, 1, "Wait Connect CarBT");
	#endif	// _LCD_

	#ifdef	_PRINT_
	printf("Wait Connect Car Bluetooth.\r\n");
	#endif	// _PRINT_
	
	wait_bluetooth();

	#ifdef _PRINT_
	printf("BoardBT Connected.\r\n");
	#endif	// _PRINT_
	#endif	// _BLUETOOTH_
	////////////////////////////////////////////////////
	
	while(1)
	{
		////////////////////////////////////////////////////
		// Create map and detect obstacles
		////////////////////////////////////////////////////
		#ifdef _LCD_
		write_string(0, 1, CLR);
		write_string(0, 2, CLR);
		write_string(0, 1, "Push SW7 For Mapping");
		#endif	// _LCD_

		#ifdef _RPINT_
		printf("Press SW7 For Mapping.\r\n");
		#endif	// _PRINT_

		PORTA = 0xFF;
		// SW7���� ������ �� ���,
		while(SW7 == 0x00);				
		while(SW7 != 0x00);
		create_map();
		////////////////////////////////////////////////////

		////////////////////////////////////////////////////
		// Sensing robot starting point using Infrared Sensor
		////////////////////////////////////////////////////
		#ifndef _SCIRX_
		#ifdef _LCD_
		write_string(0, 1, CLR);
		write_string(0, 2, CLR);
		write_string(0, 1, "Start Detect Pos");
		#endif	// _LCD_

		#ifdef _PRINT_
		printf("Start Detect Position.\r\n");
		printf("IR LED On.\r\n");
		#endif	// _PRINT_

		send_Data("IRON");

		detect_startPoint();

		#ifdef _PRINT_
		printf("IR LED Off.\r\n");
		#endif	// _PRINT_

		send_Data("IROF");
		#endif	//	_SCIRX_
		////////////////////////////////////////////////////

		#ifdef _LCD_
		write_string(11, 1, "SET");
		write_string(0, 2, "Calculating Maze");
		#endif	// _LCD_

		#ifdef _PRINT_
		printf("Start Position => (%d, 1)\r\n", x_start);
		#endif	// _PRINT_

		////////////////////////////////////////////////////
		// Set starting point as step = 1
		////////////////////////////////////////////////////
		step = 1;							
		map_ref[x_start][1] = step;
		map_prev[1][x_start][1] = step;		
		nStep_prev = 1;
		////////////////////////////////////////////////////

		#ifdef _SCITX_
		send_refMap();
		#endif	// _SCITX_

		#ifdef _PRINT_
		printf("First Reference Map.\r\n");
		printMap(REF);
		#endif	// _PRINT_

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// execute finding path algorithm
		////////////////////////////////////////////////////////////////////////////////////////////////////
		flag_calc = TRUE;
		while(flag_calc)
		{
			road_prev = 1;
			road_now = 0;
			for(y = 1; y < 8; y++)
			{
				for(x = 1; x < 7; x++)
				{
					if(map_ref[x][y] == step) 
					{
						process_step();			// ���۷����ʿ��� ���� ������ ������ Step�� ã�� �� ������ �����Ѵ�.
						#ifdef _PRINT_
						printf("%d Step Map Now.\r\n", step);
						printMap(NOW);
						#endif	// _PRINT_
					}
				}
			}
			
			// ���� ���ܿ����� ���� ������ ������,
			if(flag_calc == FALSE)				// ���������� ����������,
			{
				select_finalMap();
				flag_dst = TRUE;

				break;
			}
			else if(road_now == 0)				// ���̻� ���� ������,
			{
				flag_calc = FALSE;
				flag_dst = FALSE;
			}
			else								// �� �� ���� �������� �Ѿ��,
			{
				move_map();
				nStep_prev = road_now;			// ���������������� ���� ���ܹ�ȣ�� �����ִµ� ����� ������ ����
				step++;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// ��� Ž���� ���� �� �˰��� (�ִ� ��� ���� & ���� ���� ���� & ���� ���� ����)
		// post processing algorithm after searching path (
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if(flag_dst == TRUE)
		{
			save_finalMap();					// �������������� ��ΰ� �ִ� ���� ���� ���� ����

			#ifdef _PRINT_
			printMap(REF);
			printMap(NOW);
			printMap(FIN);
			#endif	// _PRINT_

			make_carMovement();					// ���� ���� �̿��Ͽ� �ڵ����� �̵� ����� ���

			#ifdef _SCITX_
			send_finalMap();
			#endif

			#ifdef _PRINT_
			printCarMovement();
			#endif	// _PRINT_


			#ifdef _PRINT_
			printf("Step Motor Brake");
			#endif

			send_Data("MTST");					// ������ ���ܸ��͸� Brake ������.

			#ifdef _LCD_
			write_string(0, 1, CLR);
			write_string(0, 2, CLR);
			write_string(0, 1, "Complete Maze");
			write_string(0, 2, "Push SW7 to Move");
			#endif	// _LCD_

			#ifdef _PRINT_
			printf("Press SW7 Start Move.\r\n");
			#endif

			while(SW7 == 0x00);					// SW7�� ������, ���� ���� ��� ����
			if(SW7 != 0x00)
			{
				PORTA = 0x01;
				while(SW7 != 0x00);

				#ifdef _PRINT_
				printf("Forward Car Movement Start.\r\n");
				printf("Send Music On.\r\n");
				#endif

				send_Data("MSON");				// Music On

				#ifdef _BLUETOOTH_
				#ifdef _LCD_
				write_string(0, 1, CLR);
				write_string(0, 2, CLR);
				write_string(0, 1, "Forward Car Move");
				#endif	// _LCD_
				send_carMovement(FORWARD);		// �̵� ����� ������ ������� �۽�
				#endif	// _BLUETOOTH_

				#ifdef _LCD_
				write_string(0, 1, CLR);
				write_string(0, 2, CLR);
				write_string(0, 1, "Push SW6 to End");
				write_string(0, 2, "Push SW7 to Reverse");
				#endif	// _LCD_

				#ifdef _PRINT_
				printf("Send Music Off.\r\n");
				#endif

				send_Data("MSOF");				// Music Off

				while((PIND & 0xC0) == 0x00);	// 6���̳� 7���� ������ ���� ���, ���� ���� 
				delay_ms(1);					// ä�͸� ���� ������
				if(SW7 != 0x00)					// 7���� ������, ������ ������� ���� ���� ����
				{
					PORTA = 0x02;
					while(SW7 != 0x00);	

					#ifdef _PRINT_
					printf("Reverse Car Movement Start.\r\n");
					printf("Send Music On.\r\n");
					#endif

					send_Data("MSON");			// Music On

					#ifdef _BLUETOOTH_
					#ifdef _LCD_
					write_string(0, 1, CLR);
					write_string(0, 2, CLR);
					write_string(0, 1, "Reverse Car Move");
					#endif	// _LCD_
					send_carMovement(REVERSE);	// �̵� ����� ������ ������� �۽�
					#endif	// _BLUETOOTH_

					#ifdef _PRINT_
					printf("Send Music Off.\r\n");
					#endif

					send_Data("MSOF");			// Music Off
				}
			}

			#ifdef _LCD_
			write_string(0, 1, CLR);
			write_string(0, 2, CLR);
			write_string(0, 1, "Success All Process");
			write_string(0, 2, "Push SW7 to Restart");
			#endif	// _LCD_

			#ifdef _PRINT_
			printf("Success All Process.\r\n");
			printf("Press SW7 to Restart.\r\n");
			#endif	// _PRINT_
		}
		else
		{
			#ifdef _LCD_
			write_string(0, 1, CLR);
			write_string(0, 2, CLR);
			write_string(0, 1, "Can't Find Path");
			write_string(0, 2, "Push SW7 to Restart");
			#endif	// _LCD_	
			
			#ifdef _PRINT_
			printf("Can't Find Path.\r\n");
			printf("Press SW7 to Restart.\r\n");
			#endif	// _PRINT_	
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////

		while(SW7 == 0x00);		// SW7�� ������ �������, ���� ����
		while(SW7 != 0x00);		// SW7�� ������ ���� ���, ���� ����
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// End of main function ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void wait_bluetooth(void)
{
	PORTA = 0x00;
	while(flag_con == FALSE)
	{
		PORTA = 0x01;
		if(flag_chk == TRUE)
		{
			PORTA = 0x02;
			// CONNECT
			if((buff_resp[0] == 'C') && (buff_resp[1] == 'O') && (buff_resp[2] == 'N') && (buff_resp[3] == 'N'))
			{
				PORTA = 0x0F;

				flag_con = TRUE;
			}	 

			flag_chk = FALSE;
			flag_resp = 0;
		}
	}
}

void create_map(void)
{
	uchar a;
	uchar b;
	uchar c;
	uchar tmp;
	
	////////////////////////////////////////////////////
	// ��ü �� �ʱ�ȭ
	// Initialize entire map matrices
	////////////////////////////////////////////////////
	for(b = 0; b < 10; b++)
	{
		for(a = 0; a < 8; a++)
		{
			map_ref[a][b] = ROAD;	
		}
	}
	
	for(c = 0; c < 8; c++)
	{
		for(b = 0; b < 10; b++)
		{
			for(a = 0; a < 8; a++)
			{
				map_prev[c][a][b] = ROAD;
				map_now[c][a][b] = ROAD;
			}
		}
	}
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	// �׵θ� ��� �� ����
	// Create Outer walls
	////////////////////////////////////////////////////
	for(a = 0; a < 8; a++)
	{
		// Reference Map
		map_ref[a][0] = WALL;
		map_ref[a][9] = WALL;
		// First Map
		map_prev[1][a][0] = WALL;
		map_prev[1][a][9] = WALL;
	}
	for(a = 0; a < 10; a++)
	{
		// Reference Map
		map_ref[0][a] = WALL;
		map_ref[7][a] = WALL;
		// First Map
		map_prev[1][0][a] = WALL;
		map_prev[1][7][a] = WALL;
	}
	////////////////////////////////////////////////////

	#ifdef _SCIRX_
	////////////////////////////////////////////////////
	// PC������� ��ֹ� �ν��Ҷ�
	// Getting obstacles data from PC using Serial Comm.(*Test Case)
	////////////////////////////////////////////////////
	printf("Get Block From SCI to COM.\r\n");	

	// �������� ����
	while((tmp = uart0_getch()) != '@');
	tmp = uart0_getch();
	x_start = tmp + 1;
		
	// ��ϵ��� ���� ����
	a = 0;
	while((tmp = uart0_getch()) != '@');
	while((tmp = uart0_getch()) != '#')
	{
		map_ref[(a % 6) + 1][(a / 6) + 2] = tmp;
		map_prev[1][(a % 6) + 1][(a / 6) + 2] = tmp;
		a++;
	}
	////////////////////////////////////////////////////
	#else	// _SCIRX_
	////////////////////////////////////////////////////
	// ����ġ�� ��ֹ��� �ν��Ҷ�
	// Detecting obstacles from physical sensors in the real map
	////////////////////////////////////////////////////
	for(b = 0; b < 6; b++)						// ���ڴ� ��ǲ, Y, m.
	{
		PORTB = ((PORTB & 0xF0) | b);			// ���� 4bit�� ����, ���� 4bit�� ����
		delay_ms(1);							// �������� �����Ͱ��� �ޱ� ���� ������
	
		for(a = 0; a < 6; a++)					// ��Ƽ�÷��� ����Ʈ, X
		{
			PORTB = ((PORTB & 0x0F) | (a << 4));// ���� 4bit�� ����, ���� 4bit�� ����
			delay_ms(1);						// �������� �����Ͱ��� �ޱ� ���� ������
			tmp = (PINC & 0x01);
			switch(tmp)
			{
			case 0:								// ROAD
				map_ref[a + 1][b + 2] = ROAD;
				map_prev[1][a + 1][b + 2] = ROAD;
			break;
			case 1:								// WALL
				map_ref[a + 1][b + 2] = WALL;
				map_prev[1][a + 1][b + 2] = WALL;
			break;
			}
		}
	}
	////////////////////////////////////////////////////
	#endif	// _SCIRX_
}

void detect_startPoint(void)
{
	uchar i;
	
	flag_adc = TRUE;
	flag_detect = FALSE;

	adc_chan = 0;
	count_sum = 0;
	
	#ifdef _LCD_
	write_string(0, 1, CLR);
	write_string(0, 2, CLR);
	write_string(0, 1, "Start Point X: ");		// 15 blocks
	#endif

	while(flag_adc)
	{
		ADMUX = 0x20 + adc_chan;
		ADCSRA |= 0x40;
		while((ADCSRA & 0x10) == 0x00);
		sum_adc[adc_chan] += ADCH;

		adc_chan++;
		if(adc_chan > 5)
		{
			adc_chan = 0;
			count_sum++;

			if(count_sum > 9)
			{
				count_sum = 0;

				for(i = 0; i < 6; i++)
				{
					data_adc[i] = sum_adc[i] / 10;
					sum_adc[i] = 0;
				}

				for(i = 0; i < 6; i++)
				{
					if(data_adc[i] > THRESHOLD)
					{
						flag_detect = TRUE;
						x_start = i + 1;
	
						break;
					}
				}
				
				if(flag_detect == 1)				// Detect IR LED from the robot on starting point, ������ ����������
				{
					#ifdef _LCD_
					write_num1(15, 1, x_start);		// 1ĭ
					#endif

					PORTA = 0x01 << (5 - x_start);
					flag_detect = FALSE;

					if(SW7 != 0x00)
					{
						while(SW7 != 0x00)
						{
							PORTA = 0xFF;
						}
						
						PORTA = 0x00;
						flag_adc = FALSE;
					}
				}
				else								// No detect, ������ �������� ��������
				{
					#ifdef _LCD_
					write_string(15, 1, "X");
					#endif
					PORTA = 0x00;
				}
			}
		}
	}
}

void process_step(void)
{
	uchar n;

	// for���� �̿��ؼ� old�ʿ��� ���� ���ܹ�ȣ�� ���� ��ȣ�� ���� ����� ã�´�.
	for(n = 1; n <= nStep_prev; n++)	
	{
		if(map_prev[n][x][y] == step)
		{
			road_prev = n; 	// ���� ������ ���ȣ�� ã�´�.
		}
	}

	// ref�ʿ��� ���� ��ǥ�� �Ʒ������� ROAD�϶�
	if(map_ref[x][y + 1] == ROAD)
	{
		map_ref[x][y + 1] = step + 1;
		
		road_now++;
		copy_map();
		map_now[road_now][x][y + 1] = step + 1;
		change_wall();
		
		if(y == 7)	// ���� y��ǥ�� ���������� ����������(y + 1 == (End Row = 8))
		{
			flag_calc = FALSE;
		}
	}

	// ref�ʿ��� ���� ��ǥ�� �����ʹ����� ROAD�϶�
	if(map_ref[x + 1][y] == ROAD)
	{
		map_ref[x + 1][y] = step + 1;

		road_now++;
		copy_map();
		map_now[road_now][x + 1][y] = step + 1;
		change_wall();
	}

	// ref�ʿ��� ���� ��ǥ�� ���ʹ����� ROAD�϶�
	if(map_ref[x - 1][y] == ROAD)
	{
		map_ref[x - 1][y] = step + 1;
		
		road_now++;
		copy_map();
		map_now[road_now][x - 1][y] = step + 1;
		change_wall();
	}

	// ref�ʿ��� ���� ��ǥ�� ���ʹ����� ROAD�϶�
	if(map_ref[x][y - 1] == ROAD)
	{
		map_ref[x][y - 1] = step + 1;
		
		road_now++;
		copy_map();
		map_now[road_now][x][y - 1] = step + 1;
		change_wall();
	}
}

void copy_map (void)
{
	uchar a;	// X
	uchar b;	// Y

	for(b = 0; b < 10; b++)
	{
		for(a = 0; a < 8; a++)
		{
			map_now[road_now][a][b] = map_prev[road_prev][a][b]; // �������� ����
		}
	}
}

// �����ѹ���ܿ��� �������Ʈ�� ��ȭ ��Ų��.
void change_wall(void)
{
	if(map_now[road_now][x + 1][y] == ROAD)
	{
		map_now[road_now][x + 1][y] = WALL;
	}
	if(map_now[road_now][x - 1][y] == ROAD)
	{
		map_now[road_now][x - 1][y] = WALL;
	}
	if(map_now[road_now][x][y + 1] == ROAD)
	{
		map_now[road_now][x][y + 1] = WALL;
	}
	if(map_now[road_now][x][y - 1] == ROAD)
	{
		map_now[road_now][x][y - 1] = WALL;
	}
}

void move_map(void)
{
	uchar k;
	uchar a;	// X
	uchar b;	// Y

	for(k = 1; k <= road_now; k++)
	{
		for(b = 0; b < 10; b++)
		{
			for(a = 0; a < 8; a++)
			{
				map_prev[k][a][b] = map_now[k][a][b];	// step�� �ѱ����, �ش� step���� ������ ���� ���� �����Ѵ�.
				map_now[k][a][b] = ROAD;				// step�� �ѱ����, �ʱ�ȭ ��Ŵ.
			}
		}
	}
}

void select_finalMap(void)
{
	uchar f;			// Y
	uchar g;			// NSTEP
	uchar nRoad = 0;	// �ⱸ�� ���ϴ� ���� ������ ���� ����
	for(g = 1; g <= road_now; g++)
	{
		for(f = 1; f < 7; f++)
		{
			if (map_now[g][f][8] == (step + 1))
			{
				road_now = g;
				nRoad++;
			}
		}
	}
}

void save_finalMap(void)
{
	uchar a;	// Y
	uchar b;	// X

	for(b = 1; b < 9; b++)
	{
		for(a = 1; a < 7; a++)
		{
			if(map_ref[a][b] == map_now[road_now][a][b])	// ���� �ִ� ���� ���� ���� ��ȣ�� ����
			{
				map_fin[0][a][b] = map_now[road_now][a][b];
			}
			else											// �������� ROAD�� ����
			{
				map_fin[0][a][b] = ROAD;
			}
		}
	}
}

void make_carMovement(void)
{
	uchar step_movement = 2;
	
	uchar a = x_start;						// X
	uchar b = 1;							// Y
	uchar c = 0;							// Road Array Index
	
	uchar direction_prev = 0;				// �� ó�� �ٶ󺸴� ������ �Ʒ�����
	uchar direction_now;

	while(b != 8)
	{
		if (map_now[road_now][a][b + 1] == step_movement ) 
		{
			b++;
			direction_now = 0; 								// 0�� �Ʒ�
		}
		else if (map_now[road_now][a + 1][b] == step_movement)
		{
			a++;
			direction_now = 1; 								// 1�� ������
		}
		else if (map_now[road_now][a - 1][b] == step_movement)
		{
			a--;
			direction_now = 3; 								// 3�� ����
		}
		else if (map_now[road_now][a][b - 1] == step_movement)
		{
			b--;
			direction_now = 2; 								// 2�� ����
		}
		
		// ���� ���ܿ����� ������ ������ �ٶ󺸴� ����� ���� ���ܿ����� �ٶ󺸴� ������ ��
		if(direction_prev == direction_now)					// ���� �ٶ󺸴� ��ġ�� ���� ��ġ�� ����
		{
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now - 1))		// ���� ��ġ�� ���ʿ� ��ġ
		{
			carPath[c] = 'L';
			c++;
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now + 1))		// ���� ��ġ�� �����ʿ� ��ġ
		{
			carPath[c] = 'R';
			c++;
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now - 3))		// ���� ��ġ�� �����ʿ� ��ġ
		{
			carPath[c] = 'R';
			c++;
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now + 3))		// 
		{
			carPath[c] = 'L';
			c++;
			carPath[c] = 'S';
			c++;
		}

		step_movement++;
		direction_prev = direction_now;
	}
	carPath[c] = '#';
	carPath_length = c;
}

void send_carMovement(uchar dir)
{
	uchar a = 0;
	char move_temp;

	if(dir == REVERSE)
	{
		for(a = 0; a < 2; a++)
		{
			uart1_putch(CR);
			uart1_putch(LF);
			uart1_putch('L');
			uart1_putch(CR);
			uart1_putch(LF);
			recv_sigOK();
		}
	}

	a = 0;
	while(a != carPath_length)
	{
		switch(dir)
		{
		case FORWARD:
			move_temp = carPath[a];
			break;
		case REVERSE:
			if(carPath[carPath_length - (a + 1)] == 'L')
			{
				move_temp = 'R';
			}
			else if(carPath[carPath_length - (a + 1)] == 'R') 
			{
				move_temp = 'L';
			}
			else if(carPath[carPath_length - (a + 1)] == 'S')
			{
				move_temp = 'S';
			}
			break;
		}

		#ifdef _PRINT_
		printf("(Action => %c) Send\r\n", move_temp);
		#endif

		uart1_putch(CR);
		uart1_putch(LF);
		uart1_putch(move_temp);
		uart1_putch(CR);
		uart1_putch(LF);
		recv_sigOK();

		a++;
	}

	if(dir == REVERSE)
	{
		for(a = 0; a < 2; a++)
		{
			uart1_putch(CR);
			uart1_putch(LF);
			uart1_putch('L');
			uart1_putch(CR);
			uart1_putch(LF);

			recv_sigOK();
		}
	}

	#ifdef _PRINT_
	printf("Complete Send Movement.\r\n");
	#endif
}

void send_Data(char* str)
{
	uart1_putch(CR);
	uart1_putch(LF);
	uart1_puts(str);
	uart1_putch(CR);
	uart1_putch(LF);

	recv_sigOK();	
}

uchar recv_sigOK(void)
{
	flag_confirm = FALSE;
	while(flag_confirm == FALSE)
	{
		if(flag_chk == TRUE)
		{
			if((buff_resp[0] == 'G') && (buff_resp[1] == 'E') && (buff_resp[2] == 'T'))
			{
				#ifdef _PRINT_
				printf("Check Confirm Signal\r\n");
				#endif

				flag_confirm = TRUE;
				flag_chk = FALSE;
				flag_resp = 0;
			}
		}
	}

	return TRUE;
}

void send_refMap(void)
{
	uchar a;
	uchar b;

	uart0_putch('@');
	for(b = 1; b < 8; b++)
	{
		for(a = 1; a < 7; a++)
		{
			uart0_putch(map_ref[a][b]);
		}
	}
	uart0_putch('#');
}

void send_finalMap(void)
{
	uchar a;
	uchar b;

	uart0_putch('@');
	for(b = 1; b < 9; b++)
	{
		for(a = 1; a < 7; a++)
		{
			uart0_putch(map_now[road_now][a][b]);
		}
	}
	uart0_putch('#');
}

#ifdef _PRINT_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Printing Functions ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void printMap(uchar select)
{
	uchar a;	// X
	uchar b;	// Y

	switch(select)
	{
		case REF:
			printf("Reference Map\r\n");
			for(b = 1; b < 9; b++)
			{
				for(a = 1; a < 7; a++)
				{
					switch(map_ref[a][b])
					{
					case ROAD:
						printf("RR ");
					break;
					case WALL:
						printf("WW ");
					break;
					default:
						printf("%2d ", map_ref[a][b]);
					break;
					}
				}
	
				if(b == 1) { printf("  <== START"); }
				else if(b == 8) { printf("  <== END"); }
				printf("\r\n");
			}
			printf("Present Position => (%d, %d)\r\n", x, y);
			printf("==============================\r\n");
		break;
		case NOW:
			printf("Present Map\r\n");
			for(b = 1; b < 9; b++)
			{
				for(a = 1; a < 7; a++)
				{
					switch(map_now[road_now][a][b])
					{
					case ROAD:
						printf("RR ");
					break;
					case WALL:
						printf("WW ");
					break;
					default:
						printf("%2d ", map_now[road_now][a][b]);
					break;
					}
				}

				if(b == 1) { printf("  <== START"); }
				else if(b == 8) { printf("  <== END"); }
				printf("\r\n");
			}	
			printf("==============================\r\n");
		break;
		case FIN:
			printf("Finish Map\r\n");
			for(b = 1; b < 9; b++)
			{
				for(a = 1; a < 7; a++)
				{
					switch(map_fin[0][a][b])
					{
					case ROAD:
						printf("RR ");
					break;
					case WALL:	
						printf("WW ");
					break;
					default:
						printf("%2d ", map_fin[0][a][b]);
					break;
					}
				}

				if(b == 1) { printf("  <== START"); }
				else if(b == 8) { printf("  <== END"); }
				printf("\r\n");
			}	
			printf("==============================\r\n");
		break;
	}
}

void printMapAll(uchar select)
{
	uchar a;	// X
	uchar b;	// Y
	uchar c;	// STEP
	
	printf("Print Map All\r\n");
	for(c = 1; c < NSTEP; c++)
	{
		printf("%d Map.\r\n", c);
		for(b = 1; b < 9; b++)
		{
			for(a = 1; a < 7; a++)
			{
				if(select == OLD)
				{
					switch(map_prev[c][a][b])
					{
					case ROAD:	
					printf("RR ");
						break;
					case WALL:
						printf("WW ");
						break;
					default:
						printf("%2d ", map_prev[c][a][b]);
					break;
					}
				}
				else if(select == NOW)
				{
					switch(map_now[c][a][b])
					{
					case ROAD:	
					printf("RR ");
						break;
					case WALL:
						printf("WW ");
						break;
					default:
						printf("%2d ", map_now[c][a][b]);
					break;
					}
				}
			}

			if(b == 1) { printf("  <== START"); }
			else if(b == 8) { printf("  <== END"); }
			printf("\r\n");
		}	
		printf("==============================\r\n");
	}
}

void printCarMovement(void)
{
	uchar a = 0;

	printf("Print Car Movement\r\n");
	while(1)
	{
		printf("%c", carPath[a]);
		if(carPath[a + 1] == '#')
		{
			break;
		}
		else
		{
			printf(" - ");
			a++;
		}
	}
	printf("\r\n");
	printf("Length => %d.\r\n", carPath_length);
}
#endif // #ifdef _PRINT_

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Initialization functions //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void init_var(void)
{
	flag_con = FALSE;			// ���� ����
	flag_chk = FALSE;			// ���� �Ϸ� ����
	flag_resp = 0;				// ���� ���� ����

	count_resp = 0;				// Current size of received message,	���� �޽����� ũ��	
}

void init_port(void)
{
	DDRA = 0xFF;				// LED PORT
	DDRB = 0xFF;				// Lower 4bits => Decoder input, Higher 4bits=> Mux Select, ���� 4bit => ���ڴ� ��ǲ, ���� 4bit => ��Ƽ�÷��� ����Ʈ
	DDRC = 0x00;				// PC0 = > Data input, PC0 => ������ ��ǲ
	DDRD = 0x00;				// BUTTON
	DDRE = 0xFF;				// Character LCD PORT

	PORTA = 0x00;				// LED PORT
	PORTB = 0x00;				// Lower 4bits => Decoder input, Higher 4bits=> Mux Select, ���� 4bit => ���ڴ� ��ǲ, ���� 4bit => ��Ƽ�÷��� ����Ʈ
	PORTE = 0x00;				// Character LCD PORT
}

void init_adc(void)
{
	ADMUX = 0x20;				// ADC Data Left Adjust
	ADCSRA = 0x87;				// ADC Enable, Prescaler 128, Convert Speed => 16MHz / 128 => 125kHz => 8us
}

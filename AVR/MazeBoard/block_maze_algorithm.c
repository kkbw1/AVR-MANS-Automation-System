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
#define ROW		8						// outer wall 2 blocks + actual 6 blocks, 외벽 2 + 블록 6 = 8
#define COL		10						// outer wall 2 blocks + actual 6 blocks + start line 1 blocks + finish line 1blocks, 
										// 외벽 2 + 블록 6 + 출발부분 1 + 끝부분 1  = 10

#define ROAD	0						// Value of Road, 길 
#define WALL	99						// Value of Wall, 벽 

// Road
#define FORWARD	0						// Driving Forward way, 	정방향 경로
#define REVERSE	1						// Driving Reversed way, 	역방향 경로

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
const char* BTID_CAR = "00190137A4C8";	// BTID of the maze robot, 		모형차에 장착된 블루투스ID

uchar flag_con;							// Status of Connectino, 		연결 상태
uchar flag_chk;							// Status of receive complete, 	수신 완료 상태
uchar flag_resp;						// Status of receive progress,	수신 진행 상태
uchar flag_confirm;						// Status of receive Command,	커맨트 수신 확인 상태

uchar count_resp;						// Current size of received message,	수신 메시지의 크기
char buff_resp[64];						// Receive message buffer,		수신 메시지 버퍼
/*********************************************************/
/*********************** IR SENSOR ***********************/
uchar flag_adc;							// 차량 위치 지정 상태
uchar flag_detect;						// 차량 감지 상태
uchar count_sum;						// 합계 횟수
uchar adc_chan;							// ADC 채널

uint  sum_adc[6];						// 채널당 ADC 합계 값, 범위 => 0 ~ 2550
uchar data_adc[6];						// 채널당 ADC 합계 평균한 값, 범위 => 0 ~ 255

uchar x_start;							// 시작 위치 x좌표
/*********************************************************/
/********************** Block Maze ***********************/
uchar map_ref[ROW][COL];				// 레퍼런스 맵
uchar map_prev[NSTEP][ROW][COL];		// 이전 맵
uchar map_now[NSTEP][ROW][COL];			// 현재 맵
uchar map_fin[4][ROW][COL];				// 최종 맵

char carPath[64];						// 최종 맵에 따라 생성한 차량의 이동 명령 배열
uchar carPath_length;					// 이동 명령 배열의 크기

uchar flag_calc;						// 길찾기 알고리즘 연산 수행 상태(?)
uchar flag_dst;							// 도착지점까지의 존재 유무 상태
uchar x;								// 연산중인 x좌표
uchar y;								// 연산중인 y좌표
uchar step;						
uchar road_prev;						// map_prev
uchar road_now;							// map_now
uchar nStep_prev;						// map_now		이전 스텝의 갯수


/*----------------------------------------------------------------*/
/*---------------------------- FUNCTION --------------------------*/
/*----------------------------------------------------------------*/
void init_var(void);					// Initialize Variables, 	변수 초기화
void init_port(void);					// Initialize GPIO,			GPIO 초기화
void init_adc(void);					// Initialize ADC, 			ADC 초기화

void send_Data(char* str);				// Sending Bluetooth data,	블루투스로 데이터를 전송
uchar recv_sigOK(void);					// Receiving ACK,			수신 확인 신호를 받음
void wait_bluetooth(void);				// Waiting BT Connection,	차량 Bluetooth 장치 접속 대기
void detect_startPoint(void);			// Detect robot start pt,	차량의 시작 위치 감지
void create_map(void);					// Create map,				맵 초기화, 벽과 길을 생성.
void send_refMap(void);
void send_finalMap(void);
/******************** Functions in the main loop *****************/
void process_step(void);
void change_wall(void);
void copy_map (void);

void move_map(void);
void select_finalMap(void);				// select final path to drive,	

/************* Functions after calculation of paths **************/
void save_finalMap(void);				// Save the map that has final path,	최단 경로가 표시된 맵 저장
void make_carMovement(void);			// Create robot movement cmd,			최단 경로를 가지는 자동차 이동 명령 생성
void send_carMovement(uchar dir);		// Send robot movement cmd,				자동차 이동 명령 전송

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
		// SW7번을 눌렀다 땔 경우,
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
						process_step();			// 레퍼런스맵에서 현재 연산을 수행할 Step을 찾은 후 연산을 시작한다.
						#ifdef _PRINT_
						printf("%d Step Map Now.\r\n", step);
						printMap(NOW);
						#endif	// _PRINT_
					}
				}
			}
			
			// 현재 스텝에서의 연산 과정이 끝난후,
			if(flag_calc == FALSE)				// 도착영역에 도달했을때,
			{
				select_finalMap();
				flag_dst = TRUE;

				break;
			}
			else if(road_now == 0)				// 더이상 길이 없을때,
			{
				flag_calc = FALSE;
				flag_dst = FALSE;
			}
			else								// 그 외 다음 스텝으로 넘어갈때,
			{
				move_map();
				nStep_prev = road_now;			// 시작지점에서부터 현재 스텝번호가 적혀있는데 블록의 갯수를 저장
				step++;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// 경로 탐색이 끝난 후 알고리즘 (최단 경로 생성 & 차량 동작 생성 & 차량 동작 전송)
		// post processing algorithm after searching path (
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if(flag_dst == TRUE)
		{
			save_finalMap();					// 도착지점까지의 경로가 있는 최종 맵을 따로 저장

			#ifdef _PRINT_
			printMap(REF);
			printMap(NOW);
			printMap(FIN);
			#endif	// _PRINT_

			make_carMovement();					// 최종 맵을 이용하여 자동차의 이동 명령을 계산

			#ifdef _SCITX_
			send_finalMap();
			#endif

			#ifdef _PRINT_
			printCarMovement();
			#endif	// _PRINT_


			#ifdef _PRINT_
			printf("Step Motor Brake");
			#endif

			send_Data("MTST");					// 차량의 스텝모터를 Brake 시켜줌.

			#ifdef _LCD_
			write_string(0, 1, CLR);
			write_string(0, 2, CLR);
			write_string(0, 1, "Complete Maze");
			write_string(0, 2, "Push SW7 to Move");
			#endif	// _LCD_

			#ifdef _PRINT_
			printf("Press SW7 Start Move.\r\n");
			#endif

			while(SW7 == 0x00);					// SW7번 누르면, 차량 동작 명령 전송
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
				send_carMovement(FORWARD);		// 이동 명령을 정주행 방식으로 송신
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

				while((PIND & 0xC0) == 0x00);	// 6번이나 7번을 누르지 않을 경우, 무한 루프 
				delay_ms(1);					// 채터링 방지 딜레이
				if(SW7 != 0x00)					// 7번을 누르면, 역주행 방식으로 차량 동작 전송
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
					send_carMovement(REVERSE);	// 이동 명령을 역주행 방식으로 송신
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

		while(SW7 == 0x00);		// SW7을 누르지 않을경우, 무한 루프
		while(SW7 != 0x00);		// SW7을 누르고 있을 경우, 무한 루프
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
	// 전체 맵 초기화
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
	// 테두리 경계 벽 생성
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
	// PC통신으로 장애물 인식할때
	// Getting obstacles data from PC using Serial Comm.(*Test Case)
	////////////////////////////////////////////////////
	printf("Get Block From SCI to COM.\r\n");	

	// 시작지점 수신
	while((tmp = uart0_getch()) != '@');
	tmp = uart0_getch();
	x_start = tmp + 1;
		
	// 블록들의 상태 수신
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
	// 스위치로 장애물을 인식할때
	// Detecting obstacles from physical sensors in the real map
	////////////////////////////////////////////////////
	for(b = 0; b < 6; b++)						// 디코더 인풋, Y, m.
	{
		PORTB = ((PORTB & 0xF0) | b);			// 상위 4bit는 유지, 하위 4bit만 변경
		delay_ms(1);							// 안정적인 데이터값을 받기 위한 딜레이
	
		for(a = 0; a < 6; a++)					// 멀티플렉서 셀렉트, X
		{
			PORTB = ((PORTB & 0x0F) | (a << 4));// 하위 4bit는 유지, 상위 4bit만 변경
			delay_ms(1);						// 안정적인 데이터값을 받기 위한 딜레이
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
				
				if(flag_detect == 1)				// Detect IR LED from the robot on starting point, 차량을 감지했을때
				{
					#ifdef _LCD_
					write_num1(15, 1, x_start);		// 1칸
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
				else								// No detect, 차량을 감지하지 못했을때
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

	// for문을 이용해서 old맵에서 이전 스텝번호와 같은 번호를 갖는 블록을 찾는다.
	for(n = 1; n <= nStep_prev; n++)	
	{
		if(map_prev[n][x][y] == step)
		{
			road_prev = n; 	// 이전 스텝의 길번호를 찾는다.
		}
	}

	// ref맵에서 현재 좌표의 아래방향이 ROAD일때
	if(map_ref[x][y + 1] == ROAD)
	{
		map_ref[x][y + 1] = step + 1;
		
		road_now++;
		copy_map();
		map_now[road_now][x][y + 1] = step + 1;
		change_wall();
		
		if(y == 7)	// 현재 y좌표가 도착영역에 도달했을때(y + 1 == (End Row = 8))
		{
			flag_calc = FALSE;
		}
	}

	// ref맵에서 현재 좌표의 오른쪽방향이 ROAD일때
	if(map_ref[x + 1][y] == ROAD)
	{
		map_ref[x + 1][y] = step + 1;

		road_now++;
		copy_map();
		map_now[road_now][x + 1][y] = step + 1;
		change_wall();
	}

	// ref맵에서 현재 좌표의 왼쪽방향이 ROAD일때
	if(map_ref[x - 1][y] == ROAD)
	{
		map_ref[x - 1][y] = step + 1;
		
		road_now++;
		copy_map();
		map_now[road_now][x - 1][y] = step + 1;
		change_wall();
	}

	// ref맵에서 현재 좌표의 위쪽방향이 ROAD일때
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
			map_now[road_now][a][b] = map_prev[road_prev][a][b]; // 기존맵을 복사
		}
	}
}

// 진행한방향외에는 모든방향루트를 벽화 시킨다.
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
				map_prev[k][a][b] = map_now[k][a][b];	// step을 넘기기전, 해당 step에서 진행한 맵을 따로 저장한다.
				map_now[k][a][b] = ROAD;				// step을 넘기기전, 초기화 시킴.
			}
		}
	}
}

void select_finalMap(void)
{
	uchar f;			// Y
	uchar g;			// NSTEP
	uchar nRoad = 0;	// 출구로 향하는 최적 스텝의 길의 갯수
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
			if(map_ref[a][b] == map_now[road_now][a][b])	// 원래 있던 벽과 현재 맵의 번호만 저장
			{
				map_fin[0][a][b] = map_now[road_now][a][b];
			}
			else											// 나머지는 ROAD로 저장
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
	
	uchar direction_prev = 0;				// 맨 처음 바라보는 방향은 아래방향
	uchar direction_now;

	while(b != 8)
	{
		if (map_now[road_now][a][b + 1] == step_movement ) 
		{
			b++;
			direction_now = 0; 								// 0은 아래
		}
		else if (map_now[road_now][a + 1][b] == step_movement)
		{
			a++;
			direction_now = 1; 								// 1은 오른쪽
		}
		else if (map_now[road_now][a - 1][b] == step_movement)
		{
			a--;
			direction_now = 3; 								// 3은 왼쪽
		}
		else if (map_now[road_now][a][b - 1] == step_movement)
		{
			b--;
			direction_now = 2; 								// 2는 위쪽
		}
		
		// 이전 스텝에서의 차량의 전면이 바라보는 방향과 현재 스텝에서의 바라보는 방향을 비교
		if(direction_prev == direction_now)					// 이전 바라보는 위치랑 현재 위치랑 같음
		{
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now - 1))		// 현재 위치가 왼쪽에 위치
		{
			carPath[c] = 'L';
			c++;
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now + 1))		// 현재 위치가 오른쪽에 위치
		{
			carPath[c] = 'R';
			c++;
			carPath[c] = 'S';
			c++;
		}
		else if(direction_prev == (direction_now - 3))		// 현재 위치가 오른쪽에 위치
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
	flag_con = FALSE;			// 연결 상태
	flag_chk = FALSE;			// 수신 완료 상태
	flag_resp = 0;				// 수신 진행 상태

	count_resp = 0;				// Current size of received message,	수신 메시지의 크기	
}

void init_port(void)
{
	DDRA = 0xFF;				// LED PORT
	DDRB = 0xFF;				// Lower 4bits => Decoder input, Higher 4bits=> Mux Select, 하위 4bit => 디코더 인풋, 상위 4bit => 멀티플렉서 셀렉트
	DDRC = 0x00;				// PC0 = > Data input, PC0 => 데이터 인풋
	DDRD = 0x00;				// BUTTON
	DDRE = 0xFF;				// Character LCD PORT

	PORTA = 0x00;				// LED PORT
	PORTB = 0x00;				// Lower 4bits => Decoder input, Higher 4bits=> Mux Select, 하위 4bit => 디코더 인풋, 상위 4bit => 멀티플렉서 셀렉트
	PORTE = 0x00;				// Character LCD PORT
}

void init_adc(void)
{
	ADMUX = 0x20;				// ADC Data Left Adjust
	ADCSRA = 0x87;				// ADC Enable, Prescaler 128, Convert Speed => 16MHz / 128 => 125kHz => 8us
}

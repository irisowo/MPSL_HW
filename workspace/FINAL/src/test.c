#include "stm32l476xx.h"
#include "core_cm4.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include "motor1.h"
#include <string.h>
#define SET_REG(REG, MASK, VAL) {((REG)=((REG) & (~(MASK))) | (VAL));};
char msg[105];
//=============================================================================
int UART_Transmit(uint8_t *arr, uint32_t size){
 //TODO: Sent string to UART and return how many bytes are successfully transmitted.
 for(unsigned int i=0; i<size; i++){
  USART3->TDR = *(arr+i);
  while((USART3->ISR & USART_ISR_TC) == 0){}
 }
 return size;
}

char last;
int UART_Receive(char *c){
 int cnt=0;
 while (!(USART3->ISR & USART_ISR_RXNE));
 //USART3->ISR = USART3->ISR & ~USART_ISR_RXNE;
 *c = USART3->RDR & 0xFF;
 return *c;
}

void readline() {
 char c;
 int pos = 0;
 do {
  UART_Receive(&c);
  if (c == '\r') {
   //UART_Transmit((uint8_t *) "\r\n", 2);
   c = '\0';
  }
   msg[pos] = c;
   pos++;
   last=c;

 } while (c != '\0');

}


void init_UART() {
// Initialize UART registers
 RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
 SET_REG(USART3->CR1, USART_CR1_M | USART_CR1_PS | USART_CR1_PCE | USART_CR1_TE | USART_CR1_RE | USART_CR1_OVER8, USART_CR1_TE | USART_CR1_RE);
 SET_REG(USART3->CR2, USART_CR2_STOP, 0x0);
 SET_REG(USART3->CR3, (USART_CR3_RTSE | USART_CR3_CTSE | USART_CR3_ONEBIT), 0x0);


 USART3->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
 USART3->CR3 &= ~(USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN);

 USART3->BRR &= ~USART_BRR_DIV_MANTISSA;
  USART3->BRR |= (26 << 4);
  USART3->BRR &= ~USART_BRR_DIV_FRACTION;
  USART3->BRR |= (0000 << 0);
 // Enable UART
 USART3->CR1 |= (USART_CR1_UE);

}

void GPIO_init(){
 RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

 SET_REG(GPIOC->MODER, GPIO_MODER_MODE10, 2 << 20);
 SET_REG(GPIOC->AFR[1], GPIO_AFRH_AFSEL10, GPIO_AFRH_AFSEL10_0 | GPIO_AFRH_AFSEL10_1 | GPIO_AFRH_AFSEL10_2);
 SET_REG(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED10, 2 << 20);

 SET_REG(GPIOC->MODER, GPIO_MODER_MODE11, 2 << 22);
 SET_REG(GPIOC->AFR[1], GPIO_AFRH_AFSEL11, GPIO_AFRH_AFSEL11_0 | GPIO_AFRH_AFSEL11_1 | GPIO_AFRH_AFSEL11_2);
 SET_REG(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED11, 2 << 22);


 SET_REG(GPIOC->MODER, GPIO_MODER_MODE13, 0 << 26);

 RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
 SET_REG(GPIOA->MODER, GPIO_MODER_MODE5, 1 << 10);

}

int check_button(){
 int cnt = 0;
 if(GPIOC->IDR & (0x1 << 13)) return 0;
 while(!(GPIOC->IDR & (0x1 << 13)) && cnt++ < 1000);
 return !(GPIOC->IDR & (0x1 << 13));
}
//===========================================================================
//extern void GPIO_init();
int duty=50;

void delay_ms(int ms){
	while(ms--);
	return;
}

void straight(){
	Motor_1_PRun(); //正转
	Motor_2_PRun();
	delay_ms(120000);
	return;
 }

void turn_right(){
	Motor_1_NRun();//右转
	delay_ms(120000);
}

void round_right(){
	Motor_1_NRun();//右转
	delay_ms(150000);
}

void turn_left(){
	Motor_2_NRun();//左转
	delay_ms(200000);
}

void round_left(){
	Motor_2_NRun();//左转
	delay_ms(200000);
}

void backward(){
	Motor_1_NRun(); //正转
	Motor_2_NRun();
	delay_ms(200000);
}

 void stop(){
	Motor_1_STOP();  //停止
	Motor_2_STOP();
 }

#define MODER_AF 0b10
#define LED_PIN 0b1

void GPIO_init_AF(){
 RCC->AHB2ENR |= 0x3;
 //GPIOB->MODER |= GPIO_MODER_MODE1_1;
 //GPIOB->AFR[0] |= GPIO_AFRL_AFSEL1_0; //PORT1 PIN4

 GPIOA->MODER &= (~(0b11 << LED_PIN * 2));
 GPIOA->MODER |= (MODER_AF << LED_PIN * 2);

 GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL1; // PA1 mask
 GPIOA->AFR[0] |= GPIO_AFRL_AFSEL1_0; // PA1: TIM2_CH2: AF1
}

void GPIO_init_AF2(){
 RCC->AHB2ENR |= 0x3;
 GPIOB->MODER |= GPIO_MODER_MODE3_1;
 GPIOB->AFR[0] |= GPIO_AFRL_AFSEL3_0; //PORT1 PIN4
}

void Timer_init(TIM_TypeDef *timer){
 RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
 timer->CR1 |= TIM_CR1_ARPE;//enable auto-reload preload
 timer->PSC = (uint32_t)399;
 timer->ARR = (uint32_t) 99;
 // Capture/Compare 2 output enable
 timer->CCER |= TIM_CCER_CC2E;
 // [6:4] 0111: PWM2
 timer->CCMR1 |= ( (TIM_CCMR1_OC2M & 0xFFFFFF8F) | (0b01110000) );
 timer->CCR2 = duty;
 timer->EGR = TIM_EGR_UG; // reinit counter
 timer->CCMR1 |= TIM_CCMR1_OC2PE;//preload enable
}


int main(void)
{
 	Motor_12_Config();
	TIM_TypeDef *timer = TIM2;
	Timer_init(timer);
	GPIO_init_AF();
	GPIO_init_AF2();
	timer->CR1 |= TIM_CR1_CEN;
	//=======================
	GPIO_init();
	init_UART();
	UART_Transmit("AT+ROLE=0\r\n", 11);
	UART_Transmit("AT+CMODE=1\r\n", strlen("AT+CMODE=1\r\n"));

	//=======================

	 while(1){
	  msg[0]='\0';
	  readline();
	  //UART_Transmit(msg, strlen(msg));

	  if (!strcmp(msg, "l")){
		  turn_left();
		  stop();
		  //UART_Transmit("left ok\r\n", strlen("left ok\r\n"));
	  }
	  else if (!strcmp(msg, "r")) {
		  turn_right();
		  stop();
		  //UART_Transmit("right ok\r\n", strlen("right ok\r\n"));
	  }
	  else if (!strcmp(msg, "s")) {
		  straight();
		  stop();
		  //UART_Transmit("straight ok\r\n", strlen("straight ok\r\n"));
	  }
	  else if (!strcmp(msg, "b")) {
		  backward();
		  stop();
		  //UART_Transmit("backward\r\n", strlen("backward\r\n"));
	  }
	  else if (!strcmp(msg, "i")) {
		  if(duty>0) duty-=10;
		  timer->CCR2 = duty;
		  //UART_Transmit("increase ok\r\n", strlen("increase ok\r\n"));
	  }
	  else if (!strcmp(msg, "d")) {
		  if(duty<100) duty+=10;
		  timer->CCR2 = duty;
		  //UART_Transmit("decrease ok\r\n", strlen("decrease ok\r\n"));
	  }
	  else if (!strcmp(msg, "R")) {
		  round_right();
		  stop();
		  //UART_Transmit("Rround ok\r\n", strlen("Rround ok\r\n"));
	  }
	  else if (!strcmp(msg, "L")) {
		  round_left();
		  stop();
		  //UART_Transmit("Lround ok\r\n", strlen("Lround ok\r\n"));
	  }
	  else{
		  continue;
	  }
	 }

	//=======================
	 /*
	 while(1){
		straight();
		stop();
		delay_ms(1000000);

		backward();
		stop();
		delay_ms(1000000);

		turn_right();
		stop();
		delay_ms(1000000);

		turn_left();
		stop();
		delay_ms(1000000);
	}
	*/
	return 0;
}



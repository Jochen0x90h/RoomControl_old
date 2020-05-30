#include "system/stm32f042x6.h"
#include <stdbool.h>

// data sheet: https://www.st.com/resource/en/datasheet/stm32f042f6.pdf
// refernece manual: https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

char const staticId[4] = {0x3e, 0x74, 0xf0, 0x19};

// configuration
int const USART1_CLOCK = 8000000;
int const USART1_BAUDRATE = 19200;
int const TIM1_CLOCK = 8000000;
int const TIM2_CLOCK = 8000000;
int const TIM3_CLOCK = 8000000;

bool jpa = false;
bool jpb = false;

// switches
int const SWITCH_PINS = 0xf; // port A

// lin driver
int const LIN_RX_PIN = 0x400; // port A
int const LIN_EN_PIN = 1; // port F
int const LIN_TIMER_FREQUENCY = 1000; // 1kHz
int const LIN_SLEEP_TIMEOUT = 1; // 1ms
int const LIN_IDLE_TIMEOUT = 4000; // 4s

// relay driver
int const RD_EN_PIN = 2; // port F
int const RD_TIMER_FREQUENCY = 1000; // 1kHz
int const RD_TIMEOUT = 10; // 10ms

// switch polling interval for stand-alone mode
int const SW_TIMER_FREQUENCY = 1000; // 1kHz
int const SW_TIMEOUT = 50; // 50ms
int const SW_HOLD_COUNT = 20; // 1s

// GPIO mode
#define IN(pin) 0
#define OUT(pin) (1 << pin * 2)
#define ALT(pin) (2 << pin * 2) // alternate function
#define AN(pin) (3 << pin * 2) // analog

// GPIO speed
#define LO(pin) 0
#define HI(pin) (3 << pin * 2)

// GPIO pull up/down
#define OFF(pin) 0
#define UP(pin) (1 << pin * 2)
#define DOWN(pin) (2 << pin * 2)

// GPIO alternate function
#define AF(pin, function) (function << (pin & 7) * 4)


// USART mode
#define USART_CR1_FULL_DUPLEX (USART_CR1_RE | USART_CR1_TE)

// USART stop bits
#define USART_CR2_STOPBITS_1 0
#define USART_CR2_STOPBITS_2 USART_CR2_STOP_1


// SPI mode
#define SPI_CR1_OFF                0
#define SPI_CR1_FULL_DUPLEX_MASTER (SPI_CR1_MSTR)

// SPI clock phase and polarity
#define SPI_CR1_CPHA_0 0
#define SPI_CR1_CPHA_1 SPI_CR1_CPHA
#define SPI_CR1_CPOL_0 0
#define SPI_CR1_CPOL_1 SPI_CR1_CPOL

// SPI bit order
#define SPI_CR1_LSB_FIRST (SPI_CR1_LSBFIRST)
#define SPI_CR1_MSB_FIRST 0

// SPI prescaler
#define SPI_CR1_DIV2   0
#define SPI_CR1_DIV4   (SPI_CR1_BR_0)
#define SPI_CR1_DIV8   (SPI_CR1_BR_1)
#define SPI_CR1_DIV16  (SPI_CR1_BR_1 | SPI_CR1_BR_0)
#define SPI_CR1_DIV32  (SPI_CR1_BR_2)
#define SPI_CR1_DIV64  (SPI_CR1_BR_2 | SPI_CR1_BR_0)
#define SPI_CR1_DIV128 (SPI_CR1_BR_2 | SPI_CR1_BR_1)
#define SPI_CR1_DIV256 (SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0)

// SPI data width
#define SPI_CR2_8BIT  (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0)
#define SPI_CR2_16BIT (SPI_CR2_DS_3 | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0)


enum LinState {
	SLEEP,
	AWAKE,
};

enum FrameState {
	// wait for break
	IDLE,

	// wait for sync
	SYNC,	
	
	// wait for id
	ID,
	
	RECEIVE,
	SEND,
};


void waitForEvent() {
	// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
	
	// data synchronization barrier
	__DSB();
	
	// wait for event (interrupts trigger an event due to SEVONPEND)
	__WFE();
}

void clearPinInterrupts() {
	EXTI->PR = LIN_RX_PIN | SWITCH_PINS;
	NVIC->ICPR[0] = (1 << EXTI0_1_IRQn) | (1 << EXTI2_3_IRQn) | (1 << EXTI4_15_IRQn);
}

void clearSwitchInterrupts() {
	EXTI->PR = SWITCH_PINS;
	NVIC->ICPR[0] = (1 << EXTI0_1_IRQn) | (1 << EXTI2_3_IRQn);
}

void resetLinTimer(int time) {
	// reset timer
	TIM1->ARR = time;
	TIM1->EGR = TIM_EGR_UG;
	
	// clear pending interrupt flags
	TIM1->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM1_BRK_UP_TRG_COM_IRQn;					
}

void enableLin() {
	// set driver pin high
	GPIOF->BSRR = LIN_EN_PIN;

	// enable peripheral clock
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM1EN;

	// enable usart
	USART1->CR1 |= USART_CR1_UE;
	
	// set and start timer
	resetLinTimer(LIN_IDLE_TIMEOUT);
	TIM1->CR1 |= TIM_CR1_CEN;	
}

void disableLin() {
	// set driver pin low
	GPIOF->BRR = LIN_EN_PIN;

	// usart
	USART1->CR1 &= ~USART_CR1_UE;

	// stop timer
	TIM1->CR1 &= ~TIM_CR1_CEN;	
	
	// clear pending interrupt flags	
	TIM1->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM1_BRK_UP_TRG_COM_IRQn;					

	// peripheral clock
	RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM1EN);
}

void clearLinBreakInterrupt() {
	USART1->ICR = USART_ICR_LBDCF;
	NVIC->ICPR[0] = 1 << USART1_IRQn;
}

uint8_t readLin() {
	uint8_t data = USART1->RDR;
	NVIC->ICPR[0] = 1 << USART1_IRQn;
	return data;
}

uint8_t calcProtectedId(uint8_t id) {
	uint8_t p0 = (id << 6 | id << 5 | id << 4 | id << 2) & 0x40;
	uint8_t p1 = (id << 6 | id << 4 | id << 3 | id << 2) & 0x80;
	return  id | p0 | p1;
}

uint8_t calcChecksum(uint8_t init, uint8_t *data, uint8_t len) {
	uint16_t tmp = init;
	uint8_t i;
	for (i = 0; i < len; ++i) {
		tmp += *data++;
		if (tmp >= 256)
			tmp -= 255;
	}
	return ~tmp;
}

void startRelayTimer() {
	// reset timer
	TIM2->ARR = RD_TIMEOUT;
	TIM2->EGR = TIM_EGR_UG;
	
	// clear pending interrupt flags
	TIM2->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM2_IRQn;					

	// start timer
	TIM2->CR1 |= TIM_CR1_CEN;	
}

bool isRelayTimerRunning() {
	return (TIM2->CR1 & TIM_CR1_CEN) != 0;
}

void clearRelayTimerInterrupts() {
	TIM2->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM2_IRQn;
}

void enableRelays() {
	// set driver pin high
	GPIOF->BSRR = RD_EN_PIN;

	// enable peripheral clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	
	// enable spi
	SPI1->CR1 |= SPI_CR1_SPE;	
}

void disableRelays() {
	// set driver pin low
	GPIOF->BRR = RD_EN_PIN;

	// disable spi
	SPI1->CR1 &= ~SPI_CR1_SPE;
	
	// stop timer
	TIM2->CR1 &= ~TIM_CR1_CEN;	

	// disable peripheral clock
	RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
}

uint8_t setRelays(uint8_t relayState, uint8_t relays) {
	uint8_t change = relayState ^ relays;

	// check if A0 or A1 need to be changed
	uint8_t ax = 0;
	uint8_t ay = 0;
	uint8_t az = 0;
	if (change & 0x33) {
		uint8_t a0 = relays & 1;
		uint8_t a1 = (relays >> 1) & 1;
		
		if (a0 == a1) {
			if (a0 == 0 || !jpa) {
				ax = 0x40 | a0;
				ay = 0x40 | (a0 ^ 1);
				az = 0x40 | a1;
			}
			relayState = (relayState & ~0x33) | (relays & 0x03);
		} else if ((change & 0x11) != 0) {
			ax = 0x40 | a0;
			ay = 0x40 | (a0 ^ 1);
			relayState = (relayState & ~0x11) | (relays & 0x01);
		} else {
			ay = 0x40 | (a1 ^ 1);
			az = 0x40 | a1;		
			relayState = (relayState & ~0x22) | (relays & 0x02);
		}
	}

	// check if B0 or B1 need to be changed
	uint8_t bx = 0;
	uint8_t by = 0;
	uint8_t bz = 0;
	if (change & 0xcc) {
		uint8_t b0 = (relays >> 2) & 1;
		uint8_t b1 = (relays >> 3) & 1;
		
		if (b0 == b1) {
			if (b0 == 0 || !jpb) {
				bx = 0x40 | b0;
				by = 0x40 | (b0 ^ 1);
				bz = 0x40 | b1;
			}
			relayState = (relayState & ~0xcc) | (relays & 0x0c);
		} else if ((change & 0x44) != 0) {
			bx = 0x40 | b0;
			by = 0x40 | (b0 ^ 1);
			relayState = (relayState & ~0x44) | (relays & 0x04);
		} else {
			by = 0x40 | (b1 ^ 1);
			bz = 0x40 | b1;		
			relayState = (relayState & ~0x88) | (relays & 0x08);
		}
	}
	
	// build 16 bit word for relay driver
	uint16_t word = ax << 6 | ay << 4 | az << 3
		| bx << 5 | by << 2 | bz << 1;
	
	// send to relay driver
	SPI1->DR = word;
		
	return relayState;
}

uint8_t trySetRelays(uint8_t relayState, uint8_t relays) {
	// check if relay state change is needed and relay timer is not running yet
	if (relays != relayState && !isRelayTimerRunning())
		return setRelays(relayState, relays);
		startRelayTimer();
	}	
}

void startSwitchTimer() {
	// peripheral clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	// reset
	TIM3->EGR = TIM_EGR_UG;
	
	// clear pending interrupt flags
	TIM3->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM3_IRQn;					

	// start
	TIM3->CR1 |= TIM_CR1_CEN;	
}

void stopSwitchTimer() {
	// stop
	TIM3->CR1 &= ~TIM_CR1_CEN;	

	// peripheral clock
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
}

void clearSwitchTimerInterrupts() {
	TIM3->SR &= ~TIM_SR_UIF;
	NVIC->ICPR[0] = 1 << TIM3_IRQn;
}



// called from system/startup_stm32f042x6.s to setup clock and flash before static constructors
void SystemInit(void) {
	// leave clock in default configuration (8MHz)
}

uint8_t linBuffer[8];

int main(void) {	
	// disabled interrupts trigger an event and wake up the processor from WFE
	// see chapter 5.3.3 in reference manual
	SCB->SCR |= SCB_SCR_SEVONPEND_Msk;
	
	// peripheral clock enable
	RCC->AHBENR = RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOFEN;
	
	// configure GPIOs
	// see chapter 8.4 in reference manual
	
	// preserve SW-DP on PA13 and PA14
	// GPIOA          PA0 SWA0 | PA1 SWA1 | PA2 SWB0 | PA3 SWB1 | PA4 NSS  | PA5 SCK  | PA6 MISO | PA7 MOSI | PA9 TX   | PA10 RX
	GPIOA->MODER   |= IN(0)    | IN(1)    | IN(2)    | IN(3)    | ALT(4)   | ALT(5)   | ALT(6)   | ALT(7)   | ALT(9)   | ALT(10);
	GPIOA->OSPEEDR |=                                             HI(4)    | HI(5)    | HI(6)    | HI(7)    | HI(9)    | HI(10);
	GPIOA->PUPDR   |= UP(0)    | UP(1)    | UP(2)    | UP(3)    | OFF(4)   | OFF(5)   | UP(6)    | OFF(7)   | OFF(9)   | UP(10);
	GPIOA->AFR[0]  |=                                             AF(4, 0) | AF(5, 0) | AF(6, 0) | AF(7, 0);
	GPIOA->AFR[1]  |=                                                                                         AF(9, 1) | AF(10, 1);

	// GPIOB       PB1 JPA | PB8 JPB
	GPIOB->MODER = IN(1)   | IN(8);
	GPIOB->PUPDR = UP(1)   | UP(8);

	// GPIOF         PF0 LIN_EN | PF1 RD_EN
	GPIOF->MODER   = OUT(0)     | OUT(1);
	GPIOF->OSPEEDR = HI(0)      | HI(1);

	// get jumper state
	bool jpa = (GPIOB->IDR & 1) == 0;
	bool jpb = (GPIOB->IDR & 8) == 0;
	
	// switch off port B again
	// GPIOB       PB1 JPA | PB8 JPB
	GPIOB->MODER = AN(1)   | AN(8);
	GPIOB->PUPDR = OFF(1)  | OFF(8);
	RCC->AHBENR = RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOFEN;
	

	// enable interrupts for switches on PA0 - PA3 and LIN_RX on PA10
	// see chapter 11.2 in reference manual and A.6.2 for code example
	SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI1_PA | SYSCFG_EXTICR1_EXTI2_PA | SYSCFG_EXTICR1_EXTI3_PA;
	EXTI->FTSR = LIN_RX_PIN | SWITCH_PINS; // detect falling edge for lin and switches
	EXTI->RTSR = SWITCH_PINS; // detect rising edge for switches
	
	// USART for LIN
	// see chapter 27 in reference manual 
	USART1->CR1 = USART_CR1_FULL_DUPLEX | USART_CR1_RXNEIE; // full duplex, rx interrupt enable
	USART1->CR2 = USART_CR2_STOPBITS_1 // 1 stop bit
		| USART_CR2_LINEN | USART_CR2_LBDL | USART_CR2_LBDIE; // LIN mode, 11 bit break, LIN interrupt enable
	USART1->BRR = (USART1_CLOCK + USART1_BAUDRATE / 2) / USART1_BAUDRATE;

	// TIM1 for LIN timeout
	// see chapter 17 in reference manual
	TIM1->CR1 = TIM_CR1_DIR; // down
	TIM1->PSC = (TIM1_CLOCK + LIN_TIMER_FREQUENCY / 2) / LIN_TIMER_FREQUENCY - 1;
	TIM1->DIER = TIM_DIER_UIE; // interrupt enable

	// SPI for relay driver
	// see chapter 28 in reference manual and A.17 for code examples
	SPI1->CR1 = SPI_CR1_FULL_DUPLEX_MASTER
		| SPI_CR1_CPHA_1 | SPI_CR1_CPOL_0 // shift on rising edge, sample on falling edge
		| SPI_CR1_MSB_FIRST | SPI_CR2_16BIT
		| SPI_CR1_DIV8;
	SPI1->CR2 = SPI_CR2_SSOE; // hardware NSS output
	
	// TIM2 for relay driver timeout
	// see chapter 18 in reference manual
	TIM2->CR1 = TIM_CR1_DIR; // down
	TIM2->PSC = (TIM2_CLOCK + RD_TIMER_FREQUENCY / 2) / RD_TIMER_FREQUENCY - 1;
	TIM2->ARR = RD_TIMEOUT;
	TIM2->DIER = TIM_DIER_UIE; // interrupt enable
	
	// TIM3 for switch polling timer in stand-alone mode
	TIM3->CR1 = TIM_CR1_DIR; // down
	TIM3->PSC = (TIM3_CLOCK + SW_TIMER_FREQUENCY / 2) / SW_TIMER_FREQUENCY - 1;
	TIM3->ARR = SW_TIMEOUT;
	TIM3->DIER = TIM_DIER_UIE; // interrupt enable
	
	
	uint8_t commissionId = calcProtectedId(1);
	uint8_t allSwitchesId = calcProtectedId(2);
	uint8_t switchesId = calcProtectedId(3);
	uint8_t relaysId = calcProtectedId(4);
	uint8_t diagnosticMasterId = calcProtectedId(0x3c);
	enum LinState linState = SLEEP;
	
	enum FrameState frameState = IDLE;
	uint8_t id;
	uint8_t bufferIndex;
	uint8_t bufferLength;

	uint8_t switches = 0;
	
	uint8_t relays = 0;
	uint8_t relayState = 0xf0; // force initial set of relays

	// enable pin interrupts to detect LIN activity and switch state changes
	EXTI->IMR = LIN_RX_PIN | SWITCH_PINS;
	
	int countA;
	int countB
	
	// loop
	while (1) {
		waitForEvent();

		// check state change of switches
		if ((EXTI->PR & SWITCH_PINS) != 0) {
			// check if we are commissioned
			if (switchesId == 0xff) {
				// no: start switch timer
				startSwitchTimer();
				
				// only disable switch pin interrupts
				EXTI->IMR &= ~SWITCH_PINS;
			} else {
				// yes: wake up LIN by sending low pulse
				enableLin();
				USART1->TDR = 0;
				linState = AWAKE;

				// disable all pin interrupts
				EXTI->IMR = 0;
			}
			clearSwitchInterrupts();
		}

		switch (linState) {
		case SLEEP:
			{
				// check for LIN wakeup
				if ((EXTI->PR & LIN_RX_PIN) != 0) {
					// enable up LIN
					enableLin();
					linState = AWAKE;

					// disable LIN interrupt and switch interrupts if we are commissioned
					if (switchesId == 0xff)
						EXTI->IMR = SWITCH_PINS;
					else
						EXTI->IMR = 0;
					
					clearPinInterrupts();
				}
			}
			break;
			
		case AWAKE:
			{
				int isr = USART1->ISR;
	
				// check if LIN break was detected
				if ((isr & USART_ISR_LBDF) != 0) {
					clearLinBreakInterrupt();
					
					// reset LIN idle timeout
					resetLinTimer(LIN_IDLE_TIMEOUT);
					
					// now wait for sync byte
					linState = SYNC;
				}
				
				// check if data has arrived
				if ((isr & USART_ISR_RXNE) != 0) {
					uint8_t data = readLin();
					
					switch (frameState) {
					case IDLE:
						// discard until a break is detected
						break;
					case SYNC:
						// check for sync byte
						if (data == 0x55) {
							// now wait for id byte
							frameState = ID;
						} else {
							// return to idle state
							frameState = IDLE;
						}
						break;
					case ID:
						{
							// get protected id
							id = data;
							
							// decide what to do
							if (id == commissionId) {
								// expect 7 data bytes (4 static node id, 1 switchesId, 1 relaysId, 1 relay state)
								bufferLength = 7;

								// enable relays here because driver needs 100us setup time
								enableRelays();

								// now receive
								frameState = RECEIVE;
							} else if ((id == allSwitchesId || id == switchesId) && switchesId != 0xff) {								
								// read switch state and set to buffer
								switches = ~GPIOA->IDR & SWITCH_PINS;
								linBuffer[0] = switchesId;
								linBuffer[1] = switches;
								bufferLength = 2;
								
								// now send
								frameState = SEND;
							} else if ((id == relaysId && relaysId != 0xff) || id == diagnosticMasterId) {
								// expect 1 data byte
								bufferLength = 1;
								
								// enable relays here because driver needs 100us setup time
								enableRelays();
								
								// now receive
								frameState = RECEIVE;
							} else {
								// ignore unknown id and return to idle state
								frameState = IDLE;
							}
								
							// first byte when in send mode
							if (frameState == SEND)
								USART1->TDR = linBuffer[0];
							bufferIndex = 0;
						} 
						break;
					case RECEIVE:
						if (bufferIndex < bufferLength) {
							linBuffer[++bufferIndex] = data;
						} else if (data == calcChecksum(id, linBuffer, bufferLength)) {
							// frame complete and checksum ok
							
							// decide what to do
							if (id == diagnosticMasterId) {
								// return to idle state
								frameState = IDLE;
								if (linBuffer[0] == 0) {
									// the master requests us to to to sleep
									
									// set short timeout before we go to sleep or wake up again
									resetLinTimer(LIN_SLEEP_TIMEOUT);
								}
							} else if (id == commissionId) {
								// check if static node id matches our id
								if (*(uint32_t*)linBuffer == *(uint32_t const*)staticId) {
									// stop switch timer of stand-alone mode
									stopSwitchTimer();
									EXTI->IMR = 0;
	
									// set LIN id for switches and relays
									if (linBuffer[4] >= 4 && linBuffer[4] < 0x3c)
										switchesId = calcProtectedId(linBuffer[4]);
									if (linBuffer[5] >= 4 && linBuffer[5] < 0x3c)
										relaysId = calcProtectedId(linBuffer[5]);
									
									// set relays
									relays = linBuffer[6] & 0x0f;
									relayState = trySetRelays(relayState, relays);
								} 								
							} else if (id == relaysId) {
								// set state of relays
								relays = linBuffer[0] & 0x0f;
								relayState = trySetRelays(relayState, relays);
								
								// return to idle state
								frameState = IDLE;
							} 
						} else {
							// checksum error: return to idle state
							frameState = IDLE;
						}
						break;					
					case SEND:
						// check if we received what we sent
						if (data == linBuffer[bufferIndex]) {
							++bufferIndex;
						
							// send next byte or checksum
							if (bufferIndex < bufferLength) {
								USART1->TDR = linBuffer[++bufferIndex];
							} else {
								USART1->TDR = calcChecksum(id, linBuffer, bufferLength);
								
								// return to idle state
								frameState = IDLE;
							}
						} else {
							// send error: maybe some other node also sends
							frameState = IDLE;
						}
						break;
					}
				}
			}
			break;
		}
				
		// check if LIN idle timeout elapsed
		if ((TIM1->SR & TIM_SR_UIF) != 0) {
			frameState = IDLE;

			// enable pin interrupts to detect lin activity switch state changes
			EXTI->IMR = LIN_RX_PIN | SWITCH_PINS;

			// go to sleep mode if switches haven't changed since last read
			if (switches == (~GPIOA->IDR & SWITCH_PINS) || switchesId == -1) {
				disableLin();
				linState = SLEEP;	

				// also disalbe relays if they are idle
				if (!isRelayTimerRunning())
					disableRelays();
			} else {
				// switches have changed: disable pin interrupts again
				EXTI->IMR = 0;

				// wake-up LIN network because we have pending switch state changes
				USART1->TDR = 0;				

				resetLinTimer(LIN_IDLE_TIMEOUT);
			}
		}
		
		// check if relay timeout elapsed
		if ((TIM2->SR & TIM_SR_UIF) != 0) {
			clearRelayTimerInterrupts();

			if (relayState == relays) {
				disableRelays();
			} else {
				relayState = setRelays(relayState, relays);
			}
		}
		
		// check if switch timeout elapsed
		if ((TIM3->SR & TIM_SR_UIF) != 0) {
		
			// enable switch interrupts
			EXTI->IMR = SWITCH_PINS;

			uint8_t lastSwitches = switches;
			switches = ~GPIOA->IDR & SWITCH_PINS;
			if (!jpa) {
				// on/off switch
				if ((switches & 0x01) != 0)
					relays |= 0x01;
				if ((switches & 0x02) != 0)
					relays &= ~0x03;
			} else {
				// roller shutter switch
				
				// check of A0 or A1 goes on or off
				if ((~lastSwitches & switches & 0x03) != 0) {
					// A0 or A1 goes on
					// check if a relay is on
					if ((relays & 0x03) != 0) {
						// switch off A0 and A1
						relays &= ~0x03;
					} else {
						// switch on A0 or A1
						relays |= switches & 0x03;
						countA = 0;
					}								
				} else if ((lastSwitches & ~switches & 0x03) != 0 && countA > SW_HOLD_COUNT) {
					// A0 or A1 goes off after minimum hold time
					// switch off A0 and A1
					relays &= ~0x03;
				}
				++countA;
			}
			if (!jpb) {
				// on/off switch
				if ((switches & 0x04) != 0)
					relays |= 0x04;
				if ((switches & 0x08) != 0)
					relays &= ~0x0c;
			} else {
				// roller shutter switch
			
				// check of B0 or B1 goes on or off
				if ((~lastSwitches & switches & 0x0c) != 0) {
					// B0 or B1 goes on
					// check if a relay is on
					if ((relays & 0x0c) != 0) {
						// switch off B0 and B1
						relays &= ~0x0c;									
					} else {
						// switch on B0 or B1
						relays |= switches & 0x0c;
						countB = 0;
					}								
				} else if ((lastSwitches & ~switches & 0x0c) != 0 && countB > SW_HOLD_COUNT) {
					// B0 or B1 goes off after minimum hold time
					// switch off B0 and B1
					relays &= ~0x0c;
				}
				++countB;
			}
			relayState = trySetRelays(relayState, relays);
			
			// stop timer if no switch is pressed
			if (switches == 0)
				stopSwitchTimer();
			clearSwitchTimerInterrupts();
		}
	}
}

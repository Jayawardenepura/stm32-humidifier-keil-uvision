#include <stdint.h>
#include <stdbool.h>
#include "stm32f411xe.h"
#include "hum_sensor.h"
#include "mcu_init.h"
#include "systick.h"

static bool h_sensor_gpio_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // SDA

	/* Alternate function mode */
	GPIOB->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1;
	
	/* Output open-drain */
	GPIOB->OTYPER |= GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9;
	
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8_0 | GPIO_OSPEEDER_OSPEEDR8_1 | 
										GPIO_OSPEEDER_OSPEEDR9_0 | GPIO_OSPEEDER_OSPEEDR9_1;

	/* Pull-UP */
	//GPIOB->PUPDR |= GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR9_0;

	/* 0100: AF4 - I2C1-3 */
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL9_2 | GPIO_AFRH_AFSEL8_2;
	
	return true;
}

static bool h_sensor_i2c_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	I2C1->CR1 |= I2C_CR1_SWRST;
	I2C1->CR1 &= ~I2C_CR1_SWRST;
	/*  Peripheral disable */
	I2C1->CR2 &= ~I2C_CR1_PE;
	
	/*  I2C mode */
	I2C1->CR2 &= ~I2C_CR1_SMBUS;
	
	/* duty cycle 50% */
	I2C1->CCR &= ~I2C_CCR_DUTY;
	
	/*  Peripheral clock frequency - 48 MHz*/
	I2C1->CR2 &= ~I2C_CR2_FREQ;
	I2C1->CR2 |= (uint32_t)MCU_CLOCK_MHZ;
	
  I2C1->CCR &= ~I2C_CCR_CCR;

	/* SM mode */
	I2C1->CCR &= ~I2C_CCR_FS;
	

	/* Tscl/2Tpclk = 10us(SM)/(2*(1/48Mhz)) - 100kHz SM mode*/
	I2C1->CCR |= 240; 
	
	/* (Trmax / Tpclk) + 1 = (1000nS (SM) / 1/48Mhz) + 1 = 48 */
	I2C1->TRISE |= 49;
	
	/*  Peripheral enable */
	I2C1->CR1 |= (I2C_CR1_PE | I2C_CR1_ACK);
		
	I2C1->CR2 |= (I2C_CR2_ITEVTEN);
	return true;
}

bool hs_init(void)
{
	h_sensor_gpio_init();
	h_sensor_i2c_init();

	return true;
}

static inline void i2c_busy_wait(void)
{
		while (I2C1->SR2 & I2C_SR2_BUSY);
}

static inline void i2c_start(void)
{
	//I2C1->SR1 = (uint32_t)0;
	i2c_busy_wait();

	I2C1->CR1 |= I2C_CR1_START;

	/* Wait until Start bit completed */
	while (!(I2C1->SR1 & I2C_SR1_SB));
}

static inline void i2c_stop(void)
{
	I2C1->CR1 |= I2C_CR1_STOP;
}

static inline void i2c_send_data(uint8_t word)
{
	I2C1->DR = word;
//	while (!(I2C1->SR1 & I2C_SR1_BTF));
	udelay(8);

	(void) I2C1->SR1;
}

static inline void i2c_set_read(void)
{
	I2C1->DR = HUM_SENSOR_I2C_ADDR+R;
//	while (!(I2C1->SR1 & I2C_SR1_BTF));
	udelay(6);

}

void hs_wakeup(void)
{
	i2c_start();
	
	i2c_send_data(HUM_SENSOR_I2C_ADDR);
	
	udelay(100);
	
	i2c_stop();
}

static inline void i2c_set_addr(void)
{
		I2C1->DR = HUM_SENSOR_I2C_ADDR;
		while(!(I2C1->SR1 & I2C_SR1_ADDR));
		(void) I2C1->SR2;
}

bool hs_write(void)
{
		i2c_start();
		//(void) I2C1->SR1;

		i2c_set_addr();

		i2c_send_data(0x03);

		i2c_send_data(0x00);

		i2c_send_data(0x04);

		i2c_stop();
}

void hs_read(void)
{
		uint8_t byte;
		i2c_start();
	
		i2c_set_read();
	

		i2c_stop();
}
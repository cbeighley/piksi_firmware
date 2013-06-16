/*
 * Copyright (C) 2011 Fergus Noble <fergusnoble@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/rcc.h>

#include "spi.h"

/** \addtogroup peripherals
 * \{ */

/** \defgroup spi SPI
 * Functions to setup and use STM32F4 SPI peripherals.
 * \{ */

/** Set up SPI buses.
 * Set up SPI peripheral, SPI clock, SPI pins, and SPI pins' clock.
 */
void spi_setup(void)
{
  /* Enable SPI1 periperal clock */
  RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
  /* Enable SPI2 periperal clock */
  RCC_APB1ENR |= RCC_APB1ENR_SPI2EN;
  /* Enable GPIO clocks for CS lines */
	RCC_AHB1ENR |= RCC_AHB1ENR_IOPAEN | RCC_AHB1ENR_IOPBEN;

  /* Setup CS line GPIOs */
  spi_slave_deselect();
  /* FPGA CS */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO4);
  /* Configuration flash CS */
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO12);
  /* Front-end CS */
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO11);

  /* Setup SPI alternate function */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO6 | GPIO7);
	gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
	gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
  /*gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO13 | GPIO15);*/

	/* Setup SPI parameters. */
  spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_2, 0, 0,
                  SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
  spi_enable_ss_output(SPI1); /* Required, see 25.3.1 section about NSS */
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2, 0, 0,
                  SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
  spi_enable_ss_output(SPI2); /* Required, see 25.3.1 section about NSS */

	/* Finally enable the SPI. */
	spi_enable(SPI1);
	spi_enable(SPI2);
}

/** Deactivate SPI buses.
 * Disable SPI peripheral, SPI clock, High-Z SPI pins, and disable SPI pins'
 * clock.
 */
void spi_deactivate(void){
  /* Wait until transfers are done per RM0090 page 811 */
  //while (!(SPI1_SR & SPI_SR_RXNE)); /* RM0090 says to do this but it hangs */
  //while (!(SPI1_SR & SPI_SR_TXE));
  while (SPI1_SR & SPI_SR_BSY);
  spi_disable(SPI1);
  /* Disable SPI1 periperal clock */
  RCC_APB2ENR &= ~RCC_APB2ENR_SPI1EN;
  //while (!(SPI2_SR & SPI_SR_RXNE));
  //while (!(SPI2_SR & SPI_SR_TXE));
  while (SPI2_SR & SPI_SR_BSY);
  spi_disable(SPI2);
  /* Disble SPI2 periperal clock */
  RCC_APB1ENR &= ~RCC_APB1ENR_SPI2EN;
  /* Set all SPI GPIOs to inputs with no pull up/down resistors */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO4);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO11);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5 | GPIO6 | GPIO7);
  gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
  /* Disble GPIO clocks for CS lines */
	RCC_AHB1ENR &= ~(RCC_AHB1ENR_IOPAEN | RCC_AHB1ENR_IOPBEN);
}

/** Drive SPI nCS line low for selected peripheral.
 * \param slave Peripheral to drive chip select for.
 */
void spi_slave_select(u8 slave)
{

  spi_slave_deselect();
  __asm__("CPSID i;"); /* Disable interrupts */

  switch (slave) {
    case SPI_SLAVE_FPGA:
      gpio_clear(GPIOA, GPIO4);
      break;
    case SPI_SLAVE_FLASH:
      gpio_clear(GPIOB, GPIO12);
      break;
    case SPI_SLAVE_FRONTEND:
      gpio_clear(GPIOB, GPIO11);
      break;
  }
}

/** Drive all SPI nCS lines high.
 * Should be called after an SPI transfer is finished.
 */
void spi_slave_deselect(void)
{
  /* Deselect FPGA CS */
  gpio_set(GPIOA, GPIO4);
  /* Deselect Configuration flash and Front-end CS */
  gpio_set(GPIOB, GPIO11 | GPIO12);
  __asm__("CPSIE i;"); /* Re-enable interrupts */
}

/** \} */

/** \} */
/*-----------------------------------------------------------------------------------------------*/
/*                                                                                               */
/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                               */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/
#ifndef __MACRO_SPI_H__
#define __MACRO_SPI_H__

#define ENABLE_SPI0_CLK         APBCLK |= SPI0_CLKEN			// Enable SPI0 clock
#define ENABLE_SPI1_CLK         APBCLK |= SPI1_CLKEN			// Enable SPI1 clock

#define SET_SPI0_MASTER_MODE    SPI0_CNTRL &= ~SPI_MODE_SLAVE	// Set SPI0 master mode
#define SET_SPI1_MASTER_MODE    SPI1_CNTRL &= ~SPI_MODE_SLAVE	// Set SPI1 master mode
#define SET_SPI0_SLAVE_MODE     SPI0_CNTRL |= SPI_MODE_SLAVE	// Set SPI0 slave mode
#define SET_SPI1_SLAVE_MODE     SPI1_CNTRL |= SPI_MODE_SLAVE	// Set SPI1 slave mode

#define ENABLE_SPI0_DUAL_BUFFER     SPI0_CNTRL |= DUAL_FIFO		// Set SPI0 FIFO dual buffer
#define ENABLE_SPI1_DUAL_BUFFER     SPI1_CNTRL |= DUAL_FIFO		// Set SPI1 FIFO dual buffer
#define DISABLE_SPI0_DUAL_BUFFER    SPI0_CNTRL &= ~DUAL_FIFO	// Set SPI0 FIFO single buffer
#define DISABLE_SPI1_DUAL_BUFFER    SPI1_CNTRL &= ~DUAL_FIFO	// Set SPI1 FIFO single buffer

#define ENABLE_SPI0_AUTO_SLAVE_SLECT    SPI0_SSR |= ASS_AUTO	// Enable SPI0 auto slave select
#define ENABLE_SPI1_AUTO_SLAVE_SLECT    SPI1_SSR |= ASS_AUTO	// Enable SPI1 auto slave select
#define DISABLE_SPI0_AUTO_SLAVE_SLECT   SPI0_SSR &= ~ASS_AUTO	// Disable SPI0 auto slave select
#define DISABLE_SPI1_AUTO_SLAVE_SLECT   SPI1_SSR &= ~ASS_AUTO	// Disable SPI1 auto slave select

#define ENABLE_SPI0_INTERRUPT       SPI0_CNTRL |= SPI_IE		// Enable SPI0 interrupt
#define ENABLE_SPI1_INTERRUPT       SPI1_CNTRL |= SPI_IE		// Enable SPI1 interrupt
#define DISABLE_SPI0_INTERRUPT      SPI0_CNTRL &= ~SPI_IE		// Disable SPI0 interrupt
#define DISABLE_SPI1_INTERRUPT      SPI1_CNTRL &= ~SPI_IE		// Disable SPI1 interrupt

#define SPI0_GO_BUSY    SPI0_CNTRL |= GO_BUSY					// Start SPI0
#define SPI1_GO_BUSY    SPI1_CNTRL |= GO_BUSY					// Start SPI1

#endif


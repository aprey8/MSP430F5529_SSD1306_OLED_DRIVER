/*
 * ssd1306.c
 *
 *  Created on: Mar 19, 2018
 *      Author: Adam Prey
 */
#include "ssd1306.h"

void ssd1306_display_init();

static uint8_t ssd1306_command(uint8_t command){

	size = 2;
	i2c_tx_buffer[0] = 0x00;
	i2c_tx_buffer[1] = command;
	Tx_data_send(i2c_tx_buffer, size);
	return 1;
}
void Tx_data_send(unsigned char *data_send, uint8_t size){
	for(i=0;i<10;i++);
	TXByteCtr = size;

		PTxData = data_send;
		UCB0CTL1 |= UCTR + UCTXSTT;                 // I2C TX, start condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
		__no_operation();
		while (UCB0STAT & UCBBUSY);
}

void Tx_data_send_bytes_no_stop(unsigned char *data_send){
	for(i=0;i<10;i++);
	TXByteCtr = 1;

		PTxData = data_send;
		UCB0CTL1 |= UCTR + UCTXSTT;                 // I2C TX, start condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
		__no_operation();
		while (UCB0STAT & UCBBUSY);
}


void ssd1306_display_init(){
	P3SEL |= 0x03;                            // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x3C;                         // Slave Address for OLED is 0x3C
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	UCB0IE |= UCTXIE;                         // Enable TX interrupt


	ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
	ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
	ssd1306_command(0x80);                                  // the suggested ratio 0x80
	ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
	ssd1306_command(SSD1306_HEIGHT - 1);
	ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
	ssd1306_command(0x0);                                   // no offset
	ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0 0x40

	ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
	ssd1306_command(0x14);

	ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
	ssd1306_command(0x00);                                  // 0x0 act like ks0108
	ssd1306_command(SSD1306_SEGREMAP | 0x1);				// 0x A1

	// Set scan direction
	ssd1306_command(SSD1306_COMSCANDEC);					// 0xC8

	ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
	ssd1306_command(0x02);

	ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
	ssd1306_command(0x8F);

	ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
	ssd1306_command(0xF1);

	ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
	ssd1306_command(0x40);

	ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4

	ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6

	ssd1306_command(SSD1306_DISPLAYON);						// 0xAF

}

static void ssd1306_clear_page(uint8_t page_number){
	// Set column start and end address
	ssd1306_command(SSD1306_COLUMNADDR);
	ssd1306_command(0);
	ssd1306_command(SSD1306_WIDTH - 1);

	// Set page start and end address
	ssd1306_command(SSD1306_PAGEADDR);
	ssd1306_command(page_number);
	ssd1306_command(page_number);


	//size = 1;

	i2c_tx_packet[0] = SSD1306_TRANSMISSION_DATA;
	//Tx_data_send(i2c_tx_buffer, size);

	uint16_t j = 0;
//	i2c_master_write_packet_wait_no_stop(i2c_master_instance, &i2c_tx_packet);
	//i2c_tx_buffer[0] = 0x00;
	for(j = 0; j < SSD1306_WIDTH; j++){
		//i2c_master_write_byte(i2c_master_instance, 0x00);
		i2c_tx_packet[j+1] = 0x00;
		size = j;
	}
	Tx_data_send(i2c_tx_packet, j);
}

void ssd1306_clear(){
	uint8_t i;
	for(i = 0; i < NUMBER_OF_PAGES; i++){
		ssd1306_clear_page(i);
	}
}

void ssd1306_clear_line(uint8_t line_number){
	if( line_number >= NUMBER_OF_LINES ){
		return;
	}

	ssd1306_clear_page(line_number);
	ssd1306_clear_page(line_number + NUMBER_OF_LINES);
}

void ssd1306_print_line(uint8_t line_number,  char* str){
	uint8_t number_of_char = strlen(str);
	if( line_number >= NUMBER_OF_LINES ){
		return;
	}
	if( number_of_char > NUMBER_OF_CHAR_PER_LINE ){
		number_of_char = NUMBER_OF_CHAR_PER_LINE;
	}
	ssd1306_clear_line(line_number);
	uint8_t i;
	for(i = 0; i < number_of_char; i++){
		ssd1306_print_char(line_number, i, *(str + i));
	}
}

static void ssd1306_print_char(uint8_t line_number, uint8_t column, char c){
	uint8_t* font_bitmap = (uint8_t *)&font[(uint16_t)(c * (SSD1306_FONT_WIDTH))];

	if( column > NUMBER_OF_CHAR_PER_LINE ){
		return;
	}
	if( line_number > NUMBER_OF_LINES ){
		return;
	}

	// Set vertical addressing mode
	ssd1306_command(SSD1306_MEMORYMODE);
	ssd1306_command(1);

	// Set column start and end address
	ssd1306_command(SSD1306_COLUMNADDR);
	if( column == 0 ){
		ssd1306_command( 0 );
		} else {
		ssd1306_command( column * SSD1306_FONT_WIDTH + 1 );
	}
	ssd1306_command( (column + 1) * SSD1306_FONT_WIDTH );

	// Set page start and end address
	ssd1306_command(SSD1306_PAGEADDR);
	ssd1306_command(line_number);
	ssd1306_command(line_number);


	i2c_tx_packet[0] = SSD1306_TRANSMISSION_DATA;

	uint8_t i;
	for(i = 0; i < SSD1306_FONT_WIDTH; i++){

		i2c_tx_packet[i+1] = *(font_bitmap + i);
	}

	Tx_data_send(i2c_tx_packet, i);
}


//------------------------------------------------------------------------------
// The USCIAB0TX_ISR is structured such that it can be used to transmit any
// number of bytes by pre-loading TXByteCtr with the byte count. Also, TXData
// points to the next byte to transmit.
//------------------------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCB0IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6: break;                           // Vector  6: STTIFG
  case  8: break;                           // Vector  8: STPIFG
  case 10: break;                           // Vector 10: RXIFG
  case 12:                                  // Vector 12: TXIFG
    if (TXByteCtr)                          // Check TX byte counter
    {
      UCB0TXBUF = *PTxData;               // Load TX buffer
      PTxData++;
      TXByteCtr--;                          // Decrement TX byte counter
    }
    else
    {
      UCB0CTL1 |= UCTXSTP;                  // I2C stop condition
      UCB0IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
      __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
    }
  default: break;
  }
}


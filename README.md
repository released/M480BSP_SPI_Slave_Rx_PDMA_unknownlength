# M480BSP_SPI_Slave_Rx_PDMA_unknownlength
 M480BSP_SPI_Slave_Rx_PDMA_unknownlength

update @ 2022/05/31

1. Init SPI0 as SPI slave RX , SPI1 as SPI master TX

SPI slave SPI0 : PA0(MOSI)/PA1(MISO)/PA2(CLK)/PA3(SS)

SPI master SPI1 : PC0(SS)/PC1(CLK)/PC2(MOSI)/PC3(MISO)

2. user ADC channel to generate random data , for SPI master length creation

3. SPI master TX comamand : head(0x5A) + length + data[0]~data[n] + checksum + tail(0xA5)

4. press terminal digit 1 to send SPI TX data 

![image](https://github.com/released/M480BSP_SPI_Slave_Rx_PDMA_unknownlength/blob/main/log.jpg)

5. below is LA SPI , MOSI capture

![image](https://github.com/released/M480BSP_SPI_Slave_Rx_PDMA_unknownlength/blob/main/SPI_MOSI_random_length.jpg)


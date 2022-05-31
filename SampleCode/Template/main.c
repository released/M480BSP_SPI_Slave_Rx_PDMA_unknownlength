/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NuMicro.h"
#include "project_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

// SPI master
#define MasterSpiPortNum				(SPI1)
#define SPI_TARGET_FREQ					(800000ul)	//(48000000ul)
#define SPI_DATA_NUM					(64)
#define SPI_SET_CS_LOW					(PC0 = 0)
#define SPI_SET_CS_HIGH					(PC0 = 1)
uint8_t g_au8MasterToSlaveTestPattern[SPI_DATA_NUM]={0};

// SPI slave
#define BridgeSpiPortNum				(SPI0)
#define SPI_GET_SSLINE_FLAG(spi)		(((spi)->STATUS & SPI_STATUS_SSLINE_Msk) >> SPI_STATUS_SSLINE_Pos)

#define SPI_GET_SSINAIF_FLAG(spi)		(((spi)->STATUS & SPI_STATUS_SSINAIF_Msk) >> SPI_STATUS_SSINAIF_Pos)
#define SPI_SET_SSINAIF_FLAG(spi)		((spi)->STATUS |= SPI_STATUS_SSINAIF_Msk)

#define SPI_GET_SSACTIF_FLAG(spi)		(((spi)->STATUS & SPI_STATUS_SSACTIF_Msk) >> SPI_STATUS_SSACTIF_Pos)
#define SPI_SET_SSACTIF_FLAG(spi)		((spi)->STATUS |= SPI_STATUS_SSACTIF_Msk)

#define SPI_SLAVE_TX_DMA_CH  			(14)
#define SPI_SLAVE_RX_DMA_CH  			(15)
#define SPI_SLAVE_OPENED_CH   			((1 << SPI_SLAVE_TX_DMA_CH) | (1 << SPI_SLAVE_RX_DMA_CH))

uint8_t g_au8SlaveRxBuffer[SPI_DATA_NUM]={0};
uint16_t packetlen = 0;

#define PDMA_GET_TRANS_CNT(pdma,u32Ch) ((uint32_t)((pdma->DSCT[(u32Ch)].CTL&PDMA_DSCT_CTL_TXCNT_Msk) >> PDMA_DSCT_CTL_TXCNT_Pos))

// ADC , for random data generation

enum
{
	ADC0_CH0 = 0 ,
	ADC0_CH1 ,
	ADC0_CH2 , 
	ADC0_CH3 , 
	ADC0_CH4 ,
	ADC0_CH5 , 
	ADC0_CH6 , 
	ADC0_CH7 ,
	ADC0_CH8 , 
	ADC0_CH9 , 
	ADC0_CH10 , 
	ADC0_CH11 ,
	ADC0_CH12 , 
	ADC0_CH13 , 
	ADC0_CH14 ,
	ADC0_CH15 , 
	
	ADC0_CH16_BAND_GAP_VOLT , 
	ADC0_CH17_TEMP_SENSOR ,
	ADC0_CH18_VBAT , 
	
	ADC_CH_DEFAULT 	
}ADC_CH_TypeDef;

uint16_t za = 0;
uint16_t zb = 0;
uint16_t zc = 0;
uint16_t zx = 0;


const uint8_t CRC8TAB[256] = 
{ 
	//0
	0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97, 
	0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E, 
	//1
	0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
	0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
	//2
	0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
	0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
	//3
	0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52, 
	0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
	//4 
	0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
	0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
	//5
	0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
	0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
	//6
	0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
	0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
	//7
	0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
	0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
	//8
	0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
	0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
	//9
	0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
	0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
	//A
	0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
	0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
	//B
	0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
	0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
	//C
	0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
	0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
	//D
	0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
	0xBD, 0x8C, 0xDF, 0xFE, 0x79, 0x48, 0x1B, 0x2A,
	//E
	0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
	0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
	//F
	0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
	0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
}; 

/*_____ M A C R O S ________________________________________________________*/
#define ENABLE_ADC_IRQ


/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

uint8_t CRC8( uint8_t *buf, uint16_t len)     
{               
	uint8_t  crc=0;

	while ( len-- )     
	{   
		crc = CRC8TAB[crc^*buf]; 

		buf++;   
	}     

	return crc;     
}  

// use ADC for random seed 

void EADC00_IRQHandler(void)
{
    set_flag(flag_ADC_Data_Ready , ENABLE);
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);      /* Clear the A/D ADINT0 interrupt flag */
}

unsigned int ADC_ReadChannel(void)
{
	uint32_t ModuleMask = BIT0 ;		// use bit 0 as module ask
	uint32_t ModuleNum = 0 ;			// use ch 0 as module num
	uint16_t get_adc = 0;

    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);
	
    EADC_ENABLE_INT(EADC, BIT0);
    EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, (ModuleMask));

	set_flag(flag_ADC_Data_Ready , DISABLE);				
	EADC_START_CONV(EADC, (ModuleMask));						
	// while(is_flag_set(flag_ADC_Data_Ready) == DISABLE);
	while(EADC_GET_DATA_VALID_FLAG(EADC, (ModuleMask)) != (ModuleMask));
	
	get_adc = EADC_GET_CONV_DATA(EADC, ModuleNum);

	#if 0	// debug
	printf("%s : 0x%4X\r\n" , __FUNCTION__ , get_adc);
	#endif
	
	return get_adc;
}

void ADC_InitChannel(uint8_t ch)
{
	uint32_t ModuleMask = BIT0 ;		// use bit 0 as module ask
	uint32_t ModuleNum = 0 ;			// use ch 0 as module num
	uint16_t get_adc = 0;

    /* Set input mode as single-end, and Single mode*/
    EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);

    EADC_SetExtendSampleTime(EADC, ch, 0x3F);
	
	EADC_ConfigSampleModule(EADC, ModuleNum, EADC_ADINT0_TRIGGER, ch);

    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);
	
    EADC_ENABLE_INT(EADC, BIT0);
    EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, (ModuleMask));
    NVIC_EnableIRQ(EADC00_IRQn);

	set_flag(flag_ADC_Data_Ready , DISABLE);				
	EADC_START_CONV(EADC, (ModuleMask));						
	// while(is_flag_set(flag_ADC_Data_Ready) == DISABLE);
	while(EADC_GET_DATA_VALID_FLAG(EADC, (ModuleMask)) != (ModuleMask));
	
	get_adc = EADC_GET_CONV_DATA(EADC, ModuleNum);

	#if 0	// debug
	printf("%s : 0x%4X\r\n" , __FUNCTION__ , get_adc);
	#endif

	UNUSED(get_adc);
}

// Fast 0-255 random number generator from http://eternityforest.com/Projects/rng.php:
uint16_t rng(void)//void uint8_t __attribute__((always_inline)) rng(void)
{
    zx++;
    za = (za^zc^zx);
    zb = (zb+za);
    zc = (zc+(zb>>1)^za);
    return zc;
}

void prepare_seed(void)
{
    za = ADC_ReadChannel(); 
    zb = ADC_ReadChannel(); 
    zc = ADC_ReadChannel(); 
    zx = ADC_ReadChannel();     
}

uint32_t random(int min, int max)
{
    uint32_t res= 0;    
    uint32_t length_of_range = 0;
    uint16_t adc_vaule = 0;
    uint16_t seed = 0;

    length_of_range = max - min + 1;

    adc_vaule = ADC_ReadChannel(); 
    seed = rng();
    srand(seed + adc_vaule);

    res = (uint32_t)(rand() % length_of_range + min);

    #if 0   // debug
    printf("adc_vaule:0x%4X,res:%5d(min:%5d,max:%5d) [0x%4X/0x%4X/0x%4X/0x%4X/0x%4X]\r\n" ,
            adc_vaule , res , min , max , 
            za , zb ,zc , zx , seed);
    #endif

    return res;
}

void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA);
	
    if (status & PDMA_INTSTS_ABTIF_Msk)   /* abort */
    {
		#if 1
        PDMA_CLR_ABORT_FLAG(PDMA, PDMA_GET_ABORT_STS(PDMA));
		#else
        if (PDMA_GET_ABORT_STS(PDMA) & (1 << SPI_SLAVE_TX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << SPI_SLAVE_TX_DMA_CH));

        if (PDMA_GET_ABORT_STS(PDMA) & (1 << SPI_SLAVE_RX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << SPI_SLAVE_RX_DMA_CH));
		#endif
    }
    else if (status & PDMA_INTSTS_TDIF_Msk)     /* done */
    {
        if((PDMA_GET_TD_STS(PDMA) & SPI_SLAVE_OPENED_CH) == SPI_SLAVE_OPENED_CH)
        {
            /* Clear PDMA transfer done interrupt flag */
            PDMA_CLR_TD_FLAG(PDMA, SPI_SLAVE_OPENED_CH);

			//insert process
			SPI_DISABLE_RX_PDMA(BridgeSpiPortNum);			
        }        		
    }
    else if (status & (PDMA_INTSTS_REQTOF0_Msk | PDMA_INTSTS_REQTOF1_Msk))     /* Check the DMA time-out interrupt flag */
    {
        PDMA_CLR_TMOUT_FLAG(PDMA,SPI_SLAVE_TX_DMA_CH);
        PDMA_CLR_TMOUT_FLAG(PDMA,SPI_SLAVE_RX_DMA_CH);
    }
    else
    {

    }	
}


void SPI_Slave_Rx_PDMA_ClrBuffer(void)
{
	reset_buffer(g_au8SlaveRxBuffer , 0x00, SPI_DATA_NUM);
}


void SPI_Slave_Rx_PDMA_Enable(void)
{
	SPI_Slave_Rx_PDMA_ClrBuffer();

	//RX
	PDMA_SetTransferCnt(PDMA,SPI_SLAVE_RX_DMA_CH, PDMA_WIDTH_8, SPI_DATA_NUM);
	PDMA_SetTransferAddr(PDMA,SPI_SLAVE_RX_DMA_CH, (uint32_t)&SPI0->RX, PDMA_SAR_FIX, (uint32_t)g_au8SlaveRxBuffer, PDMA_DAR_INC);		
	/* Set request source; set basic mode. */
	PDMA_SetTransferMode(PDMA,SPI_SLAVE_RX_DMA_CH, PDMA_SPI0_RX, FALSE, 0);
	SPI_TRIGGER_RX_PDMA(SPI0);	

	PDMA_EnableInt(PDMA, SPI_SLAVE_RX_DMA_CH, PDMA_INT_TRANS_DONE);
}

void SPI_Slave_Rx_PDMA_Init(void)
{
	SPI_Slave_Rx_PDMA_ClrBuffer();
	
    PDMA_Open(PDMA, SPI_SLAVE_OPENED_CH);

	//RX
    PDMA_SetTransferCnt(PDMA,SPI_SLAVE_RX_DMA_CH, PDMA_WIDTH_8, SPI_DATA_NUM);
    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(PDMA,SPI_SLAVE_RX_DMA_CH, (uint32_t)&SPI0->RX, PDMA_SAR_FIX, (uint32_t)g_au8SlaveRxBuffer, PDMA_DAR_INC);
    /* Set request source; set basic mode. */
    PDMA_SetTransferMode(PDMA,SPI_SLAVE_RX_DMA_CH, PDMA_SPI0_RX, FALSE, 0);
    /* Single request type. SPI only support PDMA single request type. */
    PDMA_SetBurstType(PDMA,SPI_SLAVE_RX_DMA_CH, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    PDMA->DSCT[SPI_SLAVE_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    SPI_TRIGGER_RX_PDMA(BridgeSpiPortNum);

    PDMA_EnableInt(PDMA, SPI_SLAVE_RX_DMA_CH, PDMA_INT_TRANS_DONE);
    NVIC_EnableIRQ(PDMA_IRQn);		
}

void SPI0_IRQHandler(void)
{

	if (SPI_GET_SSINAIF_FLAG(BridgeSpiPortNum))
	{
//		SPI_ClearRxFIFO(BridgeSpiPortNum);
//		SPI_ClearTxFIFO(BridgeSpiPortNum);

		packetlen = PDMA_GET_TRANS_CNT(PDMA, SPI_SLAVE_RX_DMA_CH);
		packetlen = (SPI_DATA_NUM - packetlen) -1;
		SPI_DISABLE_RX_PDMA(BridgeSpiPortNum);
		
		set_flag(flag_SPI_Slave_RX , ENABLE);

		// #if 1	// debug
		// printf("SPI RX = (data len:0x%2X/%2d(%2d),packet len:0x%2X,checksum :0x%2X)\r\n" ,packetlen-4,packetlen-4,g_au8SlaveRxBuffer[1],packetlen,g_au8SlaveRxBuffer[packetlen-2]);
		// dump_buffer_hex(g_au8SlaveRxBuffer , SPI_DATA_NUM);
		// #endif

		// compare_buffer(g_au8MasterToSlaveTestPattern,g_au8SlaveRxBuffer,packetlen);
		
		// SPI_Slave_Rx_PDMA_Enable();	

		SPI_SET_SSINAIF_FLAG(BridgeSpiPortNum);			
	}
}

/*
	SPI slave SPI0 : PA0(MOSI)/PA1(MISO)/PA2(CLK)/PA3(SS)
*/
void SPI_Slave_Init(void)
{
	SYS_ResetModule(SPI0_RST);
    SPI_Open(BridgeSpiPortNum, SPI_SLAVE, SPI_MODE_0, 8, (uint32_t)NULL);

	SPI_ClearRxFIFO(BridgeSpiPortNum);

	SPI_SetFIFO(BridgeSpiPortNum , 4 , 4);

	SPI_EnableInt(BridgeSpiPortNum,SPI_SSINACT_INT_MASK );
    SPI_WRITE_TX(BridgeSpiPortNum, 0xFFFFFFFF);    /* Dummy Write to prevent TX under run */
    NVIC_EnableIRQ(SPI0_IRQn);
}


void SPI_Master_TX(SPI_T *spi , uint8_t *buffer , uint32_t len)
{
	uint32_t TxDataCount = 0;

    // /CS: active
    SPI_SET_SS_LOW(MasterSpiPortNum);

	#if 0
    while ((SPI_GET_TX_FIFO_FULL_FLAG(spi) == 0)&& (TxDataCount < len))
    {
        SPI_WRITE_TX(spi, buffer[TxDataCount]);
		TxDataCount++;
    }
	#else
    while (TxDataCount < len)
    {
        SPI_WRITE_TX(spi, buffer[TxDataCount++]);
		while(SPI_IS_BUSY(spi));
    }
	#endif

	// CS: de-active
    SPI_SET_SS_HIGH(MasterSpiPortNum);
}


/*
	SPI master SPI1 : PC0(SS)/PC1(CLK)/PC2(MOSI)/PC3(MISO)
*/
void SPI_Master_Init(void)
{
    SPI_Open(MasterSpiPortNum, SPI_MASTER, SPI_MODE_0, 8, SPI_TARGET_FREQ);

    SPI_DisableAutoSS(MasterSpiPortNum);
}

/*
	BYTE0 		: head : 0x5A
	BYTE1 		: length : 
	
	BYTE(n-1) 	: checksum :
	BYTE(n)tail : 0xA5
*/
void generate_random_SPI_TX_data(void)
{	
	uint8_t datalen = 0;
	uint8_t i = 0;
	uint8_t checksum = 0;
	uint8_t packetsize = 0;

	uint8_t head_byte = 1;
	uint8_t length_byte = 1;

	reset_buffer(g_au8MasterToSlaveTestPattern , 0x00, SPI_DATA_NUM);

	datalen = random(1 , SPI_DATA_NUM-4);	// buffer length is 64  , exclude head/length/checksum/tail

	for (i = 0 ; i < datalen  ; i++)
	{
		g_au8MasterToSlaveTestPattern[i+2] = i;
	}

	checksum = CRC8( (uint8_t *) &g_au8MasterToSlaveTestPattern[2] , datalen+(head_byte+length_byte) );

	g_au8MasterToSlaveTestPattern[0] = 0x5A;
	g_au8MasterToSlaveTestPattern[1] = datalen;	
	g_au8MasterToSlaveTestPattern[datalen+2] = checksum;	
	g_au8MasterToSlaveTestPattern[datalen+3] = 0xA5;
	packetsize = datalen + 4;	//head/length/checksum/tail

	#if 1	// debug
	printf("SPI TX = (data len:0x%2X/%2d ,packet len:0x%2X,checksum :0x%2X)\r\n" ,datalen,datalen,packetsize,checksum);
	dump_buffer_hex(g_au8MasterToSlaveTestPattern , packetsize);
	#endif

	SPI_Master_TX(MasterSpiPortNum , g_au8MasterToSlaveTestPattern,packetsize);

}

void loop(void)
{
	if (is_flag_set(flag_SPI_Slave_RX))
	{
		set_flag(flag_SPI_Slave_RX ,DISABLE);

		#if 1	// debug
		printf("SPI RX = (data len:0x%2X/%2d(%2d),packet len:0x%2X,checksum :0x%2X)\r\n" ,packetlen-4,packetlen-4,g_au8SlaveRxBuffer[1],packetlen,g_au8SlaveRxBuffer[packetlen-2]);
		dump_buffer_hex(g_au8SlaveRxBuffer , packetlen);
		#endif

		// compare_buffer(g_au8MasterToSlaveTestPattern,g_au8SlaveRxBuffer,packetlen);
		
		SPI_Slave_Rx_PDMA_Enable();	
	}

}

void TMR1_IRQHandler(void)
{
	// static uint32_t LOG = 0;
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
			// printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PH0 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				generate_random_SPI_TX_data();
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}


void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif
}

void Custom_Init(void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI0_MODULE);

    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI1_MODULE);	

    /* Enable EADC module clock */
    CLK_EnableModuleClock(EADC_MODULE);

    /* EADC clock source is 96MHz, set divider to 8, EADC clock is 96/8 MHz */
    CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(8));	

   CLK_EnableModuleClock(PDMA_MODULE);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

	/*
		SPI slave SPI0 : PA0(MOSI)/PA1(MISO)/PA2(CLK)/PA3(SS)
		SPI master SPI1 : PC0(SS)/PC1(CLK)/PC2(MOSI)/PC3(MISO)
	*/

    /* Setup SPI0 multi-function pins */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | SYS_GPA_MFPL_PA1MFP_SPI0_MISO | SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA3MFP_SPI0_SS);	

    /* Enable SPI0 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

    /* Enable SPI0 I/O high slew rate */
    GPIO_SetSlewCtl(PA, 0xF, GPIO_SLEWCTL_HIGH);

    /* Setup SPI1 multi-function pins */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC0MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk | SYS_GPC_MFPL_PC3MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC0MFP_SPI1_SS | SYS_GPC_MFPL_PC1MFP_SPI1_CLK | SYS_GPC_MFPL_PC2MFP_SPI1_MOSI| SYS_GPC_MFPL_PC3MFP_SPI1_MISO);	

    /* Enable SPI1 clock pin (PC1) schmitt trigger */
    PC->SMTEN |= GPIO_SMTEN_SMTEN1_Msk;

    /* Enable SPI1 I/O high slew rate */
    GPIO_SetSlewCtl(PC, 0xF, GPIO_SLEWCTL_HIGH);	

    PB->MODE &= ~(GPIO_MODE_MODE14_Msk);

    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB14MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB14MFP_EADC0_CH14);

    /* Disable the GPB0 - GPB3 digital input path to avoid the leakage current. */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT14);

    /* Enable temperature sensor */
    SYS->IVSCTL |= SYS_IVSCTL_VTEMPEN_Msk;

    /* Set reference voltage to external pin (3.3V) */
    SYS_SetVRef(SYS_VREFCTL_VREF_PIN);	
	
    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	Custom_Init();	
	TIMER1_Init();

	ADC_InitChannel(ADC0_CH14); 
    prepare_seed();  

	SPI_Master_Init();

	SPI_Slave_Init();
	SPI_Slave_Rx_PDMA_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
		loop();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

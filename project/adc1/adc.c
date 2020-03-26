#include "gd32f1x0.h"
#include "printf.h"

volatile uint32_t delay = 0;
uint16_t adc_value;

void SysTick_Handler(void)
{
        if (0 != delay) 
                delay--;
}

void delay_1ms(uint32_t count)
{
        delay = count;
        while(0 != delay);
}

void sys_putchar(char c)
{
        usart_data_transmit(USART0, (uint8_t)c);
        while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
}

int main(void)
{
        if (SysTick_Config(SystemCoreClock / 1000))
                while (1);
        NVIC_SetPriority(SysTick_IRQn, 0x00);
        
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_USART0);
        rcu_periph_clock_enable(RCU_DMA);
        rcu_periph_clock_enable(RCU_ADC);
        rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);
        
        gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_9);
        gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_10);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_10);
        
        usart_deinit(USART0);
        usart_baudrate_set(USART0, 115200U);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_enable(USART0);
        
        gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);
        
        adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);
        adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_SWRCST); 
        adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
        adc_channel_length_config(ADC_REGULAR_CHANNEL, 1);
        adc_regular_channel_config(0, ADC_CHANNEL_0, ADC_SAMPLETIME_239POINT5);
        
        adc_enable();
        adc_calibration_enable();
        
        while(1) {
                adc_flag_clear(ADC_FLAG_EOC);
                adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
                
                while(SET != adc_flag_get(ADC_FLAG_EOC));
                
                adc_value = ADC_RDATA;
                printf("ADC: %d\r\n", adc_value);
               
                delay_1ms(1000);
        }
}

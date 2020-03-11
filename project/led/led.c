#include "gd32f1x0.h"
#include <stdio.h>

volatile uint32_t delay;

void delay_1ms(uint32_t count)
{
        delay = count;
        while(0 != delay);
}

void SysTick_Handler(void)
{
        if (0 != delay) 
                delay--;
}

int main(void)
{
        if (SysTick_Config(SystemCoreClock / 1000))
                while (1);
        NVIC_SetPriority(SysTick_IRQn, 0x00);
        
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_1);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);    
        
        while(1) {
                GPIO_BOP(GPIOA) = GPIO_PIN_0;
                GPIO_BC(GPIOA) = GPIO_PIN_1;
                delay_1ms(1000);
                
                GPIO_BC(GPIOA) = GPIO_PIN_0;
                GPIO_BOP(GPIOA) = GPIO_PIN_1;
                delay_1ms(1000);
        }
}

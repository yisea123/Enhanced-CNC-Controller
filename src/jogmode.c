#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "cnc-controller.h"
#include "jogmode.h"

static uint32_t isInRect(uint32_t x, uint32_t y, uint32_t Height, uint32_t Width, uint32_t point_x, uint32_t point_y){
    if((point_x > x) && \
       (point_x < x + Width) && \
       (point_y > y) && \
       (point_y < y + Height)){
        return 1;
    }
    return 0;
}

void jogUI(void *pvParameters){
    TP_STATE *tp;

    while(1){
        tp = IOE_TP_GetState(); 

        if( tp->TouchDetected ){
            if(isInRect(60, 25, 60, 120, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(20, 0, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(60, 235, 60, 120, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(-20, 0, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(0, 110, 100, 50, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 20, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(190, 110, 100, 50, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, -20, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(60, 95, 60, 120, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 0, 20);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(60, 165, 60, 120, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 0, -20);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
}

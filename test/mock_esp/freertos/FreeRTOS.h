/* Mock FreeRTOS critical sections */
#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#define portENTER_CRITICAL_ISR(lock) ((void)0)
#define portEXIT_CRITICAL_ISR(lock)  ((void)0)

#endif

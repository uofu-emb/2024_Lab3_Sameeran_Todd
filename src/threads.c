#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define SIDE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define SIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t semaphore;

int counter;
int on;

// void side_thread(void *params)
// {
// 	while (1) {
//         vTaskDelay(100);
//         counter += counter + 1;
// 		printf("hello world from %s! Count %d\n", "thread", counter);
// 	}
// }

// void main_thread(void *params)
// {
// 	while (1) {
//         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
//         vTaskDelay(100);
// 		printf("hello world from %s! Count %d\n", "main", counter++);
//         on = !on;
// 	}
// }
// Core logic for incrementing counter
BaseType_t increment_counter(SemaphoreHandle_t semaphore, int *counter, int increment) {
    // Try to acquire semaphore
    if (xSemaphoreTake(semaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        *counter += increment;
        xSemaphoreGive(semaphore);
        return pdTRUE; // Success
    } else {
        return pdFALSE; // Failed to acquire semaphore
    }
}

// Main thread logic
void main_thread_logic() {
    increment_counter(semaphore, &counter, 1);
    printf("hello world from main! Count %d\n", counter);
}

// Side thread logic
void side_thread_logic() {
    if (increment_counter(semaphore, &counter, counter + 1)) {
        printf("hello world from side thread! Count %d\n", counter);
    } else {
        printf("side_thread: Semaphore timeout. Retrying...\n");
    }
}
void side_thread(void *params) {
    // while (1) {
    //     vTaskDelay(100);
    //     if (xSemaphoreTake(semaphore, pdMS_TO_TICKS(50))) {  // Timeout after 50ms
    //         counter += counter + 1;
    //         xSemaphoreGive(semaphore);
    //     } else {
    //         printf("side_thread: Semaphore timeout. Retrying...\n");
    //     }
    // }
     while (1) {
        side_thread_logic();
        vTaskDelay(100);
    }
}

void main_thread(void *params) {
    // while (1) {
    //     if (xSemaphoreTake(semaphore, portMAX_DELAY)) {
    //         counter++;
    //         xSemaphoreGive(semaphore);
    //     }
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
    //     printf("hello world from %s! Count %d\n", "main", counter);
    //     on = !on;
    //     vTaskDelay(100);
    // }
       while (1) {
        main_thread_logic();
        vTaskDelay(100);
    }
}

// Shared resources
SemaphoreHandle_t lockA;
SemaphoreHandle_t lockB;

// Thread 1: Acquires lockA first, then waits for lockB
void thread1(void *params) {
    printf("Thread 1: Trying to acquire lockA...\n");
    xSemaphoreTake(lockA, portMAX_DELAY); // Acquire lockA
    printf("Thread 1: Acquired lockA. Trying to acquire lockB...\n");

    // Simulate work
    vTaskDelay(pdMS_TO_TICKS(100));

    xSemaphoreTake(lockB, portMAX_DELAY); // Wait for lockB
    printf("Thread 1: Acquired lockB.\n");

    // Release locks
    xSemaphoreGive(lockB);
    xSemaphoreGive(lockA);

    vTaskDelete(NULL);
}

// Thread 2: Acquires lockB first, then waits for lockA
void thread2(void *params) {
    printf("Thread 2: Trying to acquire lockB...\n");
    xSemaphoreTake(lockB, portMAX_DELAY); // Acquire lockB
    printf("Thread 2: Acquired lockB. Trying to acquire lockA...\n");

    // Simulate work
    vTaskDelay(pdMS_TO_TICKS(100));

    xSemaphoreTake(lockA, portMAX_DELAY); // Wait for lockA
    printf("Thread 2: Acquired lockA.\n");

    // Release locks
    xSemaphoreGive(lockA);
    xSemaphoreGive(lockB);

    vTaskDelete(NULL);
}

int main(void)
{
    stdio_init_all();
    // hard_assert(cyw43_arch_init() == PICO_OK);
    // on = false;
    // counter = 0;
    // TaskHandle_t main, side;
    // semaphore = xSemaphoreCreateCounting(1, 1);
    // // xTaskCreate(main_thread, "MainThread",
    // //             MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &main);
    // // xTaskCreate(side_thread, "SideThread",
    // //             SIDE_TASK_STACK_SIZE, NULL, SIDE_TASK_PRIORITY, &side);
    // xTaskCreate(main_thread, "MainThread",
    //             configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &main);
    // xTaskCreate(side_thread, "SideThread",
    //             configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &side);

// Initialize semaphores
    lockA = xSemaphoreCreateMutex();
    lockB = xSemaphoreCreateMutex();

    // Create threads
    TaskHandle_t thread1Handle, thread2Handle;
    xTaskCreate(thread1, "Thread 1", configMINIMAL_STACK_SIZE, NULL, 1, &thread1Handle);
    xTaskCreate(thread2, "Thread 2", configMINIMAL_STACK_SIZE, NULL, 1, &thread2Handle);


    vTaskStartScheduler();
	return 0;
}

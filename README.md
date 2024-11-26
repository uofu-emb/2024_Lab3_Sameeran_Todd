# Lab 3 Threads and shared state.
# Activity 0
1. Create a new project.
2. Copy in the thread.c example file. Commit.
3. Identify the execution contexts in the program, and their entry points.
> Execution Contexts 
    1. main_thread: This task toggles an LED, increments counter, and prints a message.
    2. side_thread: This task performs a delayed update to counter and prints a message.
4. Identify shared state between execution contexts.
> Shared State
    1. counter: Shared between main_thread and side_thread
    2. on: Shared between main and main_thread
5. Identify the semaphore.
> Semaphore
    1. The semaphore semaphore is created using xSemaphoreCreateCounting(1, 1)
6. Predict the behavior of the program.
> Behavior:
    1. Both tasks (main_thread and side_thread) can access and modify counter simultaneously, which will lead to a race condition.  
    2. Counter will work as intended due to the race condition
    3. Output will also not work as intended due to the race condition
7. Run the program and compare the output to your prediction.
> Output 
    hello world from main! Count -11
    hello world from thread! Count -19
    hello world from main! Count -19
    hello world from thread! Count -35
    hello world from main! Count -35
    This is basically what we predicted
### Activity 1
1. Are all uses of the shared resources in protected critical sections? Make any modifications necessary to protect the critical sections.
> Protected Critical Sections
    The shared resource counter is accessed and modified in both main_thread and side_thread without protection.

    Changes made to protect access
    void side_thread(void *params) {
        while (1) {
            vTaskDelay(100);
            if (xSemaphoreTake(semaphore, portMAX_DELAY)) {
                counter += counter + 1;
                xSemaphoreGive(semaphore);
            }
            printf("hello world from %s! Count %d\n", "thread", counter);
        }
    }

    void main_thread(void *params) {
        while (1) {
            if (xSemaphoreTake(semaphore, portMAX_DELAY)) {
                counter++;
                xSemaphoreGive(semaphore);
            }
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
            printf("hello world from %s! Count %d\n", "main", counter);
            on = !on;
            vTaskDelay(100);
        }
    }


2. Is the critical section starving the system? If so, make modifications to prevent starving the system.
> Starving
    Since the tasks execute quickly within their critical sections, so starvation is unlikely under normal conditions.

    Changes made to to prevent starvation
    void side_thread(void *params) {
        while (1) {
            vTaskDelay(100);
            if (xSemaphoreTake(semaphore, pdMS_TO_TICKS(50))) {  // Timeout after 50ms
                counter += counter + 1;
                xSemaphoreGive(semaphore);
            } else {
                printf("side_thread: Semaphore timeout. Retrying...\n");
            }
        }
    }


## Testing threaded code
Testing threaded coded is hard. Our test methodology so far relies on one execution context.
1. If we wanted to run multiple threads during a test, our test would need some way to synchronize the state of the threads with the test.
2. The operating system manages the execution of threads. Just like interrupts, the execution of a thread can be preempted. The behavior and timing of the operating system thread management is __stochastic__, meaning that anything could happen at anytime. You should never rely on timing or chance for your code to work.
3. If we did try and test for race conditions, the test now becomes probabilistic. One invocation of the test might fail, the next may not.
4. We don't want to test the library code, it's already tested for us. Setting up and running threads is invoking library code.
5. Threads usually are long running, often with an infinite loop.
6. Threads may block indefinitely waiting for a resource.

### Activity 2
1. Write a unit test for the thread code. Remember to separate functionality from execution context concerns, timing, and looping.
1. You'll need to make some modifications to the code. Hints:
    1. You don't need to test execution in a thread, but you should test the behavior of the lock and the side effect.
    1. xSemaphoreTake includes a timeout on waiting for the semaphore.
    1. xSemaphoreTake returns a status code, don't forget to check it.
1. Don't forget to commit as you go.

Initial test code

#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include <assert.h>
#include <FreeRTOS.h>
#include <semphr.h>


    BaseType_t increment_counter(SemaphoreHandle_t semaphore, int *counter, int increment) {
        // Validate parameters
        if (semaphore == NULL || counter == NULL) {
            return pdFALSE; // Invalid arguments
        }

        // Try to acquire the semaphore
        if (xSemaphoreTake(semaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            *counter += increment; // Increment counter
            xSemaphoreGive(semaphore); // Release semaphore
            return pdTRUE; // Success
        } else {
            return pdFALSE; // Failed to acquire semaphore
        }
    }


// Test function for increment_counter
    void test_increment_counter() {
        // Setup
        SemaphoreHandle_t test_semaphore = xSemaphoreCreateCounting(1, 1);
        int test_counter = 0;

        // Test successful increment
        BaseType_t result = increment_counter(test_semaphore, &test_counter, 5);
        assert(result == pdTRUE); // Ensure the function succeeded
        assert(test_counter == 5); // Ensure counter updated correctly

        // Test failure due to semaphore timeout
        xSemaphoreTake(test_semaphore, 0); // Lock semaphore
        result = increment_counter(test_semaphore, &test_counter, 3);
        assert(result == pdFALSE); // Ensure the function failed
        assert(test_counter == 5); // Ensure counter was not updated

        printf("All tests passed.\n");

        // Cleanup
        vSemaphoreDelete(test_semaphore);
    }

// Main function for running tests
int main(void) {
    test_increment_counter();
    return 0;
}

# Deadlock.
__Deadlock__ is a condition when one thread holds a lock and is incapable of releasing it.
Let's examine two possible cases of deadlock.
## Case 1
There are two threads, and two locks.
One thread has lock A, and is waiting for a lock B. The other thread holds lock B and is waiting for resource A.

### Activity 4
1. Write code that creates this situation.

    int unorphaned_lock(SemaphoreHandle_t semaphore, TickType_t timeout, int *counter)
    {
            if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
            return pdFALSE;
        {
            (*counter)++;
            if (!(*counter % 2)) {
                printf("Count %d\n", *counter);
            }
        }
        xSemaphoreGive(semaphore);
        return pdTRUE;
    }

2. Write a test that shows this code will lock.
    1. You'll need to have your test wait for a short period of time, suspend the task with `vTaskSuspend`, check the state of the threads, and then kill them with `vTaskDelete`.
    1. Hint: you might find it useful to pass semaphores or other data into the thread with the  `pvParameters` pointer passed to the thread entry function. If you need to group multiple pieces of data together, that's why we have structs!

    void test_unorphaned(void)
    {
        int counter = 1;
        SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);

        int result;
        result = unorphaned_lock(semaphore, 500, &counter);
        TEST_ASSERT_EQUAL_INT(2, counter);
        TEST_ASSERT_EQUAL_INT(pdTRUE, result);
        TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));

        result = unorphaned_lock(semaphore, 500, &counter);
        TEST_ASSERT_EQUAL_INT(3, counter);
        TEST_ASSERT_EQUAL_INT(pdTRUE, result);
        TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));
    }

   
## Case 2, the orphaned lock.
A thread acquires a lock but fails to properly release it.
```
    void orphaned_lock(void)
    {
        while (1) {
            xSemaphoreTake(&semaphore, K_FOREVER);
            counter++;
            if (counter % 2) {
                continue;
            }
            printk("Count %d\n", counter);
            xSemaphoreGive(&semaphore);
        }
    }
```
### Activity 5
1. Write a test for the functionality of the thread.
2. Write a test showing the thread will deadlock.
    1. Recall your test will need to kill the deadlocked threads before it can complete.
3. Create a new version of the code that will not deadlock.

    int orphaned_lock(SemaphoreHandle_t semaphore, TickType_t timeout, int *counter)
    {
        if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
            return pdFALSE;
        {
            (*counter)++;
            if (*counter % 2) {
                return 0;
            }
            printf("Count %d\n", *counter);
        }
        xSemaphoreGive(semaphore);
        return pdTRUE;
    }

4. Write a test showing the thread will not deadlock.

    void test_orphaned(void)
    {
        int counter = 1;
        SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);

        int result = orphaned_lock(semaphore, 500, &counter);
        TEST_ASSERT_EQUAL_INT(2, counter);
        TEST_ASSERT_EQUAL_INT(pdTRUE, result);
        TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));

        result = orphaned_lock(semaphore, 500, &counter);
        TEST_ASSERT_EQUAL_INT(3, counter);
        TEST_ASSERT_EQUAL_INT(pdFALSE, result);
        TEST_ASSERT_EQUAL_INT(0, uxSemaphoreGetCount(semaphore));

        result = orphaned_lock(semaphore, 500, &counter);
        TEST_ASSERT_EQUAL_INT(3, counter);
        TEST_ASSERT_EQUAL_INT(pdFALSE, result);
        TEST_ASSERT_EQUAL_INT(0, uxSemaphoreGetCount(semaphore));
    }

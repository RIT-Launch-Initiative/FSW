/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/os/n_rtos.h>
#include <zephyr/kernel.h>
#include <functional>

std::function<void(void*, void*, void*)> g_lambda;
// Wrapper function that matches the k_thread_entry_t signature
void wrapper(void *p1, void *p2, void *p3) {
    g_lambda(p1, p2, p3);
}

// Function to create a lambda matching k_thread_entry_t signature
k_thread_entry_t CreateThreadEntry(CTask& task) {
    // Assign the lambda to the global std::function
    g_lambda = [&task](void *p1, void *p2, void *p3) {
        task.Run();
    };
    // Return the wrapper function
    return &wrapper;
}

void NRtos::AddTask(CTask &task, size_t stack_size, int stack_flags) {
    k_thread_entry_t threadEntry = CreateThreadEntry(task);

    std::function<void(void*, void*, void*)> lambda;


    k_thread_stack_t *stack = nullptr;
    size_t stackSize = task.GetStack(stack);
}

void NRtos::StartRtos() {
}

void NRtos::StopRtos() {
    // Stop RTOS
}


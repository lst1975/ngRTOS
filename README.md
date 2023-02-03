# ngRTOS
A Tiny Realtime-Operating-System (STM32 devices); ngRTOS = Next Generation Realtime Operating System.

ngRTOS is a real-time operating system designed to support embedded and IoT applications. ngRTOS provides real-time reliability for embedded devices with flexible scheduling, low latency, and small memory footprint. It supports multitasking, provides multiple functions such as queuing, semaphores, secure task management, multi-priority task scheduling, task preemption, and time-slice polling, and provides a variety of common business applications to help developers build efficient and reliable embedded solutions.

# License
ULB1Dv1: Unlimited License but for one Declaration.

# Core Files
src/sys/ng_task.c

src/sys/ng_channel.c

# Memory
1. TLSF (https://github.com/mattconte/tlsf)
2. PEAK (Support Memory Leak Detect, need gracefully shutdown the system)
3. PEAK on TLSF (Support Memory Leak Detect, need gracefully shutdown the system)

   [Our suggestion is to use CHOICE 3.]

# Status
Under construction ...

ng_time.c is unfinished ...

# Email: 
980680431@qq.com

# Chips

![stm32](https://user-images.githubusercontent.com/28725147/211881021-549a4bdc-c3e2-4581-9ccc-b195aa9f7f2e.jpg)

# Code

![image](https://user-images.githubusercontent.com/28725147/212552325-978f46df-7dea-4188-a21a-a215a04d89f3.png)

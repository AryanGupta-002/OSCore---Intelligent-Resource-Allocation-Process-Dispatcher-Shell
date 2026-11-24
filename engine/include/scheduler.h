// Code by Bibham Bibhuti
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"


// Yahan hum saare CPU scheduling algorithms ko ek common name de rahe hai....
// Taaki main file mein easily identify kar sakein ki konsa algorithm select hua hai.
typedef enum {
    ALGORITHM_FCFS,          // First Come First Serve
    ALGORITHM_SJF,           // Shortest Job First
    ALGORITHM_PRIORITY,      // Priority Scheduling
    ALGORITHM_ROUND_ROBIN,   // Round Robin Scheduling
    ALGORITHM_SRTF,          // Shortest Remaining Time First
    ALGORITHM_MLFQ           // Multilevel Feedback Queue
} AlgorithmType;


// Yahan hum saare scheduling algorithms ke function declare kar rahe hain.
// Inki actual implementation respective .c files mein likhi gayi hai.


// FCFS mein jo process sabse pahle aata hai usko CPU pahle execute karta hai.
int schedule_fcfs(
    Process *processes[],   // Yahan saare processes ke addresses store honge
    int process_count       // Total kitne processes scheduling ke liye available hai idhar 
);


// SJF mein available processes mein se sabse kam burst time wale process ko select karenge.... 
int schedule_sjf(
    Process *processes[],
    int process_count
);


// Priority scheduling mein process ki priority ke according execution order decide hoga....
// Agar priority ka rule change karna ho toh uski implementation wali .c file mein changes karenge.
int schedule_priority(
    Process *processes[],
    int process_count
);


// Round Robin mein har process ko execute hone ke liye ek fixed time diya jata hai.
// Agar time quantum change karna ho toh uski value yahan parameter ke through pass karenge.
int schedule_round_robin(
    Process *processes[],
    int process_count,
    int time_quantum       // Har process ko ek turn mein kitna CPU time milega
);


// SRTF mein jis process ka remaining burst time sabse kam hota hai usko CPU milta hai....
// Agar koi naya process kam remaining time ke saath aaye toh current process preempt ho sakta hai....
int schedule_srtf(
    Process *processes[],
    int process_count
);


// MLFQ mein multiple queues hoti hai aur har queue ka priority alag hota hai....
// Process ke execution ke behaviour ke according uska queue level change ho sakta hai...
int schedule_mlfq(
    Process *processes[],
    int process_count
);


#endif

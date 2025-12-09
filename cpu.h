#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define MEM_SIZE 65536

// The System State
typedef struct {
    uint16_t memory[MEM_SIZE];
    uint16_t registers[8];
    uint16_t pc;   
    uint16_t ir;
    uint16_t accum;
    uint16_t mar;
    uint16_t mbr;
    bool running;
    
    // Flags
    bool zero_flag;
    bool neg_flag;
    bool overflow_flag;
    bool carry_flag;
} System;

// Function Prototypes (Promises that these functions exist)
void init_system(System *sys);
void step_cpu(System *sys);

#endif
#include "cpu.h"
#include <stdio.h>

void init_system(System *sys) {
    for (int i = 0; i < MEM_SIZE; i++) sys->memory[i] = 0;
    for (int i = 0; i < 8; i++) sys->registers[i] = 0;
    sys->pc = 0x0000; 
    sys->registers[0] = 0xFFFF; //SP 
    sys->running = true;
}

void step_cpu(System *sys) {
    // 1. Fetch
    uint16_t instruction = sys->memory[sys->pc];
    sys->pc++;

    // 2. Decode
    uint16_t opcode = (instruction >> 12) & 0xF;
    uint16_t dest   = (instruction >> 9) & 0x7;
    printf(" inst = %u\n", instruction);

    // 3. Execute
    switch (opcode) {
        case 0x0: // HLT
            sys->running = false;
            break;
            
        case 0x1: { // ADD 
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] += immd;
                //printf("ADDI %u %u => %u\n", dest, immd, sys->registers[dest]);       
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] += sys->registers[src];
                //printf("ADD %u %u => %u\n", dest, src, sys->registers[dest]);
            }     
            break;
        }
        
        case 0x2:{ // SUB
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] -= immd;
                //printf("SUBI %u %u => %u\n", dest, immd, sys->registers[dest]);
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] -= sys->registers[src];
                //printf("SUB %u %u => %u\n", dest, src, sys->registers[dest]);
            }     
            break;
        }
        
        case 0x3:{ // MUL
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] *= immd;
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] *= sys->registers[src];
            }     
            break;
        }
        
        case 0x4:{ // DIV
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] /= immd;
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] /= sys->registers[src];
            }     
            break;
        }

        case 0x5:{ // AND
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] &= immd;
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] &= sys->registers[src];
            }     
            break;
        }

        case 0x6:{ // OR
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] |= immd;
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] |= sys->registers[src];
            }     
            break;
        }

        case 0x7:{ // XOR
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] ^= immd;
                //printf("XORI %u %u => %u\n", dest, immd, sys->registers[dest]);
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] ^= sys->registers[src];
                //printf("XOR %u %u => %u\n", dest, src, sys->registers[dest]);
            }     
            break;
        }

        case 0x8:{ //SHIFT
            bool imm = instruction & 1;
            uint8_t mode = (instruction >> 1) & 0x3;
            uint8_t amount;
            if (imm){
                amount = (instruction >> 0x3) & 0x3F;
            } else{
                amount = sys->registers[(instruction >> 0x3) & 0x3F];
            }
            //printf("%d\n", mode);
            if (mode == 0){
                sys->registers[dest] = sys->registers[dest] << amount;
                //printf("SHFL %u %u => %u\n", dest, amount, sys->registers[dest]);
            } else if (mode == 1){
                sys->registers[dest] = sys->registers[dest] >> amount;
                //printf("SHFLR %u %u => %u\n", dest, amount, sys->registers[dest]);
            } else if (mode == 2){
                sys->registers[dest] = (int16_t)sys->registers[dest] >> amount;
                //printf("SHFAR %u %u => %u\n", dest, amount, sys->registers[dest]);
            } else{
                sys->registers[dest] = (sys->registers[dest] >> amount) | (sys->registers[dest] << (16 - amount));
                //printf("SHFRO %u %u => %u\n", dest, amount, sys->registers[dest]);
            }
            break;
        }

        case 0x9:{ //MOVE
            if (instruction & 1){
                uint8_t immd = (instruction >> 1) & 0xFF;
                sys->registers[dest] = immd;
                //printf("MOVI %u %u => %u\n", dest, immd, sys->registers[dest]);
            } else {
                uint8_t src = (instruction >> 1) & 0x7;
                sys->registers[dest] = sys->registers[src];
                //printf("MOV %u %u => %u\n", dest, src, sys->registers[dest]);
            }
            break;
        }

        case 0xA: { // LD (Memory Load)
            uint8_t r_data = (instruction >> 9) & 0x7; 
            uint8_t r_base = (instruction >> 6) & 0x7;
            int8_t offset = instruction & 0x3F;      

            if (offset & 0x20) {
                offset |= 0xC0; // Fill the top bits with 1s to make it a valid negative C integer
            }

            uint16_t addr = sys->registers[r_base] + offset;

            if (addr < 0x8000 && addr > 0x00FF) {
                //printf("SEGFAULT: Writing to Code Space at %X\n", addr);
                sys->running = false;
            } else {
                sys->registers[r_data] = sys->memory[addr];
            }
            break;
        }
            
        case 0xB: { // ST (Memory Store)
            uint8_t r_data = (instruction >> 9) & 0x7; 
            uint8_t r_base = (instruction >> 6) & 0x7;
            int8_t offset = instruction & 0x3F;      

            if (offset & 0x20) {
                offset |= 0xC0; 
            }

            uint16_t addr = sys->registers[r_base] + offset;

            if (addr < 0x8000 && addr > 0x00FF) {
                //printf("SEGFAULT: Writing to Code Space at %X\n", addr);
                sys->running = false;
            } else {
                sys->memory[addr] = sys->registers[r_data];
                //printf("ST %u %u => %u", r_data, addr, sys->memory[addr]);
            }
            break;
        }

        case 0xC: { //STACK
            uint8_t mode = instruction & 0x3;
            if (mode == 0) {
                if (sys->registers[0] < 0xF000) {
                    //printf("ERROR: Stack Overflow!\n");
                    sys->running = false;
                    return;
                }
                uint16_t val = sys->registers[(instruction >> 2) & 0x7];
                sys->registers[0]--; 
                sys->memory[sys->registers[7]] = val; 
            } else if (mode == 1) {
                if(sys->registers[0] > 0xFFFF){
                    //printf("ERROR: Stack Underflow!\n");
                    sys->running = false;
                    return;
                }
                uint16_t val = sys->memory[sys->registers[7]]; 
                sys->registers[0]++; 
                sys->registers[(instruction >> 2) & 0x7] = val;
            } else if (mode == 2) {
                if (sys->registers[0] < 0xF000) {
                    //printf("ERROR: Stack Overflow!\n");
                    sys->running = false;
                    return;
                }
                uint16_t imm = (instruction >> 2) & 0x3FF; 
                sys->registers[0]--; 
                sys->memory[sys->registers[7]] = imm; 
            } 
        }

        case 0xD: { //CMP
            uint8_t src = (instruction >> 6) & 0x7; 
            uint16_t val1 = sys->registers[dest];
            uint16_t val2 = sys->registers[src];
            int16_t Result = val1 - val2;
            uint16_t A_u = (uint16_t)val1;
            uint16_t B_u = (uint16_t)val2;
            
            sys->zero_flag = (Result == 0);
            sys->neg_flag = (Result < 0);
            sys->carry_flag = (A_u < B_u);
            
            int val1_sign = (val1 >> 15) & 1; 
            int val2_sign = (val2 >> 15) & 1; 
            int R_sign = (Result >> 15) & 1; 

            if (val1_sign == 0 && val2_sign == 1 && R_sign == 1) {
                sys->overflow_flag = 1;
            } else if (val1_sign == 1 && val2_sign == 0 && R_sign == 0) {
                sys->overflow_flag = 1;
            } else {
                sys->overflow_flag = 0;
            }
            //printf("CMP R%u, R%u. Flags: Z=%d, N=%d, C=%d V=%d\n", dest, src, sys->zero_flag, sys->neg_flag, sys->carry_flag, sys->overflow_flag);
            break;
        }

        case 0xE:{ // BR
            uint8_t cond = instruction & 0x7; 
            uint16_t offset = (instruction >> 3) & 0x1FF;

            if (offset & 0x100) { 
                offset |= 0xFE00; 
            }

            bool jump = false;
            switch (cond) {
                case 0: jump = sys->zero_flag; break;        // ==
                case 1: jump = !sys->zero_flag; break;       // !=
                case 2: jump = !sys->neg_flag && !sys->zero_flag; break; // >
                case 3: jump = sys->neg_flag; break;         // <
                case 4: jump = !sys->neg_flag || sys->zero_flag; break;       // >=
                case 5: jump = sys->neg_flag || sys->zero_flag; break;      // <=
                case 6: jump = true; break;                  
            }

            if (jump) 
                sys->pc += offset;
            break;
        }

        case 0xF:{ // FUNC 
            if ((instruction & 1) == 0) {
                uint16_t offset = (instruction >> 1) & 0x7FF; 
                if (offset & 0x400) offset |= 0xF800;

                sys->registers[0]--; 
                sys->memory[sys->registers[7]] = sys->pc;
                sys->pc += offset;
            } else {
                uint16_t return_addr = sys->memory[sys->registers[7]];
                sys->registers[7]++; 
                sys->pc = return_addr;
            }
            break;
        }
            
        default:
            printf("Unknown Opcode: %X at %X\n", opcode, sys->pc);
            sys->running = false;
    }
}
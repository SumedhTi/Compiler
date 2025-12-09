# 16-Bit Virtual Machine & Custom Compiler

A full-stack implementation of a custom 16-bit CPU architecture.  
This project includes a **Virtual Machine** (written in C) that emulates the hardware and an **Assembler** (currently) that translates custom assembly language into machine code.

The system features a 2-operand RISC-like instruction set, memory-mapped I/O, and a 64×64 pixel display output.

---

## 🚀 Key Features

- **Custom ISA:** Designed a unique 16-bit Instruction Set Architecture (ISA) from scratch.  
- **Graphics Output:** Simulates a 64×64 video display using SDL3 (memory-mapped at `0xE000`).  

---

## 🧩 Architecture Overview

- **Word Size:** 16-bit  
- **Memory:** 65,536 words (64KB address space)  
- **Registers:** 8 general-purpose registers (`R0` – `R7`)  
  - `R0` is set as the Stack Pointer (SP)  
- **Instruction Format:** 2-operand (Dest, Src/Immediate)

---

## 📦 Memory Map

| Address Range       | Function                     |
|---------------------|------------------------------|
| `0x0000 – 0x7FFF`   | Program Code (32 KB)         |
| `0x8000 – 0xDFFF`   | Data / Heap (Variables)      |
| `0xE000 – 0xEFFF`   | Video Memory (64×64 Display) |
| `0xF000 – 0xFFFF`   | Stack                        |

---

## 🧮 Instruction Set (Opcodes)

For detailed bit-wise breakdown and instruction formats, see the included Excel file:  
[ISA Design Sheet.xlsx](Documents/ops.xlsx)

### Supported Mnemonics

**Math:** `ADD`, `SUB`, `MUL`, `DIV`,   
**Logic:** `AND`, `OR`, `XOR`, `SHF`, `CMP`  
**Memory:** `LD` (Load), `ST` (Store), `MOV`  
**Control Flow:** `BR` (Conditional Branch and jumps), `FUNC` (Function Call/Ret)  
**System:** `HLT` (Halt), `STACK` (Push/Pop)

---

## ▶️ How to Run

### Prerequisites

- **C Compiler:** GCC (MinGW on Windows)  
- **Library:** SDL3 (for graphics)

### 1. Assemble Code

Write your assembly in `program.asm`, then generate the binary by compiling and running the assembler.c:

```bash
gcc assembler.c
./assembler.exe 
OR 
./a.out
# Output: program.bin
```

### 2. Build the VM

Compile the C Virtual Machine (ensure SDL2 is linked):

```bash
gcc -g main.c CPU.c -o my_vm -I <SDL3 Include file path> -L <SDL3 lib path> -lSDL3
```

### 3. Run

```bash
./my_vm
```

---

## 🖥️ Visual Demo

Writing to address `0xE000` updates the screen instantly.

### Example Assembly to make the screen white

```asm
_start:
  MOV R1, #0
  SUB R1, #1      ; R1 = 0xFFFF (White)
  MOV R2, #14
  SHF R2, #12     ; R2 = 0xE000 (VRAM Start)
  ST R1, [R2]     ; Write white pixel
```

---

## 👤 Author

- **Sumedh Tiwari**  
- **College:** KIIT University  
- **Course:** B.Tech Computer Science Engineering  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// --- 1. DEFINE YOUR ISA ---
// Using structs and lookup functions to emulate Python dictionaries
typedef struct {
    const char* key;
    uint16_t value;
} Opcode;

Opcode OPCODES[] = {
    {"HLT", 0b0000}, {"ADD", 0b0001}, {"SUB", 0b0010}, {"MUL", 0b0011},
    {"DIV", 0b0100}, {"AND", 0b0101}, {"OR",  0b0110}, {"XOR", 0b0111},
    {"SHF", 0b1000}, {"MOV", 0b1001}, {"LD",  0b1010}, {"ST",  0b1011},
    {"STACK", 0b1100}, {"CMP", 0b1101}, {"BR",  0b1110}, {"FUNC",0b1111},
    // Aliases for SHF
    {"SHL", 0b1000}, {"SHR", 0b1000}, {"SAR", 0b1000}, {"ROR", 0b1000},
    // Aliases for STACK
    {"PUSH", 0b1100}, {"POP", 0b1100},
    // Aliases for FUNC
    {"CALL", 0b1111}, {"RET", 0b1111}
};
int NUM_OPCODES = sizeof(OPCODES) / sizeof(Opcode);

typedef struct {
    const char* key;
    uint8_t value;
} Register;

Register REGISTERS[] = {
    {"r0", 0b000}, {"r1", 0b001}, {"r2", 0b010}, {"r3", 0b011},
    {"r4", 0b100}, {"r5", 0b101}, {"r6", 0b110}, {"r7", 0b111}
};
int NUM_REGISTERS = sizeof(REGISTERS) / sizeof(Register);

// Helper to find opcode value from mnemonic
int find_opcode(const char* mnemonic, uint16_t* value) {
    for (int i = 0; i < NUM_OPCODES; i++) {
        if (strcasecmp(OPCODES[i].key, mnemonic) == 0) {
            *value = OPCODES[i].value;
            return 1; // Found
        }
    }
    return 0; // Not found
}

// Helper to find register value from name
int find_register(const char* name, uint8_t* value) {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (strcasecmp(REGISTERS[i].key, name) == 0) {
            *value = REGISTERS[i].value;
            return 1; // Found
        }
    }
    return 0; // Not found
}

// Data structures for two-pass assembly
typedef struct {
    char* name;
    int address;
} LabelInfo;

typedef struct {
    char* line;
    char* label_name; // The label this line belongs to
} AssemblyLine;

// Struct to hold the result of assembly
typedef struct {
    uint16_t* instructions;
    int count;
} BinaryOutput;

// Helper to trim whitespace from a string (in-place)
char* trim(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

BinaryOutput assemble(const char* source_code) {
    char* source_copy = strdup(source_code);
    char* line_saveptr = NULL;

    AssemblyLine* assembly_lines = NULL;
    int line_count = 0;
    LabelInfo* labels = NULL;
    int label_count = 0;

    // --- Pass 1: Parse lines and identify labels ---
    int program_counter = 0;
    for (char* line = strtok_r(source_copy, "\n", &line_saveptr); line != NULL; line = strtok_r(NULL, "\n", &line_saveptr)) {
        // Remove comments
        char* comment = strchr(line, ';');
        if (comment) *comment = '\0';
        
        char* trimmed_line = trim(line);
        if (strlen(trimmed_line) == 0) continue; // Skip empty lines

        char* label_part = strchr(trimmed_line, ':');
        if (label_part) {
            *label_part = '\0'; // Split label name from the rest of the line
            labels = realloc(labels, (label_count + 1) * sizeof(LabelInfo));
            labels[label_count].name = strdup(trimmed_line);
            labels[label_count].address = program_counter;
            label_count++;
            trimmed_line = trim(label_part + 1);
        }

        if (strlen(trimmed_line) > 0) {
            assembly_lines = realloc(assembly_lines, (line_count + 1) * sizeof(AssemblyLine));
            assembly_lines[line_count].line = strdup(trimmed_line);
            assembly_lines[line_count].label_name = (label_count > 0) ? labels[label_count - 1].name : "_start";
            line_count++;
            program_counter++;
        }
    }

    // --- Pass 2: Generate machine code ---
    uint16_t* binary_output_data = malloc(line_count * sizeof(uint16_t));
    int instruction_count = 0;

    for (int i = 0; i < line_count; i++) {
        char* line_copy = strdup(assembly_lines[i].line);
        char* token_saveptr = NULL;
        char* mnemonic = strtok_r(line_copy, " \t", &token_saveptr);

        uint16_t opcode_val;
        if (!find_opcode(mnemonic, &opcode_val)) {
            fprintf(stderr, "Error: Unknown opcode %s\n", mnemonic);
            free(line_copy);
            // Proper cleanup would be needed here in a real application
            BinaryOutput result = {NULL, 0};
            return result;
        }

        uint16_t machine_code = 0;

        //Bit Map: [Op:4] [Unused:12]
        if (strcasecmp(mnemonic, "HLT") == 0) {
            machine_code = (opcode_val << 12);
        }
        
        // Bit Map: [Op:4] [Rd:3] [Imm:8] [Mode:1]
        else if (strcasecmp(mnemonic, "ADD") == 0 || strcasecmp(mnemonic, "SUB") == 0 || strcasecmp(mnemonic, "MUL") == 0 || strcasecmp(mnemonic, "DIV") == 0 || strcasecmp(mnemonic, "AND") == 0 || strcasecmp(mnemonic, "OR") == 0 || strcasecmp(mnemonic, "XOR") == 0 || strcasecmp(mnemonic, "MOV") == 0 || strcasecmp(mnemonic, "CMP") == 0) {
            char* dest_str = strtok_r(NULL, " \t", &token_saveptr);
            char* src_str = strtok_r(NULL, " \t", &token_saveptr);
            
            uint8_t rd;
            find_register(dest_str, &rd);

            if (src_str[0] == '#') {
                int imm_val = atoi(&src_str[1]);
                if (imm_val > 255) { fprintf(stderr, "Immediate too big for 8 bits\n"); exit(1); }
                machine_code = (opcode_val << 12) | (rd << 9) | (imm_val << 1) | 0b1;
            } else {
                uint8_t rs;
                find_register(src_str, &rs);
                machine_code = (opcode_val << 12) | (rd << 9) | (rs << 1) | 0b0;
            }
        }
        
        //Bit Map: [Op:4] [Rd;3] [Amnt:6] [Mode:3]
        else if (strcasecmp(mnemonic, "SHL") == 0 || strcasecmp(mnemonic, "SHR") == 0 || strcasecmp(mnemonic, "SAR") == 0 || strcasecmp(mnemonic, "ROR") == 0) {
            char* dest_str = strtok_r(NULL, " \t", &token_saveptr);
            char* amount_str = strtok_r(NULL, " \t", &token_saveptr);
            uint8_t rd;
            find_register(dest_str, &rd);

            int amount;
            if (amount_str[0] == '#') {
                amount = atoi(&amount_str[1]);
            } else {
                uint8_t rs_val;
                find_register(amount_str, &rs_val);
                amount = rs_val;
            }
            if (amount > 63) { fprintf(stderr, "Amount too big for 6 bits\n"); exit(1); }
            
            int mode = 0;
            if (strcasecmp(mnemonic, "SHR") == 0) mode = 1;
            else if (strcasecmp(mnemonic, "SAR") == 0) mode = 2;
            else if (strcasecmp(mnemonic, "ROR") == 0) mode = 3;

            machine_code = (opcode_val << 12) | (rd << 9) | (amount << 3) | mode | (amount_str[0] == '#');
        }
        
        //Bit Map: [Op:4] [Rd:3] [Rs:3] [Offset:6]
        else if (strcasecmp(mnemonic, "LD") == 0 || strcasecmp(mnemonic, "ST") == 0) {
            char* dest_str = strtok_r(NULL, " \t", &token_saveptr);
            char* src_str = strtok_r(NULL, " \t[]", &token_saveptr);
            uint8_t rd;
            find_register(dest_str, &rd);

            char rs_name[3] = {src_str[0], src_str[1], '\0'};
            uint8_t rs;
            find_register(rs_name, &rs);
            int offset = atoi(&src_str[2]);
            if (offset < -32 || offset > 31) { fprintf(stderr, "Offset too big for 6 bits signed\n"); exit(1); }

            machine_code = (opcode_val << 12) | (rd << 9) | (rs << 6) | (offset & 0x3F);
        }
        
        //Bit Map: [Op:4] [Rd:10] [Mode:2]
        else if (strcasecmp(mnemonic, "PUSH") == 0 || strcasecmp(mnemonic, "POP") == 0) {
            char* dest_str = strtok_r(NULL, " \t", &token_saveptr);
            int imm = (dest_str[0] == '#');
            
            int val;
            if (imm) {
                val = atoi(&dest_str[1]);
            } else {
                uint8_t reg_val;
                find_register(dest_str, &reg_val);
                val = reg_val;
            }
            if (val > 1023) { fprintf(stderr, "Value too big for 10 bits\n"); exit(1); }

            int mode = (strcasecmp(mnemonic, "POP") == 0);
            machine_code = (opcode_val << 12) | (val << 2) | (mode << 1) | imm;
        }
        
        //Bit Map: [Op:4] [Rs:9] [Mode:3] 
        else if (strcasecmp(mnemonic, "BR") == 0) {
            const char* modes[] = {"EQ", "NE", "GT", "LT", "GE", "LE"};
            char* token1 = strtok_r(NULL, " \t", &token_saveptr);
            char* token2 = strtok_r(NULL, " \t", &token_saveptr);
            
            int mode = 6; 
            char* lab_name = token1;

            for(int j=0; j<6; j++) {
                if(strcasecmp(token1, modes[j]) == 0) {
                    mode = j;
                    lab_name = token2;
                    break;
                }
            }
            
            int target_addr = -1;
            for(int j=0; j<label_count; j++) {
                if(strcmp(labels[j].name, lab_name) == 0) target_addr = labels[j].address;
            }
            if(target_addr == -1) { fprintf(stderr, "Label %s not found\n", lab_name); exit(1); }
            
            int offset = target_addr - (i + 1);
            if (offset < -256 || offset > 255) { fprintf(stderr, "Offset too big for 9 bits\n"); exit(1); }

            machine_code = (opcode_val << 12) | ((offset & 0x1FF) << 3) | mode;
        }
        
        //Bit Map: [Op:4] [Offset:11] [Mode:1]
        else if (strcasecmp(mnemonic, "CALL") == 0 || strcasecmp(mnemonic, "RET") == 0) {
            int mode = (strcasecmp(mnemonic, "RET") == 0);
            char* lab_name = strtok_r(NULL, " \t", &token_saveptr);
            
            int target_addr = -1;
            for(int j=0; j<label_count; j++) {
                if(strcmp(labels[j].name, lab_name) == 0) {
                    target_addr = labels[j].address;
                    break;
                }
            }
            if(target_addr == -1) { fprintf(stderr, "Label %s not found\n", lab_name); exit(1); }

            int offset = target_addr - (i + 1);
            if (offset < -1024 || offset > 1023) { fprintf(stderr, "Offset too big for 11 bits\n"); exit(1); }
            
            machine_code = (opcode_val << 12) | ((offset & 0x7FF) << 1) | mode;
        }
        
        else {
            fprintf(stderr, "Unknown or unimplemented opcode %s\n", mnemonic);
            exit(1);
        }
        
        binary_output_data[instruction_count++] = machine_code;
        free(line_copy);
    }

    // --- Cleanup ---
    free(source_copy);
    for (int i = 0; i < line_count; i++) free(assembly_lines[i].line);
    free(assembly_lines);
    for (int i = 0; i < label_count; i++) free(labels[i].name);
    free(labels);

    BinaryOutput result = {binary_output_data, instruction_count};
    return result;
}

// Helper to print a 16-bit number in binary
void print_binary16(uint16_t n) {
    for (int i = 15; i >= 0; i--) {
        putchar((n & (1 << i)) ? '1' : '0');
    }
}

int main() {
    // Read assembly code from a file
    FILE *assembly_file = fopen("assembly.asm", "r");
    if (assembly_file == NULL) {
        perror("Error opening assembly.asm");
        return 1;
    }

    fseek(assembly_file, 0, SEEK_END);
    long file_size = ftell(assembly_file);
    fseek(assembly_file, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        perror("Error allocating memory for file content");
        fclose(assembly_file);
        return 1;
    }

    fread(file_content, 1, file_size, assembly_file);
    file_content[file_size] = '\0';
    fclose(assembly_file);


    BinaryOutput bin_prog = assemble(file_content);

    printf("--- ASSEMBLY OUTPUT ---\n");
    for (int i = 0; i < bin_prog.count; i++) {
        printf("Line %d: ", i);
        print_binary16(bin_prog.instructions[i]);
        printf("  (Hex: %04X)\n", bin_prog.instructions[i]);
    }   
    
    // Write to a .bin file
    FILE *f = fopen("output.bin", "w");
    if (f == NULL) {
        perror("Error opening file");
        return 1;
    }
    fwrite(bin_prog.instructions, sizeof(uint16_t), bin_prog.count, f);
    fclose(f);
    printf("\nBinary output written to output.bin\n");

    // Write to a .txt file with 1s and 0s
    FILE *ftxt = fopen("output.txt", "w");
    if (ftxt == NULL) {
        perror("Error opening text file");
        return 1;
    }
    for (int i = 0; i < bin_prog.count; i++) {
        for (int j = 15; j >= 0; j--) {
            fputc((bin_prog.instructions[i] & (1 << j)) ? '1' : '0', ftxt);
        }
        fputc('\n', ftxt);
    }
    fclose(ftxt);
    printf("Text output written to output.txt\n");



    free(bin_prog.instructions);

    return 0;
}

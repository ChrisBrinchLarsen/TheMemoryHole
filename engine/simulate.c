#include "simulate.h"
#include "mmu.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// Global variables
int R[32];                                            // Registers
uint32_t PC = 0;                                      // Program Counter
int8_t advancePC = 1;                                 // This flag controls if automatic PC advances should be enabled for a given iteration
int stepSize = 4;                                     // The default 32bit processor moves the PC by 4 each instruction
int terminateFlag = 0;                                // Flag for deciding if the program should terminate
int log_enabled = 0;                                  // Flag for enabling writing to a log file
FILE* log_file_global;                                // Globally accessible FILE* pointer to write log information to
const int A0 = 10;                                    // Alias for the a0 register
const int A7 = 17;                                    // Alias for the a7 register

// Function prototypes
void ExecuteInstruction(int OPCODE, int instruction, struct memory *mem); // Relays instruction execution to various functions
void wrReg(int registerID, int data);                 // Register setter such that x0 is never not zero.
void ProcessR(int instruction);                       // Register/Register instructions
void ProcessI_A(int instruction);                     // Arithmetic Immediate instructions
void ProcessI_L(int instruction, struct memory *mem); // Load instructions
void ProcessI_E(int instruction);                     // ECall
void ProcessI_J(int instruction);                     // jalr
void ProcessS(int instruction, struct memory *mem);   // Store instructions
void ProcessB(int instruction);                       // Branch instructions
void ProcessJ(int instruction);                       // Jump and Link
void ProcessU_L(int instruction);                     // Load Upper Immediate
void ProcessU_A(int instruction);                     // Add unsigned immediate to PC (auipc)
void PrintRegisters();                                // Function that prints register overview for debugging
void wrInstToLog(struct assembly *as, int jump);      // Writes information about the current instruction to a log file

FILE* CACHE_LOG_POINTER;

// Simulates provided RISC-V assembly instructions.
long int simulate(struct memory *mem, struct assembly *as, int start_addr, FILE *log_file, struct hashmap *map) {
    CACHE_LOG_POINTER = get_cache_log();
    fprintf(CACHE_LOG_POINTER, "---- PROGRAM START ----\n");

    PC = start_addr;                                  // Initializing PC
    uint32_t prevPC = PC-4;                           // Used for keeping track of if we jumped or not
    int instructionCount = 0;                         // Instruction count for return value
    log_enabled = (log_file != NULL);                 // Only log information to file if file was supplied
    log_file_global = log_file;                       // Making the log file visible everywhere

    // Main Instruction Loop
    while (1) {
        if (log_enabled) {
            int isJump = prevPC+4 != PC;
            wrInstToLog(as, isJump);
        }

        // Read next instruction
        fprintf(CACHE_LOG_POINTER, "fetch: ");
        int instructionInt = mmu_rd_instr(mem, PC);
        fprintf(CACHE_LOG_POINTER, "endfetch\n");

        fprintf(CACHE_LOG_POINTER, "instr:\n");
        // Least significant 6 bits of instruction make up the OPCODE
        uint32_t OPCODE = instructionInt & 0x7F; 
        ExecuteInstruction(OPCODE, instructionInt, mem); // Perform current instruction

        const ProgramLineMap_t *plm = hashmap_get(map, &(ProgramLineMap_t){.pc=PC});
        if (plm != NULL) {
            fprintf(CACHE_LOG_POINTER, "pc %d %d %d\n", plm->pc, plm->start, plm->end);
        }
        
        fprintf(CACHE_LOG_POINTER, "CS %u\n", mmu_get_checksum());
        fprintf(CACHE_LOG_POINTER, "endinstr\n");
        

        // Increment PC and instruction count
        prevPC = PC;
        PC += advancePC ? stepSize : 0;
        instructionCount++;
        advancePC = 1;
        
        if (log_enabled) {
            fprintf(log_file, "\n"); // End each instruction log with a newline
        }
        // Terminate if flag was turned on during instruction
        if (terminateFlag) {break;}
    }

    ////// DEBUGGING ////// 
    // PrintRegisters(); // Uncomment this line to see contents of registers at end of execution.
    ////// DEBUGGING //////

    return instructionCount;
}

// We always use this setter to write to registers so that we don't overwrite x0
void wrReg(int registerID, int data) {
    if (registerID == 0) {return;}
    fprintf(CACHE_LOG_POINTER, "w %d %d\n", registerID, data);
    R[registerID] = data;
}

int rdReg(int registerID) {
    fprintf(CACHE_LOG_POINTER, "r %d\n", registerID);
    return R[registerID];
}

// This function takes an OPCODE and a full instruction integer and delegates to
// more specific functions that handle each type of OPCODE instruction.
void ExecuteInstruction(int OPCODE, int instruction, struct memory *mem) {
    switch (OPCODE) {
        case 0x33: // R
            ProcessR(instruction);
            break;
        case 0x13: // I (Immediate arithmetics)
            ProcessI_A(instruction);
            break;
        case 0x3: // I-2 (Loads)
            ProcessI_L(instruction, mem);
            break;
        case 0x73: // I-3 (ecall)
            ProcessI_E(instruction);
            break;
        case 0x67: // I-4 (jalr)
            ProcessI_J(instruction);
            break;
        case 0x23: // S
            ProcessS(instruction, mem);
            break;
        case 0x63: // B
            ProcessB(instruction);
            break;
        case 0x6F: // J
            ProcessJ(instruction);
            break;
        case 0x37: // U
            ProcessU_L(instruction);
            break;
        case 0x17: // U-2
            ProcessU_A(instruction);
            break;
        default:
            printf("OPCODE didn't match any expected types\n");
            dump_memory(instruction);
            exit(-1);
    }
    return;
}

// Proccesses R instructions.
void ProcessR(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 =  (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;

    switch (funct3) {
    case 0x0: // Add/Sub/Mul
        if (funct7 == 0x20) { // Sub
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d - %d = %d)", rd, R[rs1], R[rs2], R[rs1] - R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "SUB %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) - rdReg(rs2));
        } else if (funct7 == 0x00) { // Add
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d + %d = %d)", rd, R[rs1], R[rs2], R[rs1] + R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "ADD %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) + rdReg(rs2));
        } else if (funct7 == 0x01) { // Mul
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d * %d = %d)", rd, R[rs1], R[rs2], R[rs1] * R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "MUL %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) * rdReg(rs2));
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x4: // XOR or Div
        if (funct7 == 0x00) { // XOR
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d ^ %d = %d)", rd, R[rs1], R[rs2], R[rs1] ^ R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "XOR %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) ^ rdReg(rs2));
        } else if (funct7 == 0x01) { // Div
            fprintf(CACHE_LOG_POINTER, "DIV %d, %d, %d\n", rd, rs1, rs2);
            rdReg(rs1);
            rdReg(rs2);
            if (R[rs2] == 0) { // Division by zero
                wrReg(rd, -1);
            } else if (R[rs1] == (int)pow(-2, 31) && R[rs2] == -1) { // Signed overflow
                wrReg(rd, (int)pow(-2, 31));
            } else {
                if (log_enabled) {
                    fprintf(log_file_global, "R[%d] <- (%d / %d = %d)", rd, R[rs1], R[rs2], R[rs1] / R[rs2]);
                }
                wrReg(rd, R[rs1] / R[rs2]);
            }
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x6: // OR and Remainder
        if (funct7 == 0x00) { // OR
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d | %d = %d)", rd, R[rs1], R[rs2], R[rs1] | R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "OR %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) | rdReg(rs2));
        } else if (funct7 == 0x01) { // Remainder
            fprintf(CACHE_LOG_POINTER, "REM %d, %d, %d\n", rd, rs1, rs2);
            rdReg(rs1);
            rdReg(rs2);
            if (R[rs2] == 0) { // Division by zero
                wrReg(rd, R[rs1]);
            } else if (R[rs1] == (int)pow(-2, 31) && R[rs2] == -1) { // Signed overflow
                wrReg(rd, 0);
            } else {
                if (log_enabled) {
                    fprintf(log_file_global, "R[%d] <- (%d mod %d = %d)", rd, R[rs1], R[rs2], R[rs1] % R[rs2]);
                }
                wrReg(rd, R[rs1] % R[rs2]);
            }
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x7: // AND and Remainder (U)
        if (funct7 == 0x00) { // AND
            fprintf(CACHE_LOG_POINTER, "ANDU %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) & rdReg(rs2));
        } else if (funct7 == 0x01) { // Remainder (U)
            fprintf(CACHE_LOG_POINTER, "REMU %d, %d, %d\n", rd, rs1, rs2);
            rdReg(rs1);
            rdReg(rs2);
            if (R[rs2] == 0) { // Division by zero
                wrReg(rd, R[rs1]);
            } else {
                unsigned int u1 = (unsigned int)R[rs1];
                unsigned int u2 = (unsigned int)R[rs2];
                if (log_enabled) {
                    fprintf(log_file_global, "R[%d] <- (%u mod %u = %d)", rd, u1, u2, u1 % u2);
                }
                wrReg(rd, u1 % u2);
            }
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x1: // Shift-L Logical
        if (funct7 == 0x00) {
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d << %d = %d)", rd, R[rs1], R[rs2], R[rs1] << R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "SLL %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) << rdReg(rs2));
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x5: // Shift-R Logical/Arithmetic and div (U)
        if (funct7 == 0x20) {
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d >> %d = %d)", rd, R[rs1], R[rs2], R[rs1] >> R[rs2]);
            }
            fprintf(CACHE_LOG_POINTER, "SRA %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, rdReg(rs1) >> rdReg(rs2)); // Arithmetic
        } else if (funct7 == 0x00) {
            unsigned int u1 = (unsigned int)rdReg(rs1);
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d >> %d = %d)", rd, u1, R[rs2], (int)((u1) >> R[rs2]));
            }
            fprintf(CACHE_LOG_POINTER, "SRL %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, (int)((u1) >> rdReg(rs2))); // Logical
        } else if (funct7 == 0x01) { // Div (U)
            fprintf(CACHE_LOG_POINTER, "DIVU %d, %d, %d\n", rd, rs1, rs2);
            rdReg(rs1);
            rdReg(rs2);
            if (R[rs2] == 0) { // Division by zero
                wrReg(rd, UINT32_MAX);
            } else {
                unsigned int u1 = (unsigned int)R[rs1];
                unsigned int u2 = (unsigned int)R[rs2];
                if (log_enabled) {
                    fprintf(log_file_global, "R[%d] <- (%d / %d = %d)", rd, u1, u2, u1 / u2);
                }
                wrReg(rd, u1 / u2); 
            }
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x2: // Set if LT
        if (funct7 == 0x00) {
            fprintf(CACHE_LOG_POINTER, "SLT %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, (rdReg(rs1) < rdReg(rs2))?1:0);
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    case 0x3: // Set if LT (U)
        if (funct7 == 0x00) {
            unsigned int u1 = (unsigned int)rdReg(rs1);
            unsigned int u2 = (unsigned int)rdReg(rs2);
            fprintf(CACHE_LOG_POINTER, "SLTU %d, %d, %d\n", rd, rs1, rs2);
            wrReg(rd, (u1 < u2)?1:0);
        } else {
            printf("Unexpected funct7 value.\n");
        }
        break;
    default:
        printf("Unexpected funct3\n");
        break;
    }
}

// Proccesses arithmetic I instructions.
void ProcessI_A(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    int imm = instruction & 0xFFF00000;
    imm = imm >> 20;
    uint32_t funct7 = (imm >> 5) & 0x7F;
    uint32_t shiftValue = imm & 0x1F;

    switch (funct3) {
    case 0x0:; // ADD Immediate // The semi colon on this line supresses a warning
        int rs1Val = R[rs1];
        fprintf(CACHE_LOG_POINTER, "ADDI %d, %d, %d\n", rd, rs1, imm);
        wrReg(rd, rdReg(rs1) + imm);
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%d + %d = %d)", rd, rs1Val, imm, rs1Val + imm);
        }
        break;
    case 0x4: // XOR Immediate
        fprintf(CACHE_LOG_POINTER, "XORI %d, %d, %d\n", rd, rs1, imm);
        wrReg(rd, rdReg(rs1) ^ imm);
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%d ^ %d = %d)", rd, R[rs1], imm, R[rs1] ^ imm);
        }
        break;
    case 0x6: // OR Immediate
        fprintf(CACHE_LOG_POINTER, "ORI %d, %d, %d\n", rd, rs1, imm);
        wrReg(rd, rdReg(rs1) | imm);
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%d | %d = %d)", rd, R[rs1], imm, R[rs1] | imm);
        }
        break;
    case 0x7: // AND Immediate
        fprintf(CACHE_LOG_POINTER, "ANDI %d, %d, %d\n", rd, rs1, imm);
        wrReg(rd, rdReg(rs1) & imm);
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%d & %d = %d)", rd, R[rs1], imm, R[rs1] & imm);
        }
        break;
    case 0x1: // Shift Left Logical Immediate
        if (funct7 == 0x00) {
            fprintf(CACHE_LOG_POINTER, "SLLI %d, %d, %d\n", rd, rs1, shiftValue);
            unsigned int u1 = (unsigned int)rdReg(rs1);
            wrReg(rd, (int)((u1) << shiftValue));
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%u << %d = %d)", rd, u1, shiftValue, R[rd]);
            }
        } else {
            printf("Unexpected funct7 value");
        }
        break;
    case 0x5: // Shift Right Logical/Arithmetic Immediate
        if (funct7 == 0x00) { // Logical
            unsigned int u1 = (unsigned int)rdReg(rs1);
            fprintf(CACHE_LOG_POINTER, "SRLI %d, %d, %d\n", rd, rs1, shiftValue);
            wrReg(rd, (int)((u1) >> shiftValue));
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%u >> %d = %d)", rd, u1, shiftValue, R[rd]);
            }
        } else if (funct7 == 0x20) { // Arithmetic
            fprintf(CACHE_LOG_POINTER, "SRAI %d, %d, %d\n", rd, rs1, shiftValue);
            wrReg(rd, rdReg(rs1) >> shiftValue);
            if (log_enabled) {
                fprintf(log_file_global, "R[%d] <- (%d >> %d = %d)", rd, R[rs1], shiftValue, R[rd]);
            }
        } else {
            printf("Unexpected funct7 value");
        }
        break;
    case 0x2: // Set Less Than Immediate
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%d < %d = %d)", rd, R[rs1], imm, (R[rs1] < imm)?1:0);
        }
        fprintf(CACHE_LOG_POINTER, "SLTI %d, %d, %d\n", rd, rs1, imm);
        wrReg(rd, (rdReg(rs1) < imm)?1:0);
        
        break;
    case 0x3:; // Set Less Than Immediate (U) // The semi-colon on this line supresses an error
        unsigned int u1 = (unsigned int)rdReg(rs1);
        unsigned int uImm = (unsigned int)imm;
        if (log_enabled) {
            fprintf(log_file_global, "R[%d] <- (%u < %u = %d)", rd, u1, uImm, (u1 < uImm)?1:0);
        }
        fprintf(CACHE_LOG_POINTER, "SLTIU %d, %d, %d\n", rd, rs1, uImm);
        wrReg(rd, (u1 < uImm)?1:0);
        break;        
    default:
        break;
    }
}

// Proccesses load I instructions.
void ProcessI_L(int instruction, struct memory *mem) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    int imm = (instruction & 0xFFF00000) >> 20;

    int valueToSave = 0;
    int valueToSaveSE = 0;
    int log_offset = 0;

    switch (funct3) {
        case 0x0: // Load Byte
            valueToSave = mmu_rd_b(mem, rdReg(rs1) + imm);
            valueToSaveSE = (valueToSave << 24) >> 24;
            if (log_enabled) {
                fprintf(log_file_global, "Load %d into R[%d] from %x", valueToSaveSE, rd, R[rs1] + imm);
            }
            fprintf(CACHE_LOG_POINTER, "LB %d, %d(%d)\n", rd, imm, rs1);
            wrReg(rd, valueToSaveSE);
            break;
        case 0x1: // Load Half
            valueToSave = mmu_rd_h(mem, rdReg(rs1) + imm);
            valueToSaveSE = (valueToSave << 16) >> 16;
            if (log_enabled) {
                fprintf(log_file_global, "Load %d into R[%d] from %x", valueToSaveSE, rd, R[rs1] + imm);
            }
            fprintf(CACHE_LOG_POINTER, "LH %d, %d(%d)\n", rd, imm, rs1);
            wrReg(rd, valueToSaveSE);
            break;
        case 0x2: // Load Word
            log_offset = R[rs1] + imm;
            fprintf(CACHE_LOG_POINTER, "LW %d, %d(%d)\n", rd, imm, rs1);
            wrReg(rd, mmu_rd_w(mem, rdReg(rs1) + imm));
            if (log_enabled) {
                fprintf(log_file_global, "Load %d into R[%d] from %x", R[rd], rd, log_offset);
            }
            break;
        case 0x4: // Load Byte (U)
            log_offset = R[rs1] + imm;
            fprintf(CACHE_LOG_POINTER, "LBU %d, %d(%d)\n", rd, imm, rs1);
            wrReg(rd, mmu_rd_b(mem, rdReg(rs1) + imm));
            if (log_enabled) {
                fprintf(log_file_global, "Load %d into R[%d] from %x", R[rd], rd, log_offset);
            }
            break;
        case 0x5: // Load Half (U)
            log_offset = R[rs1] + imm;
            fprintf(CACHE_LOG_POINTER, "LHU %d, %d(%d)\n", rd, imm, rs1);
            wrReg(rd, mmu_rd_h(mem, rdReg(rs1) + imm));
            if (log_enabled) {
                fprintf(log_file_global, "Load %d into R[%d] from %x", R[rd], rd, log_offset);
            }
            break;    
        default:
            break;
    }
}

// Proccesses ecall I instructions.
void ProcessI_E(int instruction) {
    // Parsing instruction
    int imm = instruction & 0xFFF00000;
    imm = imm >> 20;


    fprintf(CACHE_LOG_POINTER, "ecall\n");
    int callType = rdReg(A7); // A7

    switch (callType) {
        case 1: // return getchar() in A0
            wrReg(A0, getchar());
            if (log_enabled) {
                fprintf(log_file_global, "Got '%c' from user", R[A0]);
            }
            break;
        case 2: // execute putchar(c), where c is taken from A0
            if (log_enabled) {
                fprintf(log_file_global, "Wrote '%c' to user", R[A0]);
            }
            putchar(rdReg(A0));
            fprintf(CACHE_LOG_POINTER, "stdout %d\n", R[A0]);
            break;
        case 3: // terminate simulation
        case 93:
            terminateFlag = 1;
            if (log_enabled) {
                fprintf(log_file_global, "Terminating program");
            }
            break;
        default:
            break;
    }
}

// Proccesses I instructions.
void ProcessI_J(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    int imm = instruction & 0xFFF00000;
    imm = imm >> 20;

    if (funct3 == 0x0) { // Jump And Link Register
        fprintf(CACHE_LOG_POINTER, "JALR %d, %d(%d)\n", rd, imm, rs1);
        int offset = rdReg(rs1);
        if (log_enabled) {
            fprintf(log_file_global, "Jump %x -> %x", PC, offset + imm);
        }
        wrReg(rd, PC+4);
        PC = offset + imm;
        advancePC = 0;
    } else {
        printf("Unexpected funct3 value\n");
    }
}

// Proccesses S instructions.
void ProcessS(int instruction, struct memory *mem) {
    // Parsing instruction
    int immUpper = (instruction & 0xFE000000);
    immUpper = immUpper >> 20; // Sign extending
    int immLower = ((instruction >> 7) & 0x1F);
    int imm = immUpper | immLower;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;

    switch (funct3) {
    case 0x0: // Store Byte
        fprintf(CACHE_LOG_POINTER, "SB %d, %d(%d)\n", rs2, imm, rs1);
        mmu_wr_b(mem, rdReg(rs1) + imm, rdReg(rs2));
        if (log_enabled) {
            fprintf(log_file_global, "(%x+%x=%x) <- %d", imm, R[rs1], R[rs1] + imm, R[rs2]);
        } 
        break;
    case 0x1: // Store Half
        if (log_enabled) {
            fprintf(log_file_global, "%x <- %d", R[rs1] + imm, R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "SH %d, %d(%d)\n", rs2, imm, rs1);
        mmu_wr_h(mem, rdReg(rs1) + imm, rdReg(rs2));
        break;
    case 0x2: // Store Word
        if (log_enabled) {
            fprintf(log_file_global, "%x <- %d", R[rs1] + imm, R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "SW %d, %d(%d)\n", rs2, imm, rs1);
        mmu_wr_w(mem, rdReg(rs1) + imm, rdReg(rs2));
        break;
    default:
        break;
    }
}

// Proccesses B instructions.
void ProcessB(int instruction) {
    // Parsing instruction
    uint32_t rs1 =  (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    int upper = ((instruction >> 31) & 0x1) << 12;
    int upperMid = ((instruction >> 7) & 0x1) << 11;
    int lowerMid = ((instruction >> 25) & 0x3F) << 5;
    int lower = ((instruction >> 8) & 0xF) << 1;
    int imm = ((upper | upperMid | lowerMid | lower) << 19) >> 19; // Sign extending 

    unsigned int u1;
    unsigned int u2;

    switch (funct3) {
    case 0x0: // Branch ==
        if (log_enabled) {
            fprintf(log_file_global, "%d == %d = %d", R[rs1], R[rs2], R[rs1] == R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "BEQ %d, %d, %d\n", rs1, rs2, imm);
        if (rdReg(rs1) == rdReg(rs2)) {PC += imm; advancePC = 0;}
        break;
    case 0x1: // Branch !=
        if (log_enabled) {
            fprintf(log_file_global, "%d != %d = %d", R[rs1], R[rs2], R[rs1] != R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "BNE %d, %d, %d\n", rs1, rs2, imm);
        if (rdReg(rs1) != rdReg(rs2)) {PC += imm; advancePC = 0;}
        break;
    case 0x4: // Branch <
        if (log_enabled) {
            fprintf(log_file_global, "%d < %d = %d", R[rs1], R[rs2], R[rs1] < R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "BLT %d, %d, %d\n", rs1, rs2, imm);
        if (rdReg(rs1) < rdReg(rs2)) {PC += imm; advancePC = 0;}
        break;
    case 0x5: // Branch >=
        if (log_enabled) {
            fprintf(log_file_global, "%d >= %d = %d", R[rs1], R[rs2], R[rs1] >= R[rs2]);
        }
        fprintf(CACHE_LOG_POINTER, "BGE %d, %d, %d\n", rs1, rs2, imm);
        if (rdReg(rs1) >= rdReg(rs2)) {PC += imm; advancePC = 0;}
        break;
    case 0x6: // Branch < (U)
        fprintf(CACHE_LOG_POINTER, "BLTU %d, %d, %d\n", rs1, rs2, imm);
        u1 = (unsigned int)rdReg(rs1);
        u2 = (unsigned int)rdReg(rs2);
        if (log_enabled) {
            fprintf(log_file_global, "%u < %u = %d", u1, u2, u1 < u2);
        }
        if (u1 < u2) {PC += imm; advancePC = 0;}
        break;
    case 0x7: // Branch >= (U)
        fprintf(CACHE_LOG_POINTER, "BGEU %d, %d, %d\n", rs1, rs2, imm);
        u1 = (unsigned int)rdReg(rs1);
        u2 = (unsigned int)rdReg(rs2);
        if (log_enabled) {
            fprintf(log_file_global, "%u >= %u = %d", u1, u2, u1 >= u2);
        }
        if (u1 >= u2) {PC += imm; advancePC = 0;}
        break;
    default:
        break;
    }
}

// Proccesses J instructions.
void ProcessJ(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    int upper = ((instruction >> 31) & 0x1) << 20;
    int upperMid = ((instruction >> 12) & 0xFF) << 12;
    int lowerMid = ((instruction >> 20) & 0x1) << 11;
    int lower = ((instruction >> 21) & 0x3FF) << 1;
    int imm = ((upper | upperMid | lowerMid | lower) << 11) >> 11; // Sign extending

    // Jump and Link
    if (log_enabled) {
        fprintf(log_file_global, "Jump %x -> %x", PC, PC + imm);
    }
    PC += imm;
    advancePC = 0;
    
    fprintf(CACHE_LOG_POINTER, "JAL %d, %d\n", rd, imm);
    wrReg(rd, PC);
}

// Proccesses U Load instructions.
void ProcessU_L(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    int imm = instruction & 0xFFFFF000;
    
    // Load Upper Imm
    fprintf(CACHE_LOG_POINTER, "LUI %d, %d\n", rd, imm);
    wrReg(rd, imm);
    if (log_enabled) {
        fprintf(log_file_global, "R[%d] <- %x", rd, R[rd]);
    }
}

// Proccesses U Add instructions
void ProcessU_A(int instruction) {
    // Parsing instruction
    uint32_t rd = (instruction >> 7) & 0x1F;
    int imm = instruction & 0xFFFFF000;

    // Write Upper Imm + PC to register
    fprintf(CACHE_LOG_POINTER, "AUIPC %d, %d\n", rd, imm);
    wrReg(rd, PC + imm);
    if (log_enabled) {
        fprintf(log_file_global, "R[%d] <- %x", rd, R[rd]);
    }
}

void PrintRegisters() {
    printf("| zero = %d | ra = %d | sp = %d | gp = %d | tp = %d | t0 = %d | t1 = %d | t2 = %d |\n"
           "| s0 = %d | s1 = %d | a0 = %d | a1 = %d | a2 = %d | a3 = %d | a4 = %d | a5 = %d |\n"
           "| a6 = %d | a7 = %d | s2 = %d | s3 = %d | s4 = %d | s5 = %d | s6 = %d | s7 = %d |\n"
           "| s8 = %d | s9 = %d | s10 = %d | s11 = %d | t3 = %d | t4 = %d | t5 = %d | t6 = %d |\n",
            R[0], R[1], R[2], R[3], R[4], R[5], R[6], R[7], R[8], R[9], R[10], R[11], R[12], R[13], R[14],
            R[15], R[16], R[17], R[18], R[19], R[20], R[21], R[22], R[23], R[24], R[25], R[26], R[27], R[28], R[29],
            R[30], R[31]);
}

void wrInstToLog(struct assembly *as, int jump) {
    int padding = 60;
    if (jump) {
        padding -= fprintf(log_file_global, "%x  => %s", PC, assembly_get(as, PC));
    } else {
        padding -= fprintf(log_file_global, "%x  -  %s", PC, assembly_get(as, PC));
    }
    fprintf(log_file_global, "%*s", padding, "");
}


// for the hashmap
int programLineMap_compare(const void *a, const void *b, void*) {
    const ProgramLineMap_t *ua = a;
    const ProgramLineMap_t *ub = b;
    return ua->pc - ub->pc;
}
bool programLineMap_iter(const void *item, void*) {
    // TODO: i have no idea what this function is meant for or if we need it tbh
    const ProgramLineMap_t *plm = item;
    printf("pc %d, start: %d, end: %d\n", plm->pc, plm->start, plm->end);
    return true;
}
uint64_t programLineMap_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const ProgramLineMap_t *plm = item;
    return hashmap_sip(&plm->pc, sizeof(int), seed0, seed1);
}
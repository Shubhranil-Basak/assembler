#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class VirtualMachine
{
private:
    static const int NUM_REGISTERS = 32;
    static const uint32_t MEMORY_SIZE = 0x00100000; // 1MB total memory
    static const uint32_t DATA_SECTION_START = 0x10000000;

    vector<uint32_t> memory;
    uint32_t registers[NUM_REGISTERS];
    uint32_t pc; // Program Counter
    bool running;

    // Helper functions to decode instructions
    uint32_t getOpcode(uint32_t instruction)
    {
        return (instruction >> 26) & 0x3F;
    }

    uint32_t getRd(uint32_t instruction)
    {
        return (instruction >> 21) & 0x1F;
    }

    uint32_t getRs(uint32_t instruction)
    {
        return (instruction >> 16) & 0x1F;
    }

    uint32_t getRt(uint32_t instruction)
    {
        return (instruction >> 11) & 0x1F;
    }

    int16_t getImmediate(uint32_t instruction)
    {
        return instruction & 0xFFFF;
    }

    uint32_t getJumpAddress(uint32_t instruction)
    {
        return instruction & 0x3FFFFFF;
    }

    // Memory access functions with address translation
    uint32_t readMemory(uint32_t address)
    {
        if (address >= DATA_SECTION_START)
        {
            address = (address - DATA_SECTION_START) + (MEMORY_SIZE / 2);
        }

        if (address >= MEMORY_SIZE * 4)
        {
            throw runtime_error("Memory access out of bounds: 0x" + to_string(address));
        }

        return memory[address / 4];
    }

    void writeMemory(uint32_t address, uint32_t value)
    {
        if (address >= DATA_SECTION_START)
        {
            address = (address - DATA_SECTION_START) + (MEMORY_SIZE / 2);
        }

        if (address >= MEMORY_SIZE * 4)
        {
            throw runtime_error("Memory access out of bounds: 0x" + to_string(address));
        }

        memory[address / 4] = value;
    }

    void executeRType(uint32_t instruction)
    {
        uint32_t rd = getRd(instruction);
        uint32_t rs = getRs(instruction);
        uint32_t rt = getRt(instruction);

        switch (getOpcode(instruction))
        {
        case 0: // ADD
            registers[rd] = registers[rs] + registers[rt];
            break;
        case 1: // SUB
            registers[rd] = registers[rs] - registers[rt];
            break;
        case 2: // MUL
            registers[rd] = registers[rs] * registers[rt];
            break;
        case 3: // DIV
            if (registers[rt] == 0)
                throw runtime_error("Division by zero");
            registers[rd] = registers[rs] / registers[rt];
            break;
        case 4: // INV
            registers[rd] = ~registers[rs];
            break;
        case 5: // AND
            registers[rd] = registers[rs] & registers[rt];
            break;
        case 6: // NAND
            registers[rd] = ~(registers[rs] & registers[rt]);
            break;
        case 7: // OR
            registers[rd] = registers[rs] | registers[rt];
            break;
        case 8: // XOR
            registers[rd] = registers[rs] ^ registers[rt];
            break;
        case 9: // XNOR
            registers[rd] = ~(registers[rs] ^ registers[rt]);
            break;
        case 10: // MOV
            registers[rd] = registers[rs];
            break;
        case 11: // SGT
            registers[rd] = (registers[rs] > registers[rt]) ? 1 : 0;
            break;
        case 12: // SLT
            registers[rd] = (registers[rs] < registers[rt]) ? 1 : 0;
            break;
        case 13: // MA
            registers[rd] = registers[rs] * registers[rt] + registers[rd];
            break;
        }
    }

    void executeIType(uint32_t instruction)
    {
        uint32_t rd = getRd(instruction);
        uint32_t rs = getRs(instruction);
        int16_t imm = getImmediate(instruction);

        switch (getOpcode(instruction))
        {
        case 14: // ADDI
            registers[rd] = registers[rs] + imm;
            break;
        case 15: // SUBI
            registers[rd] = registers[rs] - imm;
            break;
        case 16: // MULI
            registers[rd] = registers[rs] * imm;
            break;
        case 17: // DIVI
            if (imm == 0)
                throw runtime_error("Division by zero");
            registers[rd] = registers[rs] / imm;
            break;
        case 18: // ANDI
            registers[rd] = registers[rs] & imm;
            break;
        case 19: // NANDI
            registers[rd] = ~(registers[rs] & imm);
            break;
        case 20: // ORI
            registers[rd] = registers[rs] | imm;
            break;
        case 21: // XORI
            registers[rd] = registers[rs] ^ imm;
            break;
        case 22: // LW
            registers[rd] = readMemory(registers[rs] + imm);
            break;
        case 23: // SW
            writeMemory(registers[rs] + imm, registers[rd]);
            break;
        case 24: // LB
            registers[rd] = readMemory(registers[rs] + imm) & 0xFF;
            break;
        case 25: // SB
            writeMemory(registers[rs] + imm, registers[rd] & 0xFF);
            break;
        case 26: // BEQ
            if (registers[rd] == registers[rs])
            {
                pc += ((imm - 4) * 4);
            }
            break;
        case 27: // BNE
            if (registers[rd] != registers[rs])
            {
                pc += (imm * 4);
            }
            break;
        case 28: // RESET
            registers[rd] = 0;
            break;
        case 31: // EXT
            running = false;
            break;
        }
    }

    void executeJType(uint32_t instruction)
    {
        uint32_t address = getJumpAddress(instruction);

        switch (getOpcode(instruction))
        {
        case 29:              // JUMP
            pc = address * 4; // Convert word address to byte address
            pc -= 8;          // Compensate for pc increment after instruction
            break;
        case 30:                    // JAL
            registers[31] = pc + 4; // Store return address in R31
            pc = address * 4;
            pc -= 4;
            break;
        }
    }

    void printInstruction(uint32_t instruction)
    {
        uint32_t opcode = getOpcode(instruction);
        cout << "PC: 0x" << hex << pc << " Instruction: 0x" << instruction;
        if (opcode <= 13)
        {
            cout << " RD: R" << dec << getRd(instruction)
                 << " RS: R" << getRs(instruction)
                 << " RT: R" << getRt(instruction);
        }
        else if(opcode <= 28 || opcode == 31)
        {
            cout << " RD: R" << dec << getRd(instruction)
                 << " RS: R" << getRs(instruction)
                 << " IMM: " << getImmediate(instruction);
        }
        else if(opcode <= 30)
        {
            cout << " ADDR: 0x" << hex << getJumpAddress(instruction);
        }
    }

public:
    VirtualMachine() : memory(MEMORY_SIZE / 4), pc(0), running(false)
    {
        for (int i = 0; i < NUM_REGISTERS; i++)
        {
            registers[i] = 0;
        }
    }
    
};
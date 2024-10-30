#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

enum InstructionType
{
    R_TYPE,
    I_TYPE,
    J_TYPE
};

struct Instruction
{
    string name;
    InstructionType type;
    int opcode;
};

enum SectionType
{
    SECTION_TEXT,
    SECTION_DATA,
    SECTION_BSS
};

struct Section
{
    string name;
    SectionType type;
    uint32_t address;
    vector<uint32_t> content;
};

map<string, Instruction> instructions = {

    //R-type format
    {"ADD", {"ADD", R_TYPE, 0}},
    {"SUB", {"SUB", R_TYPE, 1}},
    {"MUL", {"MUL", R_TYPE, 2}},
    {"DIV", {"DIV", R_TYPE, 3}},
    {"INV", {"INV", R_TYPE, 4}},
    {"AND", {"AND", R_TYPE, 5}},
    {"NAND", {"NAND", R_TYPE, 6}},
    {"OR", {"OR", R_TYPE, 7}},
    {"XOR", {"XOR", R_TYPE, 8}},
    {"XNOR", {"XNOR", R_TYPE, 9}},
    {"MOV", {"MOV", R_TYPE, 10}},
    {"SGT", {"SGT", R_TYPE, 11}},
    {"SLT", {"SLT", R_TYPE, 12}},
    {"MA", {"MA", R_TYPE, 13}},

    // I-type format
    {"ADDI", {"ADDI", I_TYPE, 14}},
    {"SUBI", {"SUBI", I_TYPE, 15}},
    {"MULI", {"MULI", I_TYPE, 16}},
    {"DIVI", {"DIVI", I_TYPE, 17}},
    {"INVI", {"INVI", I_TYPE, 18}},
    {"ANDI", {"ANDI", I_TYPE, 19}},
    {"NANDI", {"NANDI", I_TYPE, 20}},
    {"ORI", {"ORI", I_TYPE, 21}},
    {"XORI", {"XORI", I_TYPE, 22}},
    {"XNORI", {"XNORI", I_TYPE, 23}},
    {"MOVI", {"MOVI", I_TYPE, 24}},
    {"SGTI", {"SGTI", I_TYPE, 25}},
    {"SLTI", {"SLTI", I_TYPE, 26}},
    {"MAI", {"MAI", I_TYPE, 27}},
    {"EXT", {"EXT", I_TYPE, 31}},

    //J-type format 
    {"JUMP", {"JUMP", J_TYPE, 29}},
    {"JAL", {"JAL", J_TYPE, 30}}};


class Assembler
{
private:
    int currentAddress;
    map<string, int> labels;
    vector<uint32_t> bytecode;

    map<string, Section> sections;
    Section *currentSection;
    uint32_t textBaseAddress = 0x00000000;
    uint32_t dataBaseAddress = 0x10000000;

    int parseRegister(const std::string &reg)
    {
        if (reg[0] != 'R' && reg[0] != 'r')
        {
            throw std::runtime_error("Invalid register format: " + reg);
        }
        return std::stoi(reg.substr(1));
    }

    int parseImmediate(const std::string &imm)
    {
        if (imm[0] == '#')
        {
            return std::stoi(imm.substr(1));
        }
        return std::stoi(imm);
    }

    uint32_t generateRType(const std::string &op, const std::vector<std::string> &operands)
    {
        uint32_t instruction = 0;
        instruction |= (static_cast<uint32_t>(instructions[op].opcode) << 26);

        if (operands.size() >= 3)
        {
            instruction |= (static_cast<uint32_t>(parseRegister(operands[0])) << 21); // rd
            instruction |= (static_cast<uint32_t>(parseRegister(operands[1])) << 16); // rs
            instruction |= (static_cast<uint32_t>(parseRegister(operands[2])) << 11); // rt
        }
        else if (operands.size() == 2)
        {
            instruction |= (static_cast<uint32_t>(parseRegister(operands[0])) << 21); // rd
            instruction |= (static_cast<uint32_t>(parseRegister(operands[1])) << 16); // rs
        }

        return instruction;
    }

    uint32_t generateIType(const std::string &op, const std::vector<std::string> &operands)
    {
        uint32_t instruction = 0;
        instruction |= (static_cast<uint32_t>(instructions[op].opcode) << 26);

        if (op == "EXT")
        {
            // EXT doesn't need operands
            return instruction;
        }

        // For normal I-type instructions
        instruction |= (static_cast<uint32_t>(parseRegister(operands[0])) << 21); // rd
        instruction |= (static_cast<uint32_t>(parseRegister(operands[1])) << 16); // rs

        // Handle immediate value or label
        int immediate;
        try
        {
            immediate = parseImmediate(operands[2]);
        }
        catch (...)
        {
            if (labels.find(operands[2]) != labels.end())
            {
                immediate = labels[operands[2]] - (currentAddress + 4);
            }
            else
            {
                throw std::runtime_error("Unknown label: " + operands[2]);
            }
        }

        instruction |= (immediate & 0xFFFF);
        return instruction;
    }

    uint32_t generateJType(const std::string &op, const std::vector<std::string> &operands)
    {
        uint32_t instruction = 0;
        instruction |= (static_cast<uint32_t>(instructions[op].opcode) << 26);

        int address;
        try
        {
            address = parseImmediate(operands[0]);
        }
        catch (...)
        {
            if (labels.find(operands[0]) != labels.end())
            {
                address = labels[operands[0]] / 4;
            }
            else
            {
                throw std::runtime_error("Unknown label: " + operands[0]);
            }
        }

        instruction |= (address & 0x3FFFFFF);
        return instruction;
    }

    void initializeSections()
    {
        sections[".text"] = {".text", SECTION_TEXT, textBaseAddress, {}};
        sections[".data"] = {".data", SECTION_DATA, dataBaseAddress, {}};
        currentSection = &sections[".text"]; // Default
    }

};
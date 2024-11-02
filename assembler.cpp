#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

using namespace std;

// Instruction type enum
enum InstructionType
{
    R_TYPE,
    I_TYPE,
    J_TYPE
};

// Instruction structure
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

    // R-type format
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

    // J-type format
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

    // Helper function to parse different parts of instructions
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

        // Normal I-type instructions
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

    void handleDirective(const std::string &directive, std::stringstream &ss)
    {
        if (directive == ".text" || directive == ".data")
        {
            currentSection = &sections[directive];
            currentAddress = currentSection->address;
        }
        else if (directive == ".word")
        {
            std::string value;
            while (ss >> value)
            {
                if (value.back() == ',')
                    value.pop_back();
                sections[".data"].content.push_back(std::stoi(value));
                currentAddress += 4;
            }
        }
    }

public:
    Assembler() : currentAddress(0) {}

    vector<uint32_t> assemble(const string &filename)
    {
        initializeSections();

        ifstream file(filename);
        if (!file.is_open())
        {
            throw runtime_error("Cannot open input file: " + filename);
        }

        vector<string> lines;
        string line;
        while (getline(file, line))
        {
            lines.push_back(line);
        }
        file.close();

        // First pass: collect labels
        firstPass(lines);

        // Setting everything to 0/default

        currentAddress = 0;
        bytecode.clear();

        // Second pass: generate bytecode
        for (const auto &line : lines)
        {
            if (line.empty() || line[0] == ';')
            {
                continue; // skip empty lines and comments
            }

            string processedLine = line;
            size_t commentPos = line.find(';');
            if (commentPos != string::npos)
            {
                processedLine = line.substr(0, commentPos);
            }

            // Handle labels
            size_t labelEnd = processedLine.find(':');
            if (labelEnd != string::npos)
            {
                processedLine = processedLine.substr(labelEnd + 1);
            }

            // Removing leading/trailing whitespace
            processedLine.erase(0, processedLine.find_first_not_of(" \t"));
            processedLine.erase(processedLine.find_last_not_of(" \t") + 1);

            if (processedLine.empty())
            {
                continue;
            }

            // Now to parse lines
            stringstream ss(processedLine);
            string firstToken;
            ss >> firstToken;

            if (firstToken[0] == '.')
            {
                // Handle section directives and data definitions
                handleDirective(firstToken, ss);
                continue; // Skip to next line after handling directive
            }

            // If not a directive, process as an instruction
            string op = firstToken;
            transform(op.begin(), op.end(), op.begin(), ::toupper);

            // Parse instruction operands
            vector<string> operands;
            string operand;

            while (ss >> operand)
            {
                if (operand.back() == ',')
                {
                    operand.pop_back();
                }
                operands.push_back(operand);
            }

            uint32_t instruction;
            if (op == "NOP")
            {
                instruction = 0;
            }
            else
            {
                auto it = instructions.find(op);
                if (it == instructions.end())
                {
                    throw runtime_error("Unknown instruction: " + op);
                }

                switch (it->second.type)
                {
                case R_TYPE:
                    instruction = generateRType(op, operands);
                    break;
                case I_TYPE:
                    instruction = generateIType(op, operands);
                    break;
                case J_TYPE:
                    instruction = generateJType(op, operands);
                    break;
                default:
                    throw runtime_error("Unknown instruction type");
                }
            }

            currentSection->content.push_back(instruction);
            currentAddress += 4;
        }

        return bytecode;
    }
};
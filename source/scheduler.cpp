#include "scheduler.h"
#include "instruction.h"

#include <filesystem>
#include <format>

using namespace scheduler;

Scheduler::Scheduler(std::filesystem::path units_config_path,
                     std::filesystem::path instr_config_path,
                     std::filesystem::path input_path,
                     std::filesystem::path output_path)
    : config_(units_config_path, instr_config_path) {

    try {
        std::ifstream input_file;
        input_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        input_file.open(input_path);

        std::stringstream input_file_contents;
        input_file_contents << input_file.rdbuf();
        input_file.close();

        std::vector<Instruction> instructions;
        for (std::string line; std::getline(input_file_contents, line);) {
            if (line.size() == 0)
                continue;

            instructions.push_back(Instruction::create(std::move(line), config_));
        }

        for (auto instr : instructions)
            instr.dump();

    } catch (const std::ifstream::failure &e) {
        throw std::runtime_error(std::format("Input file read error: {}", e.what()));
    }
}


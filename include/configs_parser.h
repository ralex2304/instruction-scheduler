#pragma once

#include "arguments_parser.h"
#include "types.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <toml++/impl/forward_declarations.hpp>
#include <toml++/impl/parse_error.hpp>
#include <toml++/toml.hpp>
#include <vector>

using namespace std::literals;

namespace scheduler {

struct UnitConfig {
    size_t quantity;
};

struct InstructionConfig {
    std::string name;
    ticks_t latency;

    std::vector<std::string> units;
    std::vector<ArgsVariant> arguments;

    bool operator==(const InstructionConfig& other) const {
        return arguments == other.arguments;
    }
};

struct Config {
    Config(std::filesystem::path units_path, std::filesystem::path instrs_path);

    const std::map<std::string, UnitConfig> units;
    const std::map<std::string, std::vector<InstructionConfig>> instructions;

    InstructionConfig start_instruction = {.name = "start", .latency = 0};
    InstructionConfig end_instruction   = {.name = "end",   .latency = 0} ;
};

} //< namespace scheduler


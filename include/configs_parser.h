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

namespace scheduler {

struct UnitConfig {
    size_t quantity;
};

struct InstructionConfig {
    std::string name;
    ticks_t latency;

    std::vector<std::map<std::string, UnitConfig>::const_iterator> units;
    std::vector<ArgsVariant> arguments;

    bool operator==(const InstructionConfig& other) const {
        return arguments == other.arguments;
    }
};

struct Config {
    Config(std::filesystem::path units_path, std::filesystem::path instrs_path);

    const std::map<std::string, UnitConfig> units;
    const std::map<std::string, std::vector<InstructionConfig>> instructions;
};

} //< namespace scheduler


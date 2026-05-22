#pragma once

#include "arguments_parser.h"
#include "types.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <toml++/impl/forward_declarations.hpp>
#include <toml++/impl/parse_error.hpp>
#include <toml++/toml.hpp>
#include <vector>

namespace scheduler {

struct UnitConfig {
    std::string name;
    size_t quantity;
};

enum class MemoryOrder {
    NON_MEMORY = 0,
    RELAXED,
    STRICT,
};

struct InstructionConfig {
    std::string name;
    ticks_t latency;

    MemoryOrder memory_order;

    std::vector<size_t> units_indexes;
    std::vector<ArgsVariant> arguments;

    bool args_equal(const InstructionConfig& other) const {
        return arguments == other.arguments;
    }
};

class Config {
public:
    Config(std::filesystem::path units_path, std::filesystem::path instrs_path);

    const InstructionConfig start_instruction = {
        .name = "start", .latency = 0, .memory_order = MemoryOrder::NON_MEMORY
    };
    const InstructionConfig end_instruction   = {
        .name = "end",   .latency = 0, .memory_order = MemoryOrder::NON_MEMORY
    };

    const auto& get_units() const { return units_; }
    const auto& get_instructions() const { return instructions_; }

private:
    std::vector<UnitConfig> units_;
    std::map<std::string, std::vector<InstructionConfig>> instructions_;

    std::map<std::string, size_t> read_units(std::filesystem::path path);

    void add_instruction_by_symbols(const toml::table& config, const InstructionConfig& instruction);
    void read_instructions(std::filesystem::path path, std::map<std::string, size_t> untis_map);
};

} //< namespace scheduler


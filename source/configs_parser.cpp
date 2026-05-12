#include "configs_parser.h"

#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <toml++/impl/forward_declarations.hpp>
#include <toml++/impl/parse_error.hpp>
#include <toml++/toml.hpp>
#include <vector>

using namespace scheduler;

namespace {

std::map<std::string, UnitConfig> read_units(std::filesystem::path path) {
    toml::table units_config = toml::parse_file(path.c_str());

    std::map<std::string, UnitConfig> units;

    units_config.for_each([&units](auto& key, auto& config) {
        if constexpr (toml::is_table<decltype(config)>) {
            auto quantity = config["quantity"].template value_exact<int64_t>();
            if (!quantity) {
                throw toml::parse_error("no or invalid \"quantity\" parameter", config.source());

            } else if (*quantity <= 0) {
                throw toml::parse_error(std::format(
                        "\"quantity\" parameter of unit \"{}\" must be > 0",
                        key.data()).c_str(), config.source());
            }

            units[std::string(key)] = {.quantity = static_cast<size_t>(*quantity)};
        } else {
            throw toml::parse_error("invalid unit declaration format",
                                    config.source());

        }
    });

    return units;
}

void read_instruction_units(InstructionConfig* instruction, const toml::table& config,
                            const std::map<std::string, UnitConfig>& units) {
    assert(instruction);

    auto instr_units = config.template get_as<toml::array>("units");
    if (!instr_units)
        return;

    instr_units->for_each([&instruction, units](auto& unit) {
        if constexpr (toml::is_string<decltype(unit)>) {
            auto it = units.find(*unit);
            if (it == units.end()) {
                throw toml::parse_error(std::format(
                        "unit \"{}\" was not declared", *unit).c_str(),
                        unit.source());
            }

            instruction->units.push_back(it);
        } else {
            throw toml::parse_error(
                    "\"units\" parameter must be list a of strings",
                    unit.source());
        }
    });
}

void read_instruction_arguments(InstructionConfig* instruction, const toml::table& config) {
    assert(instruction);

    auto arguments = config.template get_as<toml::array>("arguments");
    if (!arguments)
        return;

    bool found_dst = false;
    arguments->for_each([&instruction, &found_dst, arguments](auto& arg) {
        if constexpr (toml::is_string<decltype(arg)>) {
            if (*arg == "rd") {
                if (found_dst) {
                    throw toml::parse_error(std::format(
                        "instruction \"{}\" has multiple destinations", instruction->name).c_str(),
                        arguments->source());
                }
                found_dst = true;

                instruction->arguments.push_back(RegArg<true>());
            } else if (*arg == "rs") {
                instruction->arguments.push_back(RegArg<false>());
            } else if (*arg == "imm") {
                instruction->arguments.push_back(ImmArg());
            } else if (*arg == "mem_imm") {
                instruction->arguments.push_back(MemoryImmArg());
            } else if (*arg == "mem_rs_imm") {
                instruction->arguments.push_back(MemoryRegImmArg());
            } else {
                throw toml::parse_error(std::format(
                        "argument \"{}\" is invalid", *arg).c_str(),
                        arg.source());
            }
        } else {
            throw toml::parse_error(
                    "\"arguments\" parameter must be list a of strings",
                    arg.source());
        }
    });
}

void add_instruction_by_symbols(std::map<std::string, std::vector<InstructionConfig>>* instructions,
                                const toml::table& config, const InstructionConfig& instruction) {
    assert(instructions);

    auto symbols = config.template get_as<toml::array>("symbol");
    if (!symbols) {
        throw toml::parse_error("no or invalid \"symbol\" parameter",
                                config.source());
    }

    symbols->for_each([instruction, instructions](auto& symbol) {
        if constexpr (toml::is_string<decltype(symbol)>) {
            auto it = instructions->find(*symbol);
            if (it == instructions->end()) {
                it = instructions->try_emplace(*symbol).first;
            }

            for (auto existing_instr : it->second) {
                if (existing_instr == instruction) {
                    throw toml::parse_error(std::format(
                            "instruction \"{}\" is ambigious with \"{}\"",
                            instruction.name, existing_instr.name).c_str(),
                            symbol.source());
                }
            }

            it->second.push_back(instruction);
        } else {
            throw toml::parse_error(
                    "\"symbol\" parameter must be a list of strings",
                    symbol.source());
        }
    });
}


std::map<std::string, std::vector<InstructionConfig>> read_instructions(
        std::filesystem::path path, const std::map<std::string, UnitConfig>& units) {
    toml::table instructions_config = toml::parse_file(path.c_str());

    std::map<std::string, std::vector<InstructionConfig>> instructions;

    instructions_config.for_each([&instructions, units, instructions_config](auto& key, auto& config) {
        if constexpr (toml::is_table<decltype(config)>) {
            auto latency = config["latency"].template value_exact<int64_t>();
            if (!latency) {
                throw toml::parse_error("no or invalid \"latency\" parameter", config.source());
            } else if (*latency < 0) {
                throw toml::parse_error(
                        "\"latency\" parameter must be >= 0", config.source());
            }

            InstructionConfig instr = {.name = std::string{key.data()},
                                       .latency = static_cast<size_t>(*latency)};

            read_instruction_units(&instr, config, units);

            read_instruction_arguments(&instr, config);

            add_instruction_by_symbols(&instructions, config, std::move(instr));
        } else {
            throw toml::parse_error("invalid instruction declaration format",
                                    config.source());
        }
    });

    return instructions;
}

} //< anonymous namespace

Config::Config(std::filesystem::path units_path, std::filesystem::path instrs_path) try
    : units(read_units(units_path))
    , instructions(read_instructions(instrs_path, units))
{} catch (const toml::parse_error& err) {
    auto source = err.source();
    throw std::runtime_error(std::format("TOML config parsing error in file {}:{}: {}",
            source.path->data(), source.begin.line, err.description()));
}


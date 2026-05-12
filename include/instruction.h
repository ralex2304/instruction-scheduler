#pragma once

#include "arguments_parser.h"
#include "configs_parser.h"
#include "types.h"

#include <format>
#include <iostream>
#include <istream>
#include <ostream>
#include <set>
#include <sstream>

namespace scheduler {

class Instruction {
public:
    static Instruction create(std::string line, const Config& config) {
        std::istringstream line_stream(line);

        std::string symbol;
        line_stream >> std::ws >> symbol;

        auto instr_configs_it = config.instructions.find(symbol);
        if (instr_configs_it == config.instructions.end())
            throw std::runtime_error(std::format("Instruction symbol \"{}\" is undefined", symbol));

        auto line_stream_recover = line_stream.tellg();

        for (const auto& instr_config : instr_configs_it->second) {
            line_stream.seekg(line_stream_recover);

            std::optional<reg_t> dst_reg;
            std::set<reg_t> src_regs;
            bool is_memory = false;

            bool correct = true; //< true for instructions without arguments
            for (auto it = instr_config.arguments.begin(); it != instr_config.arguments.end(); it++) {
                const auto& arg = *it;

                correct = std::visit([&line_stream, &dst_reg, &src_regs, &is_memory](auto&& type) {
                    auto result = type.parse(line_stream);
                    if (!result) {
                        return false;
                    }

                    using T = std::decay_t<decltype(type)>;
                    if constexpr (std::is_same_v<T, RegArg<true>>) {
                        assert(!dst_reg && "Invalid instruction config: multiple destination registers");
                        dst_reg = *result;
                    } else if constexpr (std::is_same_v<T, RegArg<false>>) {
                        src_regs.insert(*result);
                    } else if constexpr (std::is_same_v<T, ImmArg>) {
                        // do nothing
                    } else if constexpr (std::is_same_v<T, MemoryImmArg>) {
                        is_memory = true;
                    } else if constexpr (std::is_same_v<T, MemoryRegImmArg>) {
                        src_regs.insert(result->first);
                        is_memory = true;
                    } else {
                        static_assert(0, "Not all argument types are handled");
                    }

                    return true;
                }, arg);

                if (!correct)
                    break;

                line_stream >> std::ws;
                if (it == instr_config.arguments.end()-1) {
                    if (!line_stream.eof()) {
                        correct = false;
                        break;
                    }
                } else if (line_stream.peek() == ',') {
                    line_stream.get();
                }
            }

            if (!correct)
                continue;

            return Instruction(std::move(dst_reg),
                               std::move(src_regs),
                               is_memory,
                               instr_config,
                               line);
        }

        throw std::runtime_error(std::format("Unknown instruction: \"{}\"", line));
    }

    void dump() {
        std::cout << "=== Instruction dump: " << instr_config_.name << " ===" << std::endl;
        if (dst_reg_)
            std::cout << "dst reg: R" << *dst_reg_ << std::endl;
        std::cout << "src regs:";
        for (auto src_reg : src_regs_)
            std::cout << " R" << src_reg;
        std::cout << std::endl;
        std::cout << "memory: " << is_memory_ << std::endl;
        std::cout << "line: " << line_ << std::endl;
    }

private:
    Instruction(std::optional<reg_t> dst_reg,
                std::set<reg_t> src_regs,
                bool is_memory,
                const InstructionConfig& instr_config,
                std::string line)
        : dst_reg_(std::move(dst_reg))
        , src_regs_(std::move(src_regs))
        , is_memory_(is_memory)
        , instr_config_(instr_config)
        , line_(line) {}

    const std::optional<reg_t> dst_reg_;
    const std::set<reg_t> src_regs_;
    const bool is_memory_;

    const InstructionConfig& instr_config_;
    const std::string line_;
};

} //< namespace scheduler


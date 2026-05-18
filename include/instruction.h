#pragma once

#include "configs_parser.h"
#include "types.h"

#include <iostream>
#include <ostream>
#include <set>

namespace scheduler {

class Instruction {
public:
    static Instruction create(std::string line, const Config& config);

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

    std::string get_line() const { return line_; }

    ticks_t get_latency() const { return instr_config_.latency; }

    std::optional<reg_t> get_dst_reg() const { return dst_reg_; }

    std::set<reg_t> get_src_regs() const { return src_regs_; }

    bool is_memory() const { return is_memory_; }

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


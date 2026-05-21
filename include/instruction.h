#pragma once

#include "configs_parser.h"
#include "types.h"

#include <set>

namespace scheduler {

class Instruction {
public:
    static Instruction create(std::string line, const Config& config);

    const InstructionConfig& get_config() const { return instr_config_; }

    std::optional<reg_t> get_dst_reg() const { return dst_reg_; }

    std::set<reg_t> get_src_regs() const { return src_regs_; }

    bool is_memory() const { return is_memory_; }

private:
    Instruction(std::optional<reg_t> dst_reg,
                std::set<reg_t> src_regs,
                bool is_memory,
                const InstructionConfig& instr_config)
        : dst_reg_(std::move(dst_reg))
        , src_regs_(std::move(src_regs))
        , is_memory_(is_memory)
        , instr_config_(instr_config) {}

    const std::optional<reg_t> dst_reg_;
    const std::set<reg_t> src_regs_;
    const bool is_memory_;

    const InstructionConfig& instr_config_;
};

} //< namespace scheduler


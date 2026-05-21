#pragma once

#include "types.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <variant>

namespace scheduler {

template <typename T>
concept ParsableArg = requires(std::istringstream& ss) {
    { T::parse(ss) };
};

template <bool DST>
struct RegArg {
    static std::optional<reg_t> parse(std::istringstream& ss) {
        ss >> std::ws;

        if (ss.peek() != 'R')
            return std::nullopt;
        ss.get();

        reg_t reg = 0;
        if (!(ss >> reg))
            throw std::runtime_error("register number parsing error");

        return reg;
    }

    bool operator==(const RegArg<DST>&) const = default;
    bool operator==(const RegArg<!DST>&) const { return true; }

    static constexpr bool is_dst = DST;
};

struct ImmArg {
    static std::optional<imm_t> parse(std::istringstream& ss) {
        ss >> std::ws;

        auto recovery_pos = ss.tellg();

        long num = 0;
        if (!(ss >> std::setbase(0) >> num >> std::setbase(10))) {
            ss.clear();
            ss.seekg(recovery_pos);
            return std::nullopt;
        }
        return num;
    }

    bool operator==(const ImmArg&) const = default;
};

struct RegImmArg {
    static std::optional<std::pair<reg_t, imm_t>> parse(std::istringstream& ss) {
        std::optional<reg_t> reg = std::nullopt;
        imm_t imm = 0;
        bool imm_found = false;

        std::optional<bool> sign = true;

        do {
            if (reg || !(reg = RegArg<false>::parse(ss))) {
                auto imm_parse = ImmArg::parse(ss);
                if (!imm_parse)
                    throw std::runtime_error("immediate parsing error");

                imm += sign ? *imm_parse : -*imm_parse;
                imm_found = true;
            }

            ss >> std::ws;
            if (ss.peek() == '+')
                sign = true;
            else if (ss.peek() == '-')
                sign = false;
            else
                sign = std::nullopt;

            if (sign)
                ss.get();

        } while (sign);

        if (!reg || !imm_found)
            return std::nullopt;

        return std::pair(*reg, imm);
    }

    bool operator==(const RegImmArg&) const = default;
};

template <ParsableArg T>
struct MemoryArgBase {
    static std::invoke_result_t<decltype(T::parse),
                                std::istringstream&> parse(std::istringstream& ss) {
        ss >> std::ws;
        if (ss.peek() != '[')
            return std::nullopt;
        ss.get();

        auto res = T::parse(ss);
        if (!res)
            return std::nullopt;

        ss >> std::ws;
        if (ss.get() != ']')
            return std::nullopt;

        return res;
    }

    bool operator==(const MemoryArgBase<T>&) const = default;
};

using MemoryImmArg = MemoryArgBase<ImmArg>;
using MemoryRegImmArg = MemoryArgBase<RegImmArg>;

using ArgsVariant = std::variant<RegArg<true>, RegArg<false>, ImmArg, MemoryImmArg, MemoryRegImmArg>;

} //< namespace scheduler


#include "configs_parser.h"
#include "data_flow_graph.h"
#include "scheduler.h"

#include <filesystem>

#include "gtest/gtest.h"

namespace {

using namespace scheduler;

#define DUMP_DIR (get_test_dump_dir(testing::UnitTest::GetInstance()->current_test_info()))

const std::filesystem::path TESTS_DIR = TESTS_SRC_DIR;
const std::filesystem::path CODE_DIR = TESTS_DIR / "code";
const std::filesystem::path CONFIGS_DIR = TESTS_DIR / "configs";

const std::filesystem::path UNITS_TOML        = TESTS_DIR / ".." / "assets" / "units.toml";
const std::filesystem::path INSTRUCTIONS_TOML = TESTS_DIR / ".." / "assets" / "instructions.toml";

constexpr bool GENERATE_DUMP_IMAGES = true;

std::filesystem::path get_test_dump_dir(const testing::TestInfo* const test_info) {
    using namespace std::filesystem;

    path dump_dir(TESTS_SRC_DIR);

    dump_dir /= path("dumps") / path(test_info->test_suite_name()) / path(test_info->name());

    std::filesystem::create_directories(dump_dir);

    return dump_dir;
}

const Config default_config(UNITS_TOML, INSTRUCTIONS_TOML);

TEST(ParsingTest, TaskExample) {
    EXPECT_NO_THROW({
        DataFlowGraph dfg(CODE_DIR / "task_example.s", default_config, GENERATE_DUMP_IMAGES);
        dfg.dump(get_test_dump_dir(test_info_) / "unscheduled_dump");
    });
}

TEST(ParsingTest, AllInstructions) {
    EXPECT_NO_THROW({
        DataFlowGraph dfg(CODE_DIR / "all_instructions.s", default_config, GENERATE_DUMP_IMAGES);
        dfg.dump(get_test_dump_dir(test_info_) / "unscheduled_dump");
    });
}

TEST(ExamplesTest, UnknownInstruction) {
    EXPECT_THROW({
        DataFlowGraph dfg(CODE_DIR / "unknown_instruction.s", default_config, GENERATE_DUMP_IMAGES);
    }, std::runtime_error);
}

TEST(ExamplesTest, RegisterRedefenition) {
    EXPECT_THROW({
        DataFlowGraph dfg(CODE_DIR / "register_redefinition.s", default_config, GENERATE_DUMP_IMAGES);
    }, std::runtime_error);
}

TEST(ExamplesTest, RegisterUndefined) {
    EXPECT_THROW({
        DataFlowGraph dfg(CODE_DIR / "register_undefined.s", default_config, GENERATE_DUMP_IMAGES);
    }, std::runtime_error);
}

TEST(LectionExampleTest, SchedulingTime) {
    Scheduler scheduler(CONFIGS_DIR / "lecture_example" / "units.toml",
                        CONFIGS_DIR / "lecture_example" / "instructions.toml",
                        CODE_DIR / "lecture_example.s",
                        get_test_dump_dir(test_info_));

    EXPECT_EQ(scheduler.get_end_time(), 15);
}

} // anonymous namespace

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}


#include <gtest/gtest.h>

#include <tinylogger/tinylogger.hpp>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(TinyLoggerTest, LogsToCout) {
	logger.log(LogLevel::TRACE, "This is a test log message. ", 1, 3.0, std::string("yes"), " I am");
	EXPECT_TRUE(true);
}
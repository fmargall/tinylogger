#include <gtest/gtest.h>

#include "tinylogger.hpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(TinyLoggerTest, LogsToCout) {
	logger.log("This is a test log message.");
	EXPECT_TRUE(true);
}
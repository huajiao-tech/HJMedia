#include <string>
#include <vector>
#include <iostream>
#include "gtest/gtest.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    ::testing::GTEST_FLAG(filter) = "HJRenderGraphTest*";

    return RUN_ALL_TESTS();
}

#include "gtest/gtest.h"

#include "../XMediaTools/HJLiveOnLineThumbCapture.h"

NS_HJ_USING;

TEST(HJLiveOnLineThumbCaptureTest, buildOutputDirUsesLevelAndUrlHash) {
    const std::string output_dir =
        buildThumbOutputDir("E:/thumb", "middle", "https://example.com/live/stream.m3u8");

    EXPECT_EQ(output_dir, "E:/thumb/middle/8dc7c62c18325890");
}

TEST(HJLiveOnLineThumbCaptureTest, scheduleRequiresTwoSecondIntervalsAndStopsAtFiveFrames) {
    HJThumbCaptureSchedule schedule;

    EXPECT_TRUE(schedule.shouldSaveFrame(0));
    schedule.markSaved(0);

    EXPECT_FALSE(schedule.shouldSaveFrame(1999));
    EXPECT_TRUE(schedule.shouldSaveFrame(2000));
    schedule.markSaved(2000);

    EXPECT_FALSE(schedule.shouldSaveFrame(3999));
    EXPECT_TRUE(schedule.shouldSaveFrame(4000));
    schedule.markSaved(4000);

    EXPECT_TRUE(schedule.shouldSaveFrame(6000));
    schedule.markSaved(6000);

    EXPECT_TRUE(schedule.shouldSaveFrame(8000));
    schedule.markSaved(8000);

    EXPECT_TRUE(schedule.isCompleted());
    EXPECT_FALSE(schedule.shouldSaveFrame(10000));
}

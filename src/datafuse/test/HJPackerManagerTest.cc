#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>
#include <numeric>

#include "../HJPackerManager.h"
#include "../HJLZ4Packer.h"
#include "../HJZlibPacker.h"
#include "../HJSegPacker.h"
#include "../HJXVibeTypes.h"

using namespace HJ;

class HJPackerManagerTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    HJVibeData::Ptr createTestData(const std::string& content, int seqId = 0) {
        std::vector<uint8_t> data(content.begin(), content.end());
        auto userData = HJUserData::make(data);
        auto vibeData = std::make_shared<HJVibeData>();
        vibeData->append(userData);
        (*vibeData)["seq_id"] = seqId;
        return vibeData;
    }

    std::string vibeDataToString(const HJVibeData::Ptr& data) {
        if (!data) return "";
        std::string result;
        for (size_t i = 0; i < data->size(); ++i) {
            auto u = data->get(i);
            result.append(u->getData().begin(), u->getData().end());
        }
        return result;
    }
};

TEST_F(HJPackerManagerTest, Construction) {
    auto manager = std::make_shared<HJPackerManager>();
    EXPECT_NE(manager, nullptr);
}

TEST_F(HJPackerManagerTest, PackNoPackers) {
    auto manager = std::make_shared<HJPackerManager>();
    auto data = createTestData("hello world");
    
    // Expect failure or empty return because no packers are registered
    // HJPackerManager::pack returns {} if m_packers is empty.
    auto result = manager->pack(data);
    EXPECT_EQ(result, nullptr);
}

TEST_F(HJPackerManagerTest, RegisterPackers) {
    auto manager = std::make_shared<HJPackerManager>();
    manager->registerPackers<HJLZ4Packer>();
    
    std::string msg = "TOOL CFG: width:720 height:1280 fps:25:1 timebase:1:25 vfr:0 vui:1:1:0:5:1:1:1:5:format:1 preset:7 tune:meetingCamera level:50 repeat-header:1 ref:3 long-term:0 open-gop:0 keyint:50:5 rc:1 cqp:32 cbr/abr:1548 crf:0:0:0 vbv-max-bitrate:2012:1 vbv-buf-size:2012:4 vbv-buf-init:0.9 max-frame-ratio:100 ip-factor:2 rc-peakratetol:5 qp:40:22:35:22 aq:5:1 wpp:1 pool-threads:2:64 svc:0 fine-tune:-1 roi:1:1 TOOL CFG: qpInitMin:22 qpInitMax:35 MaxRatePeriod:1 MaxBufPeriod:4 HAD:1 FDM:1 RQT:1 TransformSkip:0 SAO:1 TMVPMode:1 SignBitHidingFlag:1 MvStart:4 BiRefNum:0 FastSkipSub:1 EstimateCostSkipSub0:0 EstimateCostSkipSub1:0 EstimateCostSkipSub2:0 SkipCurFromSplit0:1 SkipCurFromSplit1:2 SkipCurFromSplit2:3 SubDiffThresh0:18 SubDiffThresh1:10 SubDiffThresh2:6 FastMergeSkip:1 SkipUV:1 RefineSkipUV:0 MergeSkipTh0:30000 MergeSkipTh1:30000 MergeSkipTh2:40 MergeSkipTh3:40 MergeSkipTh:0 MergeSkipDepth:0 MergeSkipEstopTh0:350 MergeSkipEstopTh1:250 MergeSkipEstopTh2:150 MergeSkipEstopTh3:100 CbfSkipIntra:1 neiIntraSkipIntra:6 SkipFatherIntra:0 SkipIntraFromRDCost:3 StopIntraFromDist0:11 StopIntraFromDist1:12 StopIntraFromDist2:14 StopIntraFromDist3:16 TopdownContentTh0:32 TopdownContentTh1:32 TopdownContentTh2:32 TopdownContentTh3:32 TopdownContentTh4:32 BottomUpContentTh0:0 BottomUpContentTh1:14 BottomUpContentTh2:12 BottomUpContentTh3:0 BottomUpContentTh4:0 BottomUpContentTh5:12 madth4merge:128 DepthSkipCur:0 CheckCurFromSubSkip:0 DepthSkipSub:3 StopCurFromNborCost:5 SkipFatherBySubmode:0 SkipL1ByNei:0 CheckCurFromNei:0 EarlySkipAfterSkipMerge:2 sccIntra8distTh:5000 sccIntraNxNdistTh:120 TuEarlyStopPu:18 FastSkipInterRdo:1 IntraCheckInInterFastSkipNxN0:17 IntraCheckInInterFastSkipNxN1:40 IntraCheckInIntraFastSkipNxN:0 IntraCheckInIntraSkipNxN:0 AmpSkipAmp:1 Skip2NAll:0 Skip2NFromSub0:600 Skip2NFromSub1:600 Skip2NFromSub2:650 Skip2NRatio0:450 Skip2NRatio1:450 Skip2NRatio2:450 SkipSubFromSkip2n:1 AdaptMergeNum:5 TuStopPu:18 qp2qstepbetter:610 qp2qstepfast:600 InterCheckMerge:1 skipIntraFromPreAnalysis0:18 skipIntraFromPreAnalysis1:18 skipIntraFromPreAnalysis2:18 DisNxNLevel:3 SubdiffSkipSub0:260 SubdiffSkipSub1:200 SubdiffSkipSub2:120 SubdiffSkipSub:0 SubdiffSkipSubRatio0:10 SubdiffSkipSubRatio1:10 SubdiffSkipSubRatio2:60 SubdiffSkipSubRatio:0 StopSubMaxCosth0:2048 StopSubMaxCosth1:0 StopSubMaxCosth2:0 StopSubMaxCosth3:0 CostSkipSub0:12 CostSkipSub1:6 CostSkipSub2:3 CostSkipSub:1 DistSkipSub0:12 DistSkipSub1:8 DistSkipSub2:4 DistSkipSub3:1 SkipSubNoCbf:1 SaoLambdaRatio0:7 SaoLambdaRatio1:8 SaoLambdaRatio2:8 SaoLambdaRatio3:9 AdaptiveSaoLambda[0]:60 AdaptiveSaoLambda[1]:90 AdaptiveSaoLambda[2]:120 AdaptiveSaoLambda[3]:150 SecModeInInter0:6 SecModeInInter1:0 SecModeInInter2:0 SecModeInInter3:4 SecModeInInter4:1 SecModeInIntra0:6 SecModeInIntra1:0 SecModeInIntra2:0 SecModeInIntra3:3 SecModeInIntra4:1 ChromaModeOptInInter0:0 ChromaModeOptInInter1:0 ChromaModeOptInInter2:0 IntraCheckInInterFastSkipUseNborCu:1 disframesao:1 skipTuSplit:16 FastQuantInter0:95 FastQuantInter1:95 FastQuantInterChroma:20 MeInterpMethod:0 MultiRef:1 BiMultiRef:0 BiMultiRefThr:0 MultiRefTh:0 MultiRefFast2nx2nTh:0 MvStart:4 StopSubFromNborCost:0 StopSubFromNborCost2:80 StopSubFromNborCount:0 imecorrect:1 fmecorrect:1 qmeguidefast:1 unifmeskip:3 fasthme:1 FasterFME:1 AdaptHmeRatio0:0 AdaptHmeRatio1:0 AdaptHmeRatio2:0 skipqme0:0 skipqme1:0 skipqme2:0 skipqme3:23 fastqmenumbaseframetype0:3 fastqmenumbaseframetype1:2 fastqmenumbaseframetype2:2 FastSubQme:12 adaptiveFmeAlg0:0 adaptiveFmeAlg1:0 adaptiveFmeAlg2:0 adaptiveFmeAlg3:0 GpbSkipL1Me:0 BackLongTermReference:1 InterSatd2Sad:2 qCompress:0.6 qCompressFirst: 0.5 CutreeLambdaPos: 2 CutreeLambdaNeg: 100 CutreeLambdaFactor: 0 AdaptPSNRSSIM:40 LHSatdscale:0 PMESearchRange:1 LookAheadHMVP:0 hmvp-thres0 LookAheadNoEarlySkip:1 LHSatdScaleTh:80 LHSatdScaleTh2:180 FreqDomainCostIntra0:-1 FreqDomainCostIntra1:-1 FreqDomainCostIntra2:-1 deblock(tc:beta):0, 0 RCWaitRatio1: 0.5 RCWaitRatio2: 0.2 RCConsist:0 QPstep:4 RoundKeyint:0 VbvPredictorSimple:0.5 VbvPredictorSimpleMin:0.5 VbvPredictorNormal:1 VbvPredictorNormalMin:1 VbvPredictorComplex:1 VbvPredictorComplexMin:1 DpbErrorResilience: 0 aud:0 lookahead-use-sad:1 intra-sad0:1 intra-sad1:1 intra-sad2:0 intra-sad3:0 intra-sad4:0 skip-sao-freq:0 skip-sao-den:0 skip-sao-dist:0 adaptive-quant:0 wpp2CachedTuCoeffRows:0 disablemergemode:0 merge-mode-depth:0 skipCudepth0:0 faster-code-coeff:0 rqt-split-first0 intra-fine-search-opt:0 sao-use-merge-mode0:0 sao-use-merge-mode1:0 sao-use-merge-mode2:0 enable-sameblock:0 sameblock-dqp:255 scclock-th:0 skip-child-by-sameblock:0 skip-no-zero-mv-by-sameblock:0 sameblock-disable-merge:0 tzsearchscc-adaptive-stepsize:0 skip-intra-fine-search-rd-thr2:0 skip-sub-from-father-skip:0 prune-split-from-rdcost:0 check-cur-from-subcost:0 skip-dct:0 list1-fast-ref-cost-thr:0 list1-fast-ref-cost-thr2:0 char-aq-cost-min:0 char-target-qp:0 content-early-stop-thresh0:0 content-early-stop-thresh1:0 content-early-stop-thresh2:0 content-early-stop-thresh3:0 sub-diff-skip-sub-chroma-weight:-1 skip-inter-by-intra:0 enable-contrast-enhancement:0 lowdelay-skip-ref:0 enable-adaptive-me:0 search-thresh:0 search-offset:0 skip-l1-by-sub:0 skip-dct-inter4x4:0 same-ctu-skip-sao:0 skip-l1-by-skip-dir:0 skip-bi-by-father:0 skip-l1-by-father:0 skip-l1-by-nei:0 enable-hmvp:0 check-cur-from-nei:0 skip-sub-from-skip-2n-cudepth:5 lp-single-ref-list-cudepth:-1 ime-round:16 camera-subjective-opt:1 enable-chroma-weight:0 disable-tu-early-stop-for-subjective:0 enable-subjective-loss:0 enable-contrast-enhancement:0 go-down-qp-for-subjective:51 skip-2n-from-sub-qp-for-subjective:51 skip-2n-from-sub-chroma-weight-for-subjective:0 skip-2n-from-sub-chroma-weight4-for-subjective:0 complex-block-thr-factor1-for-subjective:3 complex-block-thr-factor2-for-subjective:0 opt-try-go-up-for-subjective:0 large-qp-large-size-intra-use-4-angle:0 aq-adjust-for-subjective:0 enable-subjective-char-qp:0 hyperfast-tune-subjective:0 hyperfast-tune-detail:0 scenecut-dqp:0 skip-dct-offset0:0 skip-dct-offset1:0 skip-dct-offset2:0 skip-lowres-inter:0 lp-single-ref-list-cu-depth:-1 lp-single-ref-frame-cu-depth:-1 slice-depth0-fast-ref:0 fast-bi-pattern-search:0 skip-intranxn-in-bslice:0 intra-stride-inter-frame:0 topdown-father-skip:0 skip-father-by-submode:0 skip-coarse-intra-pred-mode-by-cost:0 skip-inter64x64:0 skip-sub-from-skip-2n-cudepth:5 inter-search-lowres-mv:0 skip-dct:0 stop-split-by-neibor-skip:0 intra-non-std-dct:0 pool-mask:0 max-merge:3 tu-inter-depth:1 tu-intra-depth:1 sr:256 me:1 pme:0 max-dqp-depth:1 cb-qp-offset:4 cr-qp-offset:4 lookahead-threads:1 vbv-adapt:0 vbv-control:0 roi2-strength:0 roi2-uv-delta:0 roi2-height-up-scale:0.25 roi2-height-down-scale:0.25 roi2-margin-left-scale:0 roi2-margin-right-scale:0";
    auto data = createTestData("test registration " + msg);
    auto result = manager->pack(data);
    EXPECT_NE(result, nullptr);
}

TEST_F(HJPackerManagerTest, PackUnpack_LZ4) {
    auto manager = std::make_shared<HJPackerManager>();
    manager->registerPackers<HJLZ4Packer>();
    
    std::string originalStr = "This is a test string for LZ4 compression. It should be long enough to be worth compressing.";
    auto data = createTestData(originalStr, 1001);
    
    auto packed = manager->pack(data);
    ASSERT_NE(packed, nullptr);
    
    // Verify header structure roughly (optional, black box test prefers checking unpack result)
    
    auto unpacked = manager->unpack(packed);
    ASSERT_NE(unpacked, nullptr);
    
    std::string unpackedStr = vibeDataToString(unpacked);
    EXPECT_EQ(originalStr, unpackedStr);
}

TEST_F(HJPackerManagerTest, PackUnpack_Zlib) {
    auto manager = std::make_shared<HJPackerManager>();
    manager->registerPackers<HJZlibPacker>();
    
    std::string originalStr = "This is a test string for Zlib compression. It should be long enough to be worth compressing. Zlib is usually better than LZ4 for ratio.";
    auto data = createTestData(originalStr, 1002);
    
    auto packed = manager->pack(data);
    ASSERT_NE(packed, nullptr);
    
    auto unpacked = manager->unpack(packed);
    ASSERT_NE(unpacked, nullptr);
    
    std::string unpackedStr = vibeDataToString(unpacked);
    EXPECT_EQ(originalStr, unpackedStr);
}

TEST_F(HJPackerManagerTest, PackUnpack_Seg) {
    auto manager = std::make_shared<HJPackerManager>();
    manager->registerPackers<HJSegPacker>();
    
    // Create data larger than typical segment size (e.g. 1KB defined in HJXVibeTypes.h as HJ_SEI_SEG_MAX_SIZE)
    // HJ_SEI_SEG_MAX_SIZE is 1 * 1024
    std::string originalStr(2500, 'S'); 
    auto data = createTestData(originalStr, 2002);
    
    auto packed = manager->pack(data);
    ASSERT_NE(packed, nullptr);
    // Should have multiple segments
    EXPECT_GT(packed->size(), 1);
    
    auto unpacked = manager->unpack(packed);
    ASSERT_NE(unpacked, nullptr);
    
    std::string unpackedStr = vibeDataToString(unpacked);
    EXPECT_EQ(originalStr, unpackedStr);
}

TEST_F(HJPackerManagerTest, PackUnpack_Chain) {
    auto manager = std::make_shared<HJPackerManager>();
    // Chain: LZ4 -> Seg
    manager->registerPackers<HJLZ4Packer, HJSegPacker>();
    
    std::string originalStr(5000, 'C'); 
    auto data = createTestData(originalStr, 3003);
    
    auto packed = manager->pack(data);
    ASSERT_NE(packed, nullptr);
    
    auto unpacked = manager->unpack(packed);
    ASSERT_NE(unpacked, nullptr);
    
    std::string unpackedStr = vibeDataToString(unpacked);
    EXPECT_EQ(originalStr, unpackedStr);
}

TEST_F(HJPackerManagerTest, InvalidData) {
    auto manager = std::make_shared<HJPackerManager>();
    manager->registerPackers<HJLZ4Packer>();
    
    // Pack null
    auto result = manager->pack(nullptr);
    EXPECT_EQ(result, nullptr);
    
    // Pack empty
    auto emptyData = std::make_shared<HJVibeData>();
    auto result2 = manager->pack(emptyData);
    EXPECT_EQ(result2, nullptr);
    
    // Unpack null
    auto result3 = manager->unpack(nullptr);
    EXPECT_EQ(result3, nullptr);
    
    // Unpack invalid data (garbage)
    std::vector<uint8_t> garbage(100, 0xFF);
    auto garbageData = HJUserData::make(garbage);
    auto garbageVibe = std::make_shared<HJVibeData>();
    garbageVibe->append(garbageData);
    
    auto result4 = manager->unpack(garbageVibe);
    EXPECT_EQ(result4, nullptr);
}

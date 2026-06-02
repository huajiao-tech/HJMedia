#include <gtest/gtest.h>
#include "HJJson.h"
#include "HJUtilitys.h"

#define HJ_HAVE_JSON_REFLECT_TEST    1

NS_HJ_BEGIN

class mediaInfo : public HJInterpreter
{
public:

    // For test verification
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getFramerate() const { return framerate; }
    double getTime() const { return time; }
    const std::string& getUrl() const { return url; }
    const std::vector<int>& getDatas() const { return datas; }

public:
    int 	width = 0;
    int 	height = 0;
    float 	framerate = 0.0f;
    double  time = 0.0;
    std::string url;
    std::vector<int> datas;

    HJ_JSON_REFLECT_BEGIN(mediaInfo)
        HJ_JSON_REFLECT_MEMBER(width)
        HJ_JSON_REFLECT_MEMBER(height)
        HJ_JSON_REFLECT_MEMBER(framerate)
        HJ_JSON_REFLECT_MEMBER(time)
        HJ_JSON_REFLECT_MEMBER(url)
        HJ_JSON_REFLECT_MEMBER(datas)
    HJ_JSON_REFLECT_END(mediaInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(mediaInfo)

class constroInfo : public HJInterpreter
{
public:

    int getTimeout() const { return timeout; }
    short getRdLimited() const { return rd_limited; }
    const std::string& getClsId() const { return cls_id; }
    const mediaInfo& getMInfo() const { return minfo; }

public:
	int 		timeout = 0;
	short 		rd_limited = 0;
	std::string cls_id;
	
	mediaInfo  minfo;

    HJ_JSON_REFLECT_BEGIN(constroInfo)
        HJ_JSON_REFLECT_MEMBER(timeout)
        HJ_JSON_REFLECT_MEMBER(rd_limited)
        HJ_JSON_REFLECT_MEMBER(cls_id)
        HJ_JSON_REFLECT_MEMBER(minfo)
    HJ_JSON_REFLECT_END(constroInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(constroInfo)

#if HJ_HAVE_JSON_REFLECT_TEST
TEST(HJJsonReflectionTest, MediaInfoDeserialization) {
    std::string jsonStr = R"({
        "width": 720,
        "height": 1280,
        "framerate": 25.0,
        "time": 1233.21,
        "url": "psdk.mp4",
        "datas": [12, 13, 14, 15]
    })";

    auto doc = HJYJsonDocument::createWithInfo(jsonStr);
    ASSERT_NE(doc, nullptr);

    mediaInfo info;
    info.deserialInfo(doc);

    EXPECT_EQ(info.getWidth(), 720);
    EXPECT_EQ(info.getHeight(), 1280);
    EXPECT_NEAR(info.getFramerate(), 25.0f, 0.001);
    EXPECT_DOUBLE_EQ(info.getTime(), 1233.21);
    EXPECT_EQ(info.getUrl(), "psdk.mp4");
    
    std::vector<int> expectedDatas = {12, 13, 14, 15};
    EXPECT_EQ(info.getDatas(), expectedDatas);
}

TEST(HJJsonReflectionTest, ConstroInfoDeserialization) {
    std::string jsonStr = R"({
        "timeout": 720,
        "rd_limited": 1280,
        "cls_id": "psdk.mp4",
        "minfo": {
            "width": 1920,
            "height": 1080,
            "url": "test.mp4"
        }
    })";

    auto doc = HJYJsonDocument::createWithInfo(jsonStr);
    ASSERT_NE(doc, nullptr);

    constroInfo info;
    info.deserialInfo(doc);

    EXPECT_EQ(info.getTimeout(), 720);
    EXPECT_EQ(info.getRdLimited(), 1280);
    EXPECT_EQ(info.getClsId(), "psdk.mp4");
    
    // Check nested object
    EXPECT_EQ(info.getMInfo().getWidth(), 1920);
    EXPECT_EQ(info.getMInfo().getHeight(), 1080);
    EXPECT_EQ(info.getMInfo().getUrl(), "test.mp4");
}

TEST(HJJsonReflectionTest, InterpreterConstructorAndSerialization) {
    std::string jsonStr = R"({
        "timeout": 720,
        "rd_limited": 1280,
        "cls_id": "psdk.mp4",
        "minfo": {
            "width": 1920,
            "height": 1080,
            "url": "test.mp4"
        }
    })";
    
    // Test constructor
    constroInfo info(jsonStr);
    EXPECT_EQ(info.getTimeout(), 720);
    EXPECT_EQ(info.getRdLimited(), 1280);
    EXPECT_EQ(info.getClsId(), "psdk.mp4");

    // Test getSerialInfo
    constroInfo info2;
    info2.timeout = 720;
	info2.rd_limited = 1280;
    info2.cls_id = "psdk.mp4";
    std::string output = info2.getSerialInfo();
    EXPECT_TRUE(output.find("\"timeout\":720") != std::string::npos);
    EXPECT_TRUE(output.find("\"rd_limited\":1280") != std::string::npos);
    EXPECT_TRUE(output.find("\"cls_id\":\"psdk.mp4\"") != std::string::npos);
}

TEST(HJJsonReflectionTest, ExceptionHandling) {
    // Empty string
    EXPECT_THROW(constroInfo(""), std::runtime_error);
    // Invalid JSON
    EXPECT_THROW(constroInfo("{invalid:json}"), std::runtime_error);
}

class TextureInfo : public HJInterpreter
{
public:
    int mframeCount = 0;
    int radius_Type = 0;
    int mid_Type = 0;
    int scale_Type = 0;
    double scale_ratio = 0;
    double anchor_offset_x = 0;
    double anchor_offset_y = 0;
    double asize_offset_x = 0;
    double asize_offset_y = 0;
    int mfaceCount = 0;
    std::string imageName;

    HJ_JSON_REFLECT_BEGIN(TextureInfo)
        HJ_JSON_REFLECT_MEMBER(mframeCount)
        HJ_JSON_REFLECT_MEMBER(radius_Type)
        HJ_JSON_REFLECT_MEMBER(mid_Type)
        HJ_JSON_REFLECT_MEMBER(scale_Type)
        HJ_JSON_REFLECT_MEMBER(scale_ratio)
        HJ_JSON_REFLECT_MEMBER(anchor_offset_x)
        HJ_JSON_REFLECT_MEMBER(anchor_offset_y)
        HJ_JSON_REFLECT_MEMBER(asize_offset_x)
        HJ_JSON_REFLECT_MEMBER(asize_offset_y)
        HJ_JSON_REFLECT_MEMBER(mfaceCount)
        HJ_JSON_REFLECT_MEMBER(imageName)
    HJ_JSON_REFLECT_END(TextureInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(TextureInfo)

class StickerInfo : public HJInterpreter
{
public:
    std::string Name;
    std::string ID;
    int Type = 0;
    int loop = 0;
    std::vector<TextureInfo> texture;

    HJ_JSON_REFLECT_BEGIN(StickerInfo)
        HJ_JSON_REFLECT_MEMBER(Name)
        HJ_JSON_REFLECT_MEMBER(ID)
        HJ_JSON_REFLECT_MEMBER(Type)
        HJ_JSON_REFLECT_MEMBER(loop)
        HJ_JSON_REFLECT_MEMBER(texture)
    HJ_JSON_REFLECT_END(StickerInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(StickerInfo)
 
class PairTestInfo : public HJInterpreter
{
public:
    std::vector<std::pair<std::string, int>> other_dirs_options;

    HJ_JSON_REFLECT_BEGIN(PairTestInfo)
        HJ_JSON_REFLECT_MEMBER(other_dirs_options)
    HJ_JSON_REFLECT_END(PairTestInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(PairTestInfo)

TEST(HJJsonReflectionTest, StickerInfoDeserialization) {
   std::string jsonStr = R"({
       "Name": "壁咚",
       "ID": "bidong",
       "Type": 0,
       "loop": 1,
       "texture": [
           {
               "mframeCount": 62,
               "radius_Type": 0,
               "mid_Type": 1,
               "scale_Type": 0,
               "scale_ratio": 98.0999984741211,
               "anchor_offset_x": -233.51777700770214,
               "anchor_offset_y": -241.14983106644104,
               "asize_offset_x": 530,
               "asize_offset_y": 530,
               "mfaceCount": 1,
               "imageName": "sticker1_"
           }
       ]
   })";

   auto doc = HJYJsonDocument::createWithInfo(jsonStr);
   ASSERT_NE(doc, nullptr);

   StickerInfo info;
   info.deserialInfo(doc);

   EXPECT_EQ(info.Name, "壁咚");
   EXPECT_EQ(info.ID, "bidong");
   EXPECT_EQ(info.Type, 0);
   EXPECT_EQ(info.loop, 1);
   
   ASSERT_EQ(info.texture.size(), 1);
   const auto& tex = info.texture[0];
   EXPECT_EQ(tex.mframeCount, 62);
   EXPECT_EQ(tex.radius_Type, 0);
   EXPECT_EQ(tex.mid_Type, 1);
   EXPECT_EQ(tex.scale_Type, 0);
   EXPECT_NEAR(tex.scale_ratio, 98.0999984741211, 0.0001);
   EXPECT_NEAR(tex.anchor_offset_x, -233.51777700770214, 0.000000001);
   EXPECT_NEAR(tex.anchor_offset_y, -241.14983106644104, 0.000000001);
   EXPECT_EQ(tex.asize_offset_x, 530);
   EXPECT_EQ(tex.asize_offset_y, 530);
   EXPECT_EQ(tex.mfaceCount, 1);
   EXPECT_EQ(tex.imageName, "sticker1_");
}

TEST(HJJsonReflectionTest, TextureInfoDeserialization) {
    std::string jsonStr = R"({
        "mframeCount": 62,
        "radius_Type": 0,
        "mid_Type": 1,
        "scale_Type": 0,
        "scale_ratio": 98,
        "anchor_offset_x": -233,
        "anchor_offset_y": -241,
        "asize_offset_x": 530,
        "asize_offset_y": 530,
        "mfaceCount": 1,
        "imageName": "sticker1_"
    })";

    auto doc = HJYJsonDocument::createWithInfo(jsonStr);
    ASSERT_NE(doc, nullptr);

    TextureInfo info;
    info.deserialInfo(doc);

    return;
}
 
TEST(HJJsonReflectionTest, PairVectorDeserialization) {
    std::string jsonStr = R"({
        "other_dirs_options": [
            {"name": "dir1", "max": 10},
            {"name": "dir2", "max": 20}
        ]
    })";

    auto doc = HJYJsonDocument::createWithInfo(jsonStr);
    ASSERT_NE(doc, nullptr);

    PairTestInfo info;
    info.deserialInfo(doc);

    ASSERT_EQ(info.other_dirs_options.size(), 2);
    EXPECT_EQ(info.other_dirs_options[0].first, "dir1");
    EXPECT_EQ(info.other_dirs_options[0].second, 10);
    EXPECT_EQ(info.other_dirs_options[1].first, "dir2");
    EXPECT_EQ(info.other_dirs_options[1].second, 20);
}

TEST(HJJsonReflectionTest, PairVectorSerialization) {
    PairTestInfo info;
    info.other_dirs_options.push_back({"test_dir", 99});

    std::string output = info.getSerialInfo();
    EXPECT_TRUE(output.find("\"name\":\"test_dir\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"max\":99") != std::string::npos);
}

TEST(HJJsonReflectionTest, VectorToStringFormatsIntegralVectors) {
    const std::vector<int> int_values = {-1, 0, 2};
    const std::vector<size_t> size_values = {1, 2, 3};

    EXPECT_EQ(HJUtilitys::vectorToString(int_values), "[-1, 0, 2]");
    EXPECT_EQ(HJUtilitys::vectorToString(size_values), "[1, 2, 3]");
}

TEST(HJJsonReflectionTest, VectorToStringFormatsEmptyVector) {
    const std::vector<int> empty_values;

    EXPECT_EQ(HJUtilitys::vectorToString(empty_values), "[]");
    EXPECT_EQ(HJUtilitys::vectorToString(empty_values, "|", "{", "}"), "{}");
}
#endif //HJ_HAVE_JSON_REFLECT_TEST

class HJPlayerContextInitConfig : public HJInterpreter {
public:
    bool valid{false};
    std::string logDir{""};
    int logLevel{0};
    int logMode{0};
    int maxSize{0};
    int maxFiles{0};
    //
    std::string medias_dir;
    int medias_cache_max;
    std::vector<std::pair<std::string, int>> other_dirs_options;
    int download_retry_max = 10;

    HJ_JSON_REFLECT_BEGIN(HJPlayerContextInitConfig)
        HJ_JSON_REFLECT_MEMBER(valid)
        HJ_JSON_REFLECT_MEMBER(logDir)
        HJ_JSON_REFLECT_MEMBER(logLevel)
        HJ_JSON_REFLECT_MEMBER(logMode)
        HJ_JSON_REFLECT_MEMBER(maxSize)
        HJ_JSON_REFLECT_MEMBER(maxFiles)
        HJ_JSON_REFLECT_MEMBER(medias_dir)
        HJ_JSON_REFLECT_MEMBER(medias_cache_max)
        HJ_JSON_REFLECT_MEMBER(other_dirs_options)
        HJ_JSON_REFLECT_MEMBER(download_retry_max)
    HJ_JSON_REFLECT_END(HJPlayerContextInitConfig)    
};
HJ_JSON_REFLECT_IMPLEMENT(HJPlayerContextInitConfig)

TEST(HJJsonReflectionTest, HJPlayerContextInitConfigDeserialization) {
    std::string jsonStr = R"({
        "valid": 1,
        "logDir": "xx",
        "logLevel": 2,
        "logMode": 1,
        "maxSize": 5,
        "maxFiles": 5,
        "medias_dir": "",
        "medias_cache_max": 200,
        "other_dirs_options": [
            {
                "name": "d:/media",
                "max": 100
            },
            {
                "name": "c:/kdk",
                "max": 200
            }
        ],
        "download_retry_max": 10
    })";

    // auto doc = HJYJsonDocument::createWithInfo(jsonStr);
    // ASSERT_NE(doc, nullptr);
    //try {
    //    HJPlayerContextInitConfig info(jsonStr);
    //    info.deserialInfo();
    //} catch (const std::exception& e) {
    //    std::cout << e.what() << std::endl;
    //}

    HJPlayerContextInitConfig info(jsonStr);
    info.deserialInfo();

    return;
}

NS_HJ_END

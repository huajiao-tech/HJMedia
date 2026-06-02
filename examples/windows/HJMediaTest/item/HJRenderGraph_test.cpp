#include <deque>



#include "gtest/gtest.h"

#include "HJRteGraphProc.h"
#include "HJRteGraphSetupInfo.h"
#include "HJRteCom.h"

//#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

using namespace HJ;

namespace
{
class TestGraphProcConfigSetup : public HJRteGraphProcConfigSetup
{
public:
    int constructGraph(HJBaseParam::Ptr /*i_param*/) override
    {
        // In tests we construct nodes manually via nodeCreate/nodeConnect.
        return HJ_OK;
    }
};

HJRteJsonNode::Ptr makeNode(const std::string& classStyle,
                            const std::string& insName,
                            const std::string& role,
                            bool enable = true,
                            bool mainSource = false)
{
    auto node = HJRteJsonNode::Create();
    node->setClassStyle(classStyle);
    node->setInsName(insName);
    node->setRole(role);
    node->setEnable(enable);
    node->setMainSource(mainSource);
    return node;
}

std::string makeNodeJson(const HJRteJsonNode::Ptr& node)
{
    return node->initSerial();
}

std::string defaultLinkJson()
{
    auto link = HJRteJsonLinkInfo::Create();
    return link->initSerial();
}

std::shared_ptr<HJRteCom> findCom(const std::deque<std::shared_ptr<HJRteCom>>& coms,
                                  const std::string& insName)
{
    for (auto& com : coms)
    {
        if (com->getInsName() == insName)
        {
            return com;
        }
    }
    return nullptr;
}

int waitForTasks(const std::shared_ptr<TestGraphProcConfigSetup>& graph)
{
    return graph->syncOverride([]() { return HJ_OK; });
}
} // namespace

class HJRenderGraphTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        graph = std::make_shared<TestGraphProcConfigSetup>();
        ASSERT_NE(nullptr, graph);
        ASSERT_EQ(0, graph->init(HJBaseParam::Create()));
    }

    void TearDown() override
    {
        if (graph)
        {
            graph->done();
        }
    }

    std::shared_ptr<TestGraphProcConfigSetup> graph;
};

TEST_F(HJRenderGraphTest, SetEnableTogglesNodeState)
{
    auto node = makeNode(HJRteGraphConfig::HJNodeClass_SourceBridge,
                         "source_one",
                         HJRteGraphConfig::HJNodeRole_Source,
                         true,
                         true);
    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(node)));
    ASSERT_EQ(0, waitForTasks(graph));

    auto source = findCom(graph->getSources(), "source_one");
    ASSERT_NE(nullptr, source);
    EXPECT_TRUE(source->isEnable());

    EXPECT_EQ(0, graph->setEnable(HJRteGraphConfig::HJNodeClass_SourceBridge, "source_one", false, ""));
    ASSERT_EQ(0, waitForTasks(graph));
    EXPECT_FALSE(source->isEnable());

    EXPECT_EQ(0, graph->setEnable(HJRteGraphConfig::HJNodeClass_SourceBridge, "source_one", true, ""));
    ASSERT_EQ(0, waitForTasks(graph));
    EXPECT_TRUE(source->isEnable());
}

TEST_F(HJRenderGraphTest, ReleaseNodeReconnectsPipeline)
{
    auto srcNode = makeNode(HJRteGraphConfig::HJNodeClass_SourceBridge,
                            "src",
                            HJRteGraphConfig::HJNodeRole_Source,
                            true,
                            true);
    auto midNode = makeNode(HJRteGraphConfig::HJNodeClass_FilterCopy2D,
                            "mid",
                            HJRteGraphConfig::HJNodeRole_Filter);
    auto dstNode = makeNode(HJRteGraphConfig::HJNodeClass_TargetUI_0,
                            "dst",
                            HJRteGraphConfig::HJNodeRole_Target);

    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(srcNode)));
    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(midNode)));
    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(dstNode)));
    ASSERT_EQ(0, waitForTasks(graph));

    ASSERT_EQ(0, graph->nodeConnect(HJRteGraphConfig::HJNodeClass_SourceBridge,
                                    "src",
                                    HJRteGraphConfig::HJNodeClass_FilterCopy2D,
                                    "mid",
                                    "",
                                    defaultLinkJson()));
    ASSERT_EQ(0, graph->nodeConnect(HJRteGraphConfig::HJNodeClass_FilterCopy2D,
                                    "mid",
                                    HJRteGraphConfig::HJNodeClass_TargetUI_0,
                                    "dst",
                                    "",
                                    defaultLinkJson()));
    ASSERT_EQ(0, waitForTasks(graph));

    auto src = findCom(graph->getSources(), "src");
    auto mid = findCom(graph->getFilters(), "mid");
    auto dst = findCom(graph->getTargets(), "dst");
    ASSERT_NE(nullptr, src);
    ASSERT_NE(nullptr, mid);
    ASSERT_NE(nullptr, dst);
    ASSERT_EQ(1u, src->getNextQueue().size());
    ASSERT_EQ(1u, mid->getPreQueue().size());
    ASSERT_EQ(1u, mid->getNextQueue().size());

    EXPECT_EQ(0, graph->nodeDelete(HJRteGraphConfig::HJNodeClass_FilterCopy2D, "mid", true));
    ASSERT_EQ(0, waitForTasks(graph));

    EXPECT_TRUE(graph->getFilters().empty());
    ASSERT_EQ(1u, src->getNextQueue().size());
    auto newLink = HJRteComLink::GetPtrFromWtr(src->getNextQueue().front());
    ASSERT_NE(nullptr, newLink);
    EXPECT_EQ(dst, newLink->getDstComPtr());
    ASSERT_EQ(1u, dst->getPreQueue().size());
}

TEST_F(HJRenderGraphTest, ConnectNodeUsesLinkInfo)
{
    auto srcNode = makeNode(HJRteGraphConfig::HJNodeClass_SourceBridge,
                            "src_for_connect",
                            HJRteGraphConfig::HJNodeRole_Source,
                            true,
                            true);
    auto dstNode = makeNode(HJRteGraphConfig::HJNodeClass_TargetUI_0,
                            "dst_for_connect",
                            HJRteGraphConfig::HJNodeRole_Target);

    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(srcNode)));
    ASSERT_EQ(0, graph->nodeCreate(makeNodeJson(dstNode)));
    ASSERT_EQ(0, waitForTasks(graph));

    auto linkInfoCfg = HJRteJsonLinkInfo::Create();
    linkInfoCfg->setLink(HJRteGraphConfig::HJNodeRenderType_Fit, 0.1, 0.2, 0.3, 0.4, true, false);
    const std::string linkInfoStr = linkInfoCfg->initSerial();

    EXPECT_EQ(0, graph->nodeConnect(HJRteGraphConfig::HJNodeClass_SourceBridge,
                                    "src_for_connect",
                                    HJRteGraphConfig::HJNodeClass_TargetUI_0,
                                    "dst_for_connect",
                                    "",
                                    linkInfoStr));
    ASSERT_EQ(0, waitForTasks(graph));

    auto src = findCom(graph->getSources(), "src_for_connect");
    auto dst = findCom(graph->getTargets(), "dst_for_connect");
    ASSERT_NE(nullptr, src);
    ASSERT_NE(nullptr, dst);
    ASSERT_EQ(1u, src->getNextQueue().size());
    auto link = HJRteComLink::GetPtrFromWtr(src->getNextQueue().front());
    ASSERT_NE(nullptr, link);
    EXPECT_EQ(dst, link->getDstComPtr());
    auto info = link->getLinkInfo();
    ASSERT_NE(nullptr, info);
    EXPECT_NEAR(0.1f, info->m_x, 1e-5f);
    EXPECT_NEAR(0.2f, info->m_y, 1e-5f);
    EXPECT_NEAR(0.3f, info->m_width, 1e-5f);
    EXPECT_NEAR(0.4f, info->m_height, 1e-5f);
    EXPECT_TRUE(info->m_xMirror);
    EXPECT_FALSE(info->m_yMirror);
}

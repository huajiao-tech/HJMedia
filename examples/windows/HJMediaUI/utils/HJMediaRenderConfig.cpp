#include "HJMediaRenderConfig.h"


NS_HJ_BEGIN
std::string HJMediaRenderConfig::m_configSourceRender = R"({
	"nodes": [
		{
			"classStyle": "HJNodeClass_SourceBridgeMediaData",
			"insName": "HJNodeClass_SourceBridgeMediaData",
			"enable": true,
			"role": "HJNodeRole_Source",
			"isMainSource": true
		},
		{
			"classStyle": "HJNodeClass_TargetPBO_0",
			"insName": "HJNodeClass_TargetPBO_0",
			"enable": true,
			"role": "HJNodeRole_Target",
			"insName": "HJNodeClass_TargetPBO_0",
			"isMainSource": false
		}
	],
	"links": [
		{
			"from": "HJNodeClass_SourceBridgeMediaData",
			"to": "HJNodeClass_TargetPBO_0",
			"shaderType": "HJNodeShaderType_Copy2D",
			"link": {
				"renderMode": "HJNodeRenderType_Clip",
				"x": 0.0,
				"y": 0.0,
				"w": 1.0,
				"h": 1.0,
				"xMirror": false,
				"yMirror": false,
				"linkId": ""
			}
		}
	]
})";



std::string HJMediaRenderConfig::GetConfig(HJMediaRenderConfigType type)
{
    switch (type)
    {
	case HJMediaRenderConfigType::HJMediaRenderConfigType_SourceRender:
        return m_configSourceRender;
    default:
        return "";
    }
	return "";
}

NS_HJ_END

#include "HJNativeExportCommon.h"
#include "HJEntryBaseRender.h"
#include "HJNativeCommon.h"
#include "HJNapiUtils.h"
#include "HJTypesMacro.h"
#include "HJComUtils.h"
#include "HJMediaData.h"
#include <limits>

NS_HJ_BEGIN

namespace
{
int priFillNodeParam(const HJBaseParam::Ptr& i_param, const std::string& i_info)
{
    if (!i_param || i_info.empty())
    {
        return HJ_OK;
    }

    auto jsonDoc = HJYJsonDocument::createWithInfo(i_info);
    if (!jsonDoc)
    {
        return HJErrInvalidParams;
    }

    for (const auto& key : jsonDoc->getKeys())
    {
        HJYJsonObject::Ptr valueObj = jsonDoc->getObj(key);
        if (!valueObj)
        {
            continue;
        }
        if (valueObj->isBool())
        {
            HJ_CatchMapPlainSetVal(i_param, bool, key, valueObj->getBool());
            continue;
        }
        if (valueObj->isStr())
        {
            HJ_CatchMapPlainSetVal(i_param, std::string, key, valueObj->getString());
            continue;
        }
        if (valueObj->isReal())
        {
            HJ_CatchMapPlainSetVal(i_param, float, key, static_cast<float>(valueObj->getReal()));
            continue;
        }
        if (valueObj->isInt64())
        {
            const int64_t value = valueObj->getInt64();
            if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max())
            {
                HJ_CatchMapPlainSetVal(i_param, int, key, static_cast<int>(value));
            }
            else
            {
                HJ_CatchMapPlainSetVal(i_param, int64_t, key, value);
            }
            continue;
        }
        if (valueObj->isUInt64())
        {
            const uint64_t value = valueObj->getUInt64();
            HJ_CatchMapPlainSetVal(i_param, uint64_t, key, value);
        }
    }

    return HJ_OK;
}
}

HJNativeExportCommon& HJNativeExportCommon::Instance()
{
    static HJNativeExportCommon instance;
    return instance;
}
    
int HJNativeExportCommon::Export(napi_env env, napi_value exports)
{
    int i_err = HJ_OK;
    do 
    {
        if ((env == nullptr) || (exports == nullptr)) 
        {
            i_err = HJErrInvalidParams;
            HJFLoge("Export: invalid params");
            break;
        }

        napi_property_descriptor desc[] = 
        {
            //DECLARE_NAPI_FUNCTION_VAL(n_contextInit, HJNativeExportCommon::contextInit),
    
            DECLARE_NAPI_FUNCTION_VAL(n_setFaceInfo, HJNativeExportCommon::setFaceInfo),
            DECLARE_NAPI_FUNCTION_VAL(n_nativeSourceOpen, HJNativeExportCommon::nativeSourceOpen),
            DECLARE_NAPI_FUNCTION_VAL(n_nativeSourceClose, HJNativeExportCommon::nativeSourceClose),
            DECLARE_NAPI_FUNCTION_VAL(n_nativeSourceAcquire, HJNativeExportCommon::nativeSourceAcquire),
            DECLARE_NAPI_FUNCTION_VAL(n_openFaceu, HJNativeExportCommon::openFaceu),
            DECLARE_NAPI_FUNCTION_VAL(n_closeFaceu, HJNativeExportCommon::closeFaceu),
            DECLARE_NAPI_FUNCTION_VAL(n_setVideoEncQuantOffset, HJNativeExportCommon::setVideoEncQuantOffset),
            DECLARE_NAPI_FUNCTION_VAL(n_setFaceProtected, HJNativeExportCommon::setFaceProtected),
            
            DECLARE_NAPI_FUNCTION_VAL(n_nodeEnable, HJNativeExportCommon::nodeEnable),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeSetParam, HJNativeExportCommon::nodeSetParam),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeCreate, HJNativeExportCommon::nodeCreate),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeConnect, HJNativeExportCommon::nodeConnect),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeDelete, HJNativeExportCommon::nodeDelete),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeDisconnect, HJNativeExportCommon::nodeDisconnect),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeLinkInfoChange, HJNativeExportCommon::nodeLinkInfoChange),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeGetPre, HJNativeExportCommon::nodeGetPre),
            DECLARE_NAPI_FUNCTION_VAL(n_nodeGetNext, HJNativeExportCommon::nodeGetNext),
        };
        
        if (napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc) != napi_ok) 
        {
            i_err = HJErrInvalidParams;
            HJFLoge("napi_define_properties error Export: invalid params");
            break;
        }    
    } while (false);
    return i_err;
}

// napi_value HJNativeExportCommon::contextInit(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_1)

//     std::string jsonStr;
//     NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, jsonStr);
//     auto jsonDoc = std::make_shared<HJYJsonDocument>();
//     auto config = std::make_shared<ContextInitConfig>(jsonDoc);
//     if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
//         napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
//         return nullptr;
//     }

//     i_err = HJEntryBaseRender::contextInit({
//         .logIsValid = config->valid, .logDir = config->logDir, .logLevel = config->logLevel, .logMode = config->logMode,
//         .logMaxFileSize = config->maxSize, .logMaxFileNum = config->maxFiles
//     });

//     NAPI_INT_RET
// }

napi_value HJNativeExportCommon::setFaceInfo(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_5)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);

    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        size_t nameLen;
        napi_get_value_string_utf8(env, args[INDEX_1], nullptr, 0, &nameLen);
        std::vector<char> nameBuf(nameLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_1], nameBuf.data(), nameBuf.size(), nullptr);
        std::string sourceInsName = std::string(nameBuf.data());

        int32_t w;
        napi_get_value_int32(env, args[INDEX_2], &w);
        int32_t h;
        napi_get_value_int32(env, args[INDEX_3], &h);

        size_t strLen;
        napi_get_value_string_utf8(env, args[INDEX_4], nullptr, 0, &strLen);

        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_4], buffer.data(), buffer.size(), nullptr);
        std::string pointsInfo = std::string(buffer.data());
        
        entryRender->setFaceInfo(sourceInsName, w, h, pointsInfo);
    }

    NAPI_INT_RET    
}
napi_value HJNativeExportCommon::nativeSourceOpen(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        bool bUsePBO = false;
        napi_get_value_bool(env, args[INDEX_1], &bUsePBO);
        i_err = entryRender->openNativeSource(bUsePBO);
    }

    NAPI_INT_RET
}
napi_value HJNativeExportCommon::nativeSourceClose(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        entryRender->closeNativeSource();
    }

    NAPI_INT_RET
}
napi_value HJNativeExportCommon::nativeSourceAcquire(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    std::shared_ptr<HJRGBAMediaData> data = nullptr;
    if (entryRender)
    {
        data = entryRender->acquireNativeSource();
        if (data)
        {
            void* arrayBufferPtr = nullptr;
            napi_value arrayBuffer = nullptr;
            size_t bufferSize = data->m_nSize;
            napi_status status = napi_create_arraybuffer(env, bufferSize, &arrayBufferPtr, &arrayBuffer);
            if (status != napi_ok || arrayBufferPtr == nullptr) 
            {
                napi_throw_error(env, nullptr, "Failed to create ArrayBuffer");
                return nullptr;
            }
            uint8_t *pData = static_cast<uint8_t*>(arrayBufferPtr);
        
            //int64_t t0 = HJCurrentSteadyMS();
            memcpy(pData, data->m_buffer->getBuf(), bufferSize);
            //int64_t t1 = HJCurrentSteadyMS();
            //HJFLogi("memcpy size:{} time is:{} ", bufferSize, (t1 - t0));
                
            napi_value resultObj;
            napi_create_object(env, &resultObj);
        
            napi_value jsWidth, jsHeight, jsScaleWidth, jsScaleHeight;
            napi_create_uint32(env, data->m_width, &jsWidth);
            napi_create_uint32(env, data->m_height, &jsHeight);
            napi_create_uint32(env, data->m_scaleWidth, &jsScaleWidth);
            napi_create_uint32(env, data->m_scaleHeight, &jsScaleHeight);

            napi_set_named_property(env, resultObj, "data", arrayBuffer);
            napi_set_named_property(env, resultObj, "width", jsWidth);
            napi_set_named_property(env, resultObj, "height", jsHeight);
            napi_set_named_property(env, resultObj, "scaleWidth", jsScaleWidth);
            napi_set_named_property(env, resultObj, "scaleHeight", jsScaleHeight);
            return resultObj; 
        }
    }
    return nullptr;
}

napi_value HJNativeExportCommon::openFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
    
    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        size_t strLen;
        napi_get_value_string_utf8(env, args[INDEX_1], nullptr, 0, &strLen);

        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_1], buffer.data(), buffer.size(), nullptr);
        std::string url = std::string(buffer.data());
        i_err = entryRender->openFaceu(url);
    }

    NAPI_INT_RET
}
napi_value HJNativeExportCommon::closeFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)
       
    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        entryRender->closeFaceu();
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::setVideoEncQuantOffset(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
       
    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        int32_t quantoffset = 0;
        napi_get_value_int32(env, args[INDEX_1], &quantoffset);
        entryRender->setVideoEncQuantOffset(quantoffset);
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::setFaceProtected(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
       
    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        bool bFaceProtected = false;
        napi_get_value_bool(env, args[INDEX_1], &bFaceProtected);
        entryRender->setFaceProtected(bFaceProtected);
    }

    NAPI_INT_RET
}
std::pair<std::string, std::string> HJNativeExportCommon::priParseClassAndName(const napi_env &env, napi_value i_classStyle, napi_value i_insName)
{
    std::string classStyle = NapiUtil::parseString(env, i_classStyle);
    std::string insName = NapiUtil::parseString(env, i_insName);
    return std::make_pair(classStyle, insName);
}
napi_value HJNativeExportCommon::nodeEnable(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_5)
       
    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [classStyle, insName] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);    
        bool bEnable = NapiUtil::parseBool(env, args[INDEX_3]);
        std::string info = NapiUtil::parseString(env, args[INDEX_4]);
        i_err = entryRender->nodeEnable(classStyle, insName, bEnable, info);
    }

    NAPI_INT_RET

}
napi_value HJNativeExportCommon::nodeSetParam(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_4)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [classStyle, insName] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        std::string paramInfo = NapiUtil::parseString(env, args[INDEX_3]);
        HJBaseParam::Ptr param = HJBaseParam::Create();
        i_err = priFillNodeParam(param, paramInfo);
        if (i_err >= 0)
        {
            i_err = entryRender->nodeSetParam(classStyle, insName, param);
        }
    }

    NAPI_INT_RET
}
napi_value HJNativeExportCommon::nodeCreate(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        std::string nodeInfo = NapiUtil::parseString(env, args[INDEX_1]);
        i_err = entryRender->nodeCreate(nodeInfo);
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::nodeConnect(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_7)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [srcClass, srcIns] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        auto [dstClass, dstIns] = priParseClassAndName(env, args[INDEX_3], args[INDEX_4]);
        std::string shaderType = NapiUtil::parseString(env, args[INDEX_5]);
        std::string linkInfo = NapiUtil::parseString(env, args[INDEX_6]);
        i_err = entryRender->nodeConnect(srcClass, srcIns, dstClass, dstIns, shaderType, linkInfo);
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::nodeDelete(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_4)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [classStyle, insName] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        bool relink = NapiUtil::parseBool(env, args[INDEX_3]);
        i_err = entryRender->nodeDelete(classStyle, insName, relink);
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::nodeDisconnect(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_6)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [srcClass, srcIns] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        auto [dstClass, dstIns] = priParseClassAndName(env, args[INDEX_3], args[INDEX_4]);
        std::string linkId = NapiUtil::parseString(env, args[INDEX_5]);
        i_err = entryRender->nodeDisconnect(srcClass, srcIns, dstClass, dstIns, linkId);
    }

    NAPI_INT_RET
}

napi_value HJNativeExportCommon::nodeLinkInfoChange(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_6)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [srcClass, srcIns] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        auto [dstClass, dstIns] = priParseClassAndName(env, args[INDEX_3], args[INDEX_4]);
        std::string linkInfo = NapiUtil::parseString(env, args[INDEX_5]);
        i_err = entryRender->nodeLinkInfoChange(srcClass, srcIns, dstClass, dstIns, linkInfo);
    }

    NAPI_INT_RET
}
napi_value HJNativeExportCommon::nodeGetPre(napi_env env, napi_callback_info info)
{
    std::string result = "";
    NAPI_PARSE_ARGS(PARAM_COUNT_3)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [curClass, curIns] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        result = entryRender->nodeGetPre(curClass, curIns);
    }

    NAPI_STR_RET(result)
}

napi_value HJNativeExportCommon::nodeGetNext(napi_env env, napi_callback_info info)
{
    std::string result = "";
    NAPI_PARSE_ARGS(PARAM_COUNT_3)

    int64_t nHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nHandle, &lossless);
    auto entryRender = reinterpret_cast<HJEntryBaseRender *>(nHandle);
    if (entryRender)
    {
        auto [curClass, curIns] = priParseClassAndName(env, args[INDEX_1], args[INDEX_2]);
        result = entryRender->nodeGetNext(curClass, curIns);
    }

    NAPI_STR_RET(result)
}

NS_HJ_END

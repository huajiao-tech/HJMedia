//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJJson.h"
#include "HJFLog.h"
#include "HJException.h"
#if defined(HJ_HAVE_YYJSON)
#include "src/yyjson.h"
#endif

#if defined(HJ_HAVE_SIMDJSON)
#include "simdjson/simdjson.h"
#endif

NS_HJ_BEGIN
//***********************************************************************************//
#if defined(HJ_HAVE_YYJSON)
HJYJsonObject::HJYJsonObject()
{
    
}
HJYJsonObject::HJYJsonObject(const std::string& key, yyjson_val* val)
    : m_key(key)
    , m_rval(val)
{
    
}
HJYJsonObject::HJYJsonObject(const std::string& key, yyjson_mut_val* val, yyjson_mut_doc* doc)
    : m_key(key)
    , m_wval(val)
    , m_mdoc(doc)
{
    if(!m_wval && m_mdoc) {
        m_wval = yyjson_mut_obj(m_mdoc);
    }
}

HJYJsonObject::HJYJsonObject(const std::string& key, HJYJsonObject::Ptr obj)
    : m_key(key)
    , m_mdoc(obj->getMDoc())
{
    if(!m_wval && m_mdoc) {
        m_wval = yyjson_mut_obj(m_mdoc);
//        obj->addObj(this);
    }
}

HJYJsonObject::~HJYJsonObject()
{
    
}

bool HJYJsonObject::isNull() {
    return yyjson_is_null(m_rval);
}
bool HJYJsonObject::isObj() {
    return yyjson_is_obj(m_rval);
}
bool HJYJsonObject::isArr() {
    return yyjson_is_arr(m_rval);
}
bool HJYJsonObject::isStr() {
    return yyjson_is_str(m_rval);
}
bool HJYJsonObject::isInt64() {
    return yyjson_is_int(m_rval);
}
bool HJYJsonObject::isUInt64() {
    return yyjson_is_uint(m_rval);
}
bool HJYJsonObject::isReal() {
    return yyjson_is_real(m_rval);
}
bool HJYJsonObject::isBool() {
    return yyjson_is_bool(m_rval);
}
int HJYJsonObject::getType() {
    return unsafe_yyjson_get_type(m_rval);
}

bool HJYJsonObject::hasObj(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return false;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return false;
    }
    return true;
}

bool HJYJsonObject::isObj(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return false;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return false;
    }
    return yyjson_is_obj(val);
}

bool HJYJsonObject::isArr(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return false;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return false;
    }
    return yyjson_is_arr(val);
}

HJYJsonObject::Ptr HJYJsonObject::getObj(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return nullptr;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return nullptr;
    }
    return std::make_shared<HJYJsonObject>(key, val);
}

int HJYJsonObject::addObj(HJYJsonObject::Ptr obj)
{
    if (!obj || !m_wval || !m_mdoc) {
        return HJErrInvalidParams;
    }
    yyjson_mut_val* val = obj->getWVal();
    if (!val) {
        return HJErrJSONValue;
    }
    bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, obj->getKey().c_str(), val);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    m_childList.push_back(obj);
    return HJ_OK;
}

bool HJYJsonObject::getBool() {
    return yyjson_get_bool(m_rval);
}
int HJYJsonObject::getInt() {
    return yyjson_get_int(m_rval);
}
int64_t HJYJsonObject::getInt64() {
    return yyjson_get_sint(m_rval);
}
uint64_t HJYJsonObject::getUInt64() {
    return yyjson_get_uint(m_rval);
}
double HJYJsonObject::getReal() {
    return yyjson_get_real(m_rval);
}
std::string HJYJsonObject::getString() {
    return yyjson_get_str(m_rval);
}
uint8_t* HJYJsonObject::getRaw() {
    return (uint8_t*)yyjson_get_raw(m_rval);
}

bool HJYJsonObject::getBool(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return false;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return false;
    }
    return yyjson_get_bool(val);
}

int HJYJsonObject::getInt(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return 0;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return 0;
    }
    return yyjson_get_int(val);
}

int64_t HJYJsonObject::getInt64(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return 0;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return 0;
    }
    return yyjson_get_sint(val);
}

uint64_t HJYJsonObject::getUInt64(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return 0;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return 0;
    }
    return yyjson_get_uint(val);
}

double HJYJsonObject::getReal(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return 0.0;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return 0.0;
    }
    return yyjson_get_real(val);
}

std::string HJYJsonObject::getString(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return "";
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return "";
    }
    return yyjson_get_str(val);
}

uint8_t* HJYJsonObject::getRaw(const std::string& key)
{
    if (!m_rval || key.empty()) {
        return NULL;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return NULL;
    }
    return (uint8_t *)yyjson_get_raw(val);
}

std::deque<HJYJsonObject::Ptr> HJYJsonObject::getArr(const std::string& key)
{
    std::deque<HJYJsonObject::Ptr> objs;
    if (!m_rval || key.empty()) {
        return objs;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val || !yyjson_is_arr(val)) {
        return objs;
    }
    size_t idx, max;
    yyjson_val* item = NULL;
    yyjson_arr_foreach(val, idx, max, item) {
        //if (yyjson_is_obj(item) || yyjson_is_arr(item)) {
            HJYJsonObject::Ptr obj = std::make_shared<HJYJsonObject>(key.c_str(), item);
            objs.push_back(obj);
        //}
    }
    return objs;
}

int HJYJsonObject::getMember(const std::string& key, bool& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_bool(val);
    
    return HJ_OK;
}

int HJYJsonObject::getMember(const std::string& key, int& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_int(val);
    
    return HJ_OK;
}
int HJYJsonObject::getMember(const std::string& key, int64_t& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_sint(val);
    
    return HJ_OK;
}
int HJYJsonObject::getMember(const std::string& key, uint64_t& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_uint(val);
    
    return HJ_OK;
}
int HJYJsonObject::getMember(const std::string& key, double& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_real(val);
    
    return HJ_OK;
}
int HJYJsonObject::getMember(const std::string& key, std::string& value)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = yyjson_get_str(val);
    
    return HJ_OK;
}
int HJYJsonObject::getMember(const std::string& key, uint8_t*& value, size_t& len)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val) {
        return HJErrNotExist;
    }
    value = (uint8_t*)yyjson_get_raw(val);
    len = yyjson_get_len(val);
    
    return HJ_OK;
}

int HJYJsonObject::getMember(const std::string& key, std::vector<HJYJsonObject::Ptr>& objs)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val || !yyjson_is_arr(val)) {
        return HJErrNotExist;
    }
    size_t idx, max;
    yyjson_val* item = NULL;
    yyjson_arr_foreach(val, idx, max, item) {
        //if (yyjson_is_obj(item) || yyjson_is_arr(item)) {
            HJYJsonObject::Ptr obj = std::make_shared<HJYJsonObject>(key.c_str(), item);
            objs.push_back(obj);
        //}
    }
    return HJ_OK;
}
int HJYJsonObject::forEachAnonymous(const std::function<int(const HJYJsonObject::Ptr&)>& cb)
{
	if (!m_rval) {
		return HJErrNotAlready;
	}
	
	int res = HJ_OK;
	size_t idx, max;
	yyjson_val* item = NULL;
	yyjson_arr_foreach(m_rval, idx, max, item) {
		//if (yyjson_is_obj(item) || yyjson_is_arr(item)) {
		HJYJsonObject::Ptr obj = std::make_shared<HJYJsonObject>("anonymous", item);
		res = cb(obj);
		if (HJ_OK != res) {
			break;
		}
		//}
	}
	return res;
}
int HJYJsonObject::forEach(const std::string& key, const std::function<int(const HJYJsonObject::Ptr &)>& cb)
{
    if (!m_rval || key.empty()) {
        return HJErrNotAlready;
    }
    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
    if (!val || !yyjson_is_arr(val)) {
        return HJErrNotExist;
    }
    int res = HJ_OK;
    size_t idx, max;
    yyjson_val* item = NULL;
    yyjson_arr_foreach(val, idx, max, item) {
        //if (yyjson_is_obj(item) || yyjson_is_arr(item)) {
            HJYJsonObject::Ptr obj = std::make_shared<HJYJsonObject>(key.c_str(), item);
            res = cb(obj);
            if (HJ_OK != res) {
                break;
            }
        //}
    }
    return res;
}

//template <typename T>
//int HJYJsonObject::getMember(const std::string& key, T& value)
//{
//    if (!m_rval || key.empty()) {
//        return HJErrNotAlready;
//    }
//    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
//    if (!val) {
//        return HJErrNotExist;
//    }
//    if (yyjson_is_bool(val)) {
//        value = yyjson_get_bool(val);
//    } else if (yyjson_is_int(val)) {
//        value = yyjson_get_int(val);
//    } else if (yyjson_is_sint(val)) {
//        value = yyjson_get_sint(val);
//    } else if (yyjson_is_uint(val)) {
//        value = yyjson_get_uint(val);
//    } else if (yyjson_is_real(val)) {
//        value = yyjson_get_real(val);
//    } else if (yyjson_is_str(val)) {
//        value = yyjson_get_str(val);
//    } else {
//        return HJErrNotSupport;
//    }
//    return HJ_OK;
//}

//template <typename T>
//int HJYJsonObject::getMember(const std::string& key, std::deque<T>& value)
//{
//    if (!m_rval || key.empty()) {
//        return HJErrNotAlready;
//    }
//    yyjson_val* val = yyjson_obj_get(m_rval, key.c_str());
//    if (!val) {
//        return HJErrNotExist;
//    }
//    size_t idx, max;
//    yyjson_val* item = NULL;
//    yyjson_arr_foreach(val, idx, max, item) {
//        if (yyjson_is_bool(item)) {
//            value.push_back(yyjson_get_bool(item));
//        } else if (yyjson_is_int(item)) {
//            value.push_back(yyjson_get_int(item));
//        } else if (yyjson_is_sint(item)) {
//            value.push_back(yyjson_get_sint(item));
//        } else if (yyjson_is_uint(item)) {
//            value.push_back(yyjson_get_uint(item));
//        } else if (yyjson_is_real(item)) {
//            value.push_back(yyjson_get_real(item));
//        } else if (yyjson_is_str(item)) {
//            value.push_back(yyjson_get_str(item));
//        } else if (yyjson_is_obj(item) || yyjson_is_arr(item)) {
//            value.push_back(std::make_shared<HJYJsonObject>(key.c_str(), item));
//        }
//    }
//    return HJ_OK;
//}

HJYJsonObject::Ptr HJYJsonObject::setMember(const std::string& key)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return nullptr;
    }
    yyjson_mut_val* val = yyjson_mut_obj_add_obj(m_mdoc, m_wval, key.c_str()); //yyjson_mut_obj(m_mdoc);
    if (!val) {
        return nullptr;
    }
//    bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, key.c_str(), val);
//    if (!ret) {
//        return nullptr;
//    }
    auto subObj = std::make_shared<HJYJsonObject>(key, val, m_mdoc);
    m_subObjs[key] = subObj;
    //
    return subObj;
}

int HJYJsonObject::setMember(const std::string& key, const bool value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVBool>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_bool(m_mdoc, m_wval, subObj->getName().c_str(), value);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const int value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVInt>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_int(m_mdoc, m_wval, subObj->getName().c_str(), value);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const int64_t value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVInt64>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_sint(m_mdoc, m_wval, subObj->getName().c_str(), value);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const uint64_t value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVUInt64>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_uint(m_mdoc, m_wval, subObj->getName().c_str(), value);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const double value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVReal>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_real(m_mdoc, m_wval, subObj->getName().c_str(), value);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const std::string value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVString>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_strn(m_mdoc, m_wval, subObj->getName().c_str(), subObj->get().c_str(), subObj->get().length());
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}
int HJYJsonObject::setMember(const std::string& key, const char* value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVCPtr>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_str(m_mdoc, m_wval, subObj->getName().c_str(), value);
//    bool ret = yyjson_mut_obj_add_strn(m_mdoc, m_wval, subObj->getName().c_str(), value, strlen(value));
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}

int HJYJsonObject::setMember(const std::string& key, const uint8_t* value, const size_t len)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVPtr>(key, value);
    m_subObjs[key] = subObj;
    //
    bool ret = yyjson_mut_obj_add_strn(m_mdoc, m_wval, subObj->getName().c_str(), (char *)value, len);
    if (!ret) {
        return HJErrJSONAddValue;
    }
    return HJ_OK;
}

int HJYJsonObject::setMember(const std::string& key, std::vector<HJYJsonObject::Ptr> value)
{
    if (key.empty() || !m_wval || !m_mdoc) {
        return HJErrNotAlready;
    }
    auto subObj = std::make_shared<HJVObjsVector>(key, value);
    m_subObjs[key] = subObj;
    //
    yyjson_mut_val* arr = yyjson_mut_obj_add_arr(m_mdoc, m_wval, subObj->getName().c_str()); //yyjson_mut_arr(m_mdoc);
    for (auto obj : value) {
        yyjson_mut_val* item = obj->getWVal();
        yyjson_mut_arr_add_val(arr, item);
    }
//    bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), arr);
//    if (!ret) {
//        return HJErrJSONAddValue;
//    }
    return HJ_OK;
}

int HJYJsonObject::setMember(const std::string& key, const std::vector<std::string>& value)
{
    int i_err = HJ_OK;
    do
    {
        if (key.empty() || !m_wval || !m_mdoc) {
            i_err = HJErrNotAlready;
            break;
        }
        auto subObj = std::make_shared<HJVStrArray>(key, value);
        m_subObjs[key] = subObj;

        int size = value.size();
        std::vector<const char*> cStyleStrings;
        cStyleStrings.reserve(size); 
        for (const auto& str : value) {
            cStyleStrings.push_back(str.c_str());
        }

        yyjson_mut_val* val = yyjson_mut_arr_with_str(m_mdoc, cStyleStrings.data(), size);
        bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), val); //not use yyjson_mut_arr_add_val
        if (!ret)
        {
            i_err = HJErrJSONAddValue;
            break;
        }
    } while (false);
    return i_err;
}
int HJYJsonObject::setMember(const std::string& key, const std::vector<int>& value)
{
    int i_err = HJ_OK;
    do
    {
        if (key.empty() || !m_wval || !m_mdoc) {
            i_err = HJErrNotAlready;
            break;
        }
        auto subObj = std::make_shared<HJVIntArray>(key, value);
        m_subObjs[key] = subObj;

        int size = value.size();
        yyjson_mut_val* val = yyjson_mut_arr_with_sint32(m_mdoc, value.data(), size);
        bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), val); //not use yyjson_mut_arr_add_val
        if (!ret)
        {
            i_err = HJErrJSONAddValue;
            break;
        }
    } while (false);
    return i_err;
}
int HJYJsonObject::setMember(const std::string& key, const std::vector<int64_t>& value)
{
    int i_err = HJ_OK;
    do
    {
        if (key.empty() || !m_wval || !m_mdoc) {
            i_err = HJErrNotAlready;
            break;
        }
        auto subObj = std::make_shared<HJVInt64Array>(key, value);
        m_subObjs[key] = subObj;

        int size = value.size();
        yyjson_mut_val* val = yyjson_mut_arr_with_sint(m_mdoc, value.data(), size);
        bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), val); //not use yyjson_mut_arr_add_val
        if (!ret)
        {
            i_err = HJErrJSONAddValue;
            break;
        }
    } while (false);
    return i_err;
}
int HJYJsonObject::setMember(const std::string& key, const std::vector<bool>& value)
{
    int i_err = HJ_OK;
    bool *pBool = nullptr;
    do
    {
        if (key.empty() || !m_wval || !m_mdoc) {
            i_err = HJErrNotAlready;
            break;
        }
        auto subObj = std::make_shared<HJVBoolArray>(key, value);
        m_subObjs[key] = subObj;

        int size = value.size();
        pBool = new bool[size];
        for (int i = 0; i < size; i++)
        {
            pBool[i] = value[i];
        }

        yyjson_mut_val* val = yyjson_mut_arr_with_bool(m_mdoc, pBool, size);
        bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), val); //not use yyjson_mut_arr_add_val
        if (!ret)
        {
            i_err = HJErrJSONAddValue;
            break;
        }
    } while (false);
    if (pBool)
    {
        delete[] pBool;
    }
    return i_err;
}
int HJYJsonObject::setMember(const std::string& key, const std::vector<double>& value)
{
    int i_err = HJ_OK;
    do
    {
        if (key.empty() || !m_wval || !m_mdoc) {
            i_err = HJErrNotAlready;
            break;
        }
        auto subObj = std::make_shared<HJVRealArray>(key, value);
        m_subObjs[key] = subObj;

        int size = value.size();
        yyjson_mut_val* val = yyjson_mut_arr_with_real(m_mdoc, value.data(), size);
        bool ret = yyjson_mut_obj_add_val(m_mdoc, m_wval, subObj->getName().c_str(), val); //not use yyjson_mut_arr_add_val
        if (!ret)
        {
            i_err = HJErrJSONAddValue;
            break;
        }
    } while (false);
    return i_err;
}

//***********************************************************************************//
HJYJsonDocument::HJYJsonDocument()
{
    
}

HJYJsonDocument::~HJYJsonDocument()
{
    if (m_rdoc) {
        yyjson_doc_free(m_rdoc);
        m_rdoc = NULL;
    }
    if (m_wdoc) {
        yyjson_mut_doc_free(m_wdoc);
        m_wdoc = NULL;
    }
}

int HJYJsonDocument::init()
{
    int res = HJ_OK;
    do {
        m_wdoc = yyjson_mut_doc_new(NULL);
        if (!m_wdoc) {
            res = HJErrJSONRead;
            break;
        }
        m_wroot = yyjson_mut_obj(m_wdoc);
        if (!m_wroot) {
            res = HJErrJSONValue;
            break;
        }
        yyjson_mut_doc_set_root(m_wdoc, m_wroot);
        //
        setMDoc(m_wdoc);
        setWVal(m_wroot);
    } while (false);
    
    return res;
}

int HJYJsonDocument::init(const std::string& info)
{
    //HJLogi("init entry");
    int res = HJ_OK;
    try
    {
        do {
            m_rdoc = yyjson_read(info.c_str(), info.size(), YYJSON_READ_ALLOW_BOM  | YYJSON_READ_ALLOW_TRAILING_COMMAS);
            if (!m_rdoc) {
                res = HJErrJSONRead;
                break;
            }
            m_rroot = yyjson_doc_get_root(m_rdoc);
            if (!m_rroot) {
                res = HJErrJSONValue;
                break;
            }
            setRVal(m_rroot);
        } while (false);
    } catch (const HJException& e)
    {
        HJLoge("error, standard exception :" + e.getFullDescription());
        res = HJErrFatal;
    } catch (...)
    {
        HJLoge("error, unkown exception");
        res = HJErrFatal;
    }
    //HJFLogi("init entry, res:{}", res);

    return res;
}

int HJYJsonDocument::initWithUrl(const std::string& url)
{
    int res = HJ_OK;
    do {
        yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
        yyjson_read_err err;
        m_rdoc = yyjson_read_file(url.c_str(), flg, NULL, &err);
        if (!m_rdoc || YYJSON_READ_SUCCESS != err.code) {
            res = HJErrJSONRead;
            break;
        }
        m_rroot = yyjson_doc_get_root(m_rdoc);
        if (!m_rroot) {
            res = HJErrJSONValue;
            break;
        }
        setRVal(m_rroot);
    } while (false);
    
    return res;
}

std::string HJYJsonDocument::getSerialInfo()
{
    if (!m_wdoc) {
        return "";
    }
    size_t len = 0;
    yyjson_write_flag flg = YYJSON_WRITE_NOFLAG;//YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
    const char *json = yyjson_mut_write(m_wdoc, flg, &len);
    if (!json) {
        return "";
    }
    std::string jstr = std::string(json, len);
    free((void *)json);
    return jstr;
}

int HJYJsonDocument::writeFile(const std::string &path)
{
    if (path.empty()) {
        return HJErrInvalidParams;
    }
    yyjson_write_err err;
    yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
    yyjson_mut_write_file(path.c_str(), m_wdoc, flg, NULL, &err);
    if (err.code) {
        HJLogi("write json error:" + HJ2STR(err.code) + HJ2SSTR(err.msg));
        return HJErrJSONWrite;
    }
    return HJ_OK;
}

#endif
//***********************************************************************************//
#if defined(HJ_HAVE_SIMDJSON)
int HJSJsonDocument::init(const std::string& info)
{
    int res = HJ_OK;
    m_parser = std::make_shared<simdjson::ondemand::parser>();
    m_doc = std::make_shared<simdjson::ondemand::document>();
    auto error = m_parser->iterate(info).get(*m_doc);
    if (error) {
        return HJErrFatal;
    }
    simdjson::ondemand::object obj;
    simdjson::ondemand::array array;

    return res;
}

int HJSJsonDocument::initWithUrl(const std::string& url)
{
    int res = HJ_OK;
    
    simdjson::padded_string json;
    auto error = simdjson::padded_string::load(url).get(json);
    if (error) {
        return HJErrIO;
    }
    m_parser = std::make_shared<simdjson::ondemand::parser>();
    m_doc = std::make_shared<simdjson::ondemand::document>();
    error = m_parser->iterate(json).get(*m_doc);
    if (error) {
        return HJErrFatal;
    }
    
    return res;
}
#endif //


NS_HJ_END

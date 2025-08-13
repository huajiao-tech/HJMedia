//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <any>
#include "HJCommons.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJKeyStorage : public std::unordered_map<std::string, std::any>
{
public:
    using Ptr = std::shared_ptr<HJKeyStorage>;

    const std::any* getStorage(const std::string& key) {
        auto it = find(key);
        if (it != end()) {
            return &it->second;
        }
        return nullptr;
    }
    int addStorage(const std::string& key, const std::any anyObj) {
        if (haveStorage(key)) {
            return HJErrAlreadyExist;
        }
        insert({key, anyObj});
        
        return HJ_OK;
    }
    bool haveStorage(const std::string& key) {
        auto it = find(key);
        if (it != end()) {
            return true;
        }
        return false;
    }
    template<typename T>
    T getStorageValue(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if(anyObj) {
            return std::any_cast<T>(*anyObj);
        }
        return (T)0;
    }
    //
    bool haveValue(const std::string& key) {
        auto it = find(key);
        if (it != end()) {
            return true;
        }
        return false;
    }
    template<typename T>
    T getValue(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<T>(*anyObj);
        }
        return (T)0;
    }
    bool getBool(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<bool>(*anyObj);
        }
        return false;
    }
    int getInt(const std::string& key) {
        return getValue<int>(key);
    }
    uint32_t getUInt(const std::string& key) {
        return getValue<uint32_t>(key);
    }
    int64_t getInt64(const std::string& key) {
        return getValue<int64_t>(key);
    }
    uint64_t getUInt64(const std::string& key) {
        return getValue<uint64_t>(key);
    }
    float getFloat(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<float>(*anyObj);
        }
        return 0.0f;
    }
    double getDouble(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<double>(*anyObj);
        }
        return 0.0;
    }
    std::string getString(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<std::string>(*anyObj);
        }
        return "";
    }
    const char* getCharPtr(const std::string& key) {
        const std::any* anyObj = getStorage(key);
        if (anyObj) {
            return std::any_cast<const char *>(*anyObj);
        }
        return "";
    }
    virtual void clone(const HJKeyStorage::Ptr& other) {
        //for (auto& it = other->begin(); it != other->end(); it++) {
        //    std::string key = it->first;
        //}
        insert(other->begin(), other->end());
    }
    virtual void pushOrder(const std::string& key) {
        if (std::find(m_keyOrder.begin(), m_keyOrder.end(), key) != m_keyOrder.end()) {
            return;
        }
        m_keyOrder.push_back(key);
    }
    std::list<std::string>& getOrder() {
        return m_keyOrder;
    }
    void for_each(const std::function<void(const std::string& key)>& cb) {
        for (const auto& it : m_keyOrder) {
            cb(it);
        }
    }
private:
    std::list<std::string>    m_keyOrder;
};
using HJSettings = HJKeyStorage;
using HJOptions = HJKeyStorage;
using HJDictionary = std::unordered_map<std::string, std::any>;
using HJDictionaryPtr = std::shared_ptr<HJDictionary>;

//
class HJAnyVector : public std::vector<std::any> {
public:
    using Ptr = std::shared_ptr<HJAnyVector>;
    HJAnyVector() = default;
    ~HJAnyVector() = default;
};
using HJVector = std::vector<std::any>;
#define HJANYCAST  std::any_cast

NS_HJ_END
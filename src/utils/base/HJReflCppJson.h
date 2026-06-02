#pragma once

#include "HJJson.h"
#include "HJComUtils.h"
#include "HJReflCpp.h"
#include <variant>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <vector>

#if defined(HJ_HAVE_YYJSON)
    #include "src/yyjson.h"
#endif

NS_HJ_BEGIN

#if defined(HJ_HAVE_YYJSON)

namespace refljson_detail {

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {
    using value_type = T;
};

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
    using value_type = T;
};

template <typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
struct is_string : std::is_same<std::decay_t<T>, std::string> {};

template <typename T>
inline constexpr bool is_string_v = is_string<T>::value;

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T>
using decay_t = std::decay_t<T>;

template <typename Elem>
int read_array_item(const HJYJsonObject::Ptr& item, Elem& value)
{
    if constexpr (std::is_same_v<Elem, bool>) {
        value = item->getBool();
    } else if constexpr (std::is_same_v<Elem, int>) {
        value = item->getInt();
    } else if constexpr (std::is_same_v<Elem, short>) {
        value = static_cast<short>(item->getInt());
    } else if constexpr (std::is_same_v<Elem, int64_t>) {
        value = item->getInt64();
    } else if constexpr (std::is_same_v<Elem, uint64_t>) {
        value = item->getUInt64();
    } else if constexpr (std::is_same_v<Elem, float>) {
        value = static_cast<float>(item->getReal());
    } else if constexpr (std::is_same_v<Elem, double>) {
        value = item->getReal();
    } else if constexpr (is_string_v<Elem>) {
        value = item->getString();
    } else {
        return HJErrNotSupport;
    }
    return HJ_OK;
}

template <typename T>
int deserialize_object(const HJYJsonObject::Ptr& obj, T& value);

template <typename T>
int serialize_object(const HJYJsonObject::Ptr& obj, const T& value);

template <typename T>
int read_array_value(const HJYJsonObject::Ptr& obj, T& value)
{
    if (!obj) {
        return HJErrInvalidParams;
    }

    if constexpr (is_vector_v<T>) {
        using Elem = typename is_vector<T>::value_type;
        value.clear();
        int i_err = obj->forEachAnonymous([&](const HJYJsonObject::Ptr& item) -> int {
            if constexpr (is_shared_ptr_v<Elem>) {
                using Pointee = typename is_shared_ptr<Elem>::value_type;
                auto ptr = std::make_shared<Pointee>();
                int ret = HJ_OK;
                if constexpr (refl::trait::is_reflectable_v<Pointee>) {
                    ret = deserialize_object(item, *ptr);
                } else if constexpr (std::is_base_of_v<HJInterpreter, Pointee>) {
                    ret = ptr->deserialInfo(item);
                } else {
                    return HJErrNotSupport;
                }
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(ptr));
                return HJ_OK;
            } else if constexpr (refl::trait::is_reflectable_v<Elem>) {
                Elem elem{};
                int ret = deserialize_object(item, elem);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            } else if constexpr (std::is_base_of_v<HJInterpreter, Elem>) {
                Elem elem;
                int ret = elem.deserialInfo(item);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            } else {
                Elem elem{};
                int ret = read_array_item(item, elem);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            }
        });
        if (i_err == HJErrNotExist) {
            return HJErrNotExist;
        }
        return i_err;
    } else {
        return HJErrNotSupport;
    }
}

template <typename T>
int deserialize_object_from_array(const HJYJsonObject::Ptr& obj, T& value)
{
    if (!obj || !obj->isArr()) {
        return HJErrInvalidParams;
    }

    int i_err = HJ_OK;
    int vector_fields = 0;
    refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
        if (i_err < 0) {
            return;
        }
        if constexpr (!refl::descriptor::is_field(member)) {
            return;
        }
        auto& field = member(value);
        using FieldT = decay_t<decltype(field)>;
        if constexpr (is_vector_v<FieldT>) {
            vector_fields += 1;
            if (vector_fields > 1) {
                i_err = HJErrNotSupport;
                return;
            }
            int ret = read_array_value<FieldT>(obj, field);
            if (ret < 0) {
                i_err = ret;
            }
        }
    });
    if (vector_fields == 0) {
        return HJErrNotSupport;
    }
    return i_err;
}

template <typename T>
int read_value(const HJYJsonObject::Ptr& obj, const std::string& key, T& value)
{
    if (!obj) {
        return HJErrInvalidParams;
    }

    if constexpr (is_vector_v<T>) {
        using Elem = typename is_vector<T>::value_type;
        value.clear();
        int i_err = obj->forEach(key, [&](const HJYJsonObject::Ptr& item) -> int {
            if constexpr (is_shared_ptr_v<Elem>) {
                using Pointee = typename is_shared_ptr<Elem>::value_type;
                auto ptr = std::make_shared<Pointee>();
                int ret = HJ_OK;
                if constexpr (refl::trait::is_reflectable_v<Pointee>) {
                    ret = deserialize_object(item, *ptr);
                } else if constexpr (std::is_base_of_v<HJInterpreter, Pointee>) {
                    ret = ptr->deserialInfo(item);
                } else {
                    return HJErrNotSupport;
                }
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(ptr));
                return HJ_OK;
            } else if constexpr (refl::trait::is_reflectable_v<Elem>) {
                Elem elem{};
                int ret = deserialize_object(item, elem);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            } else if constexpr (std::is_base_of_v<HJInterpreter, Elem>) {
                Elem elem;
                int ret = elem.deserialInfo(item);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            } else {
                Elem elem{};
                int ret = read_array_item(item, elem);
                if (ret < 0) {
                    return ret;
                }
                value.push_back(std::move(elem));
                return HJ_OK;
            }
        });
        if (i_err == HJErrNotExist) {
            return HJErrNotExist;
        }
        return i_err;
    } else if constexpr (is_string_v<T>) {
        return obj->getMember(key, value);
    } else if constexpr (std::is_same_v<T, bool> ||
                         std::is_same_v<T, int> ||
                         std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, uint64_t> ||
                         std::is_same_v<T, double>) {
        return obj->getMember(key, value);
    } else if constexpr (std::is_same_v<T, float>) {
        double tmp = 0.0;
        int ret = obj->getMember(key, tmp);
        if (ret < 0) {
            return ret;
        }
        value = static_cast<float>(tmp);
        return HJ_OK;
    } else if constexpr (std::is_same_v<T, short>) {
        int tmp = 0;
        int ret = obj->getMember(key, tmp);
        if (ret < 0) {
            return ret;
        }
        value = static_cast<short>(tmp);
        return HJ_OK;
    } else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>) {
        uint64_t tmp = 0;
        int ret = obj->getMember(key, tmp);
        if (ret < 0) {
            return ret;
        }
        value = static_cast<T>(tmp);
        return HJ_OK;
    } else if constexpr (refl::trait::is_reflectable_v<T>) {
        auto subObj = obj->getObj(key);
        if (!subObj) {
            return HJErrNotExist;
        }
        return deserialize_object(subObj, value);
    } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
        auto subObj = obj->getObj(key);
        if (!subObj) {
            return HJErrNotExist;
        }
        return value.deserialInfo(subObj);
    } else {
        return HJErrNotSupport;
    }
}

template <typename T>
int write_value(const HJYJsonObject::Ptr& obj, const std::string& key, const T& value)
{
    if (!obj) {
        return HJErrInvalidParams;
    }

    if constexpr (is_vector_v<T>) {
        using Elem = typename is_vector<T>::value_type;
        if constexpr (std::is_same_v<Elem, std::string> ||
                      std::is_same_v<Elem, int> ||
                      std::is_same_v<Elem, int64_t> ||
                      std::is_same_v<Elem, bool> ||
                      std::is_same_v<Elem, double>) {
            return obj->setMember(key, value);
        } else if constexpr (std::is_same_v<Elem, float>) {
            std::vector<double> tmp;
            tmp.reserve(value.size());
            for (auto& v : value) {
                tmp.push_back(static_cast<double>(v));
            }
            return obj->setMember(key, tmp);
        } else if constexpr (is_shared_ptr_v<Elem>) {
            using Pointee = typename is_shared_ptr<Elem>::value_type;
            std::vector<HJYJsonObject::Ptr> items;
            items.reserve(value.size());
            for (auto& elem : value) {
                if (!elem) {
                    continue;
                }
                auto itemObj = std::make_shared<HJYJsonObject>(key + "_item", obj);
                int ret = HJ_OK;
                if constexpr (refl::trait::is_reflectable_v<Pointee>) {
                    ret = serialize_object(itemObj, *elem);
                } else if constexpr (std::is_base_of_v<HJInterpreter, Pointee>) {
                    ret = elem->serialInfo(itemObj);
                } else {
                    return HJErrNotSupport;
                }
                if (ret < 0) {
                    return ret;
                }
                items.push_back(std::move(itemObj));
            }
            return obj->setMember(key, items);
        } else if constexpr (refl::trait::is_reflectable_v<Elem> || std::is_base_of_v<HJInterpreter, Elem>) {
            std::vector<HJYJsonObject::Ptr> items;
            items.reserve(value.size());
            for (auto& elem : value) {
                auto itemObj = std::make_shared<HJYJsonObject>(key + "_item", obj);
                int ret = HJ_OK;
                if constexpr (refl::trait::is_reflectable_v<Elem>) {
                    ret = serialize_object(itemObj, elem);
                } else {
                    ret = elem.serialInfo(itemObj);
                }
                if (ret < 0) {
                    return ret;
                }
                items.push_back(std::move(itemObj));
            }
            return obj->setMember(key, items);
        } else {
            return HJErrNotSupport;
        }
    } else if constexpr (is_string_v<T>) {
        return obj->setMember(key, value);
    } else if constexpr (std::is_same_v<T, bool> ||
                         std::is_same_v<T, int> ||
                         std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, uint64_t> ||
                         std::is_same_v<T, double>) {
        return obj->setMember(key, value);
    } else if constexpr (std::is_same_v<T, float>) {
        return obj->setMember(key, static_cast<double>(value));
    } else if constexpr (std::is_same_v<T, short>) {
        return obj->setMember(key, static_cast<int>(value));
    } else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>) {
        return obj->setMember(key, static_cast<uint64_t>(value));
    } else if constexpr (refl::trait::is_reflectable_v<T>) {
        auto subObj = std::make_shared<HJYJsonObject>(key, obj);
        if (!subObj) {
            return HJErrInvalidParams;
        }
        int ret = obj->addObj(subObj);
        if (ret < 0) {
            return ret;
        }
        return serialize_object(subObj, value);
    } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
        auto subObj = std::make_shared<HJYJsonObject>(key, obj);
        if (!subObj) {
            return HJErrInvalidParams;
        }
        int ret = obj->addObj(subObj);
        if (ret < 0) {
            return ret;
        }
        return value.serialInfo(subObj);
    } else {
        return HJErrNotSupport;
    }
}

template <typename T>
int deserialize_object(const HJYJsonObject::Ptr& obj, T& value)
{
    if (!obj) {
        return HJErrInvalidParams;
    }
    if (obj->isArr()) {
        return deserialize_object_from_array(obj, value);
    }
    int i_err = HJ_OK;
    refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
        if (i_err < 0) {
            return;
        }
        if constexpr (!refl::descriptor::is_field(member)) {
            return;
        }
        auto& field = member(value);
        using FieldT = decay_t<decltype(field)>;
        const std::string key = member.name.str();
        int ret = read_value<FieldT>(obj, key, field);
        if (ret == HJErrNotExist) {
            return;
        }
        if (ret < 0) {
            i_err = ret;
        }
    });
    return i_err;
}

template <typename T>
int serialize_object(const HJYJsonObject::Ptr& obj, const T& value)
{
    if (!obj) {
        return HJErrInvalidParams;
    }
    int i_err = HJ_OK;
    refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
        if (i_err < 0) {
            return;
        }
        if constexpr (!refl::descriptor::is_field(member)) {
            return;
        }
        const auto& field = member(value);
        using FieldT = decay_t<decltype(field)>;
        const std::string key = member.name.str();
        int ret = write_value<FieldT>(obj, key, field);
        if (ret < 0) {
            i_err = ret;
        }
    });
    return i_err;
}

} // namespace refljson_detail

class HJReflCppJson
{
public:
    template <typename T>
    static int deserial(const HJYJsonObject::Ptr& obj, T& value)
    {
        if constexpr (refl::trait::is_reflectable_v<T>) {
            return refljson_detail::deserialize_object(obj, value);
        } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
            return value.deserialInfo(obj);
        } else {
            return HJErrNotSupport;
        }
    }

    template <typename T>
    static int serial(const HJYJsonObject::Ptr& obj, const T& value)
    {
        if constexpr (refl::trait::is_reflectable_v<T>) {
            return refljson_detail::serialize_object(obj, value);
        } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
            return value.serialInfo(obj);
        } else {
            return HJErrNotSupport;
        }
    }
};

template <typename T>
class HJReflCppJsonInterpreter : public HJInterpreter
{
private:
    using HJInterpreter::init;
    using HJInterpreter::initWithUrl;

public:
    using HJInterpreter::HJInterpreter;

    int deserial(const std::string& info)
    {
        if (info.empty()) {
            return HJErrInvalidParams;
        }
        auto doc = HJYJsonDocument::createWithInfo(info);
        if (!doc) {
            return HJErrInvalidParams;
        }
        m_obj = doc;
        return deserialInfo(m_obj);
    }

    std::string serial()
    {
        auto doc = HJYJsonDocument::create();
        if (!doc) {
            return "";
        }
        m_obj = doc;
        HJReflCppJson::serial(doc, static_cast<const T&>(*this));
        std::string res = doc->getSerialInfo();
        if (!res.empty()) {
            return res;
        }
        if (doc->getWVal()) {
            size_t len = 0;
            char* json = yyjson_mut_val_write(doc->getWVal(), YYJSON_WRITE_NOFLAG, &len);
            if (json) {
                std::string valRes(json, len);
                free(json);
                return valRes;
            }
        }
        return res;
    }

    int deserialInfo(const HJYJsonObject::Ptr& obj = nullptr) override
    {
        auto target = obj ? obj : m_obj;
        if (!target) {
            return HJErrInvalidParams;
        }
        return HJReflCppJson::deserial(target, static_cast<T&>(*this));
    }

    int serialInfo(const HJYJsonObject::Ptr& obj = nullptr) override
    {
        auto target = obj ? obj : m_obj;
        if (!target) {
            return HJErrInvalidParams;
        }
        return HJReflCppJson::serial(target, static_cast<const T&>(*this));
    }
};

#endif // defined(HJ_HAVE_YYJSON)

NS_HJ_END

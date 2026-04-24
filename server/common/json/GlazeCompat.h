#pragma once

#ifndef MEMOCHAT_JSON_GLAZECOMPAT_INCLUDED
#define MEMOCHAT_JSON_GLAZECOMPAT_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <glaze/json/generic.hpp>
#include <bit>
#include <glaze/json/write.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <array>
#include <functional>
#include <sstream>
#include <iterator>
#include <algorithm>

namespace memochat::json {

using array_t = glz::generic_json<>::array_t;
using object_t = glz::generic_json<>::object_t;

class JsonValue;

class JsonMemberRef {
    glz::generic_json<> val_;
public:
    JsonMemberRef() = default;
    explicit JsonMemberRef(const glz::generic_json<>& v) : val_(v) {}

    std::string asString() const {
        if (val_.is_string()) return val_.get<std::string>();
        return "";
    }

    int asInt() const { return val_.as<int>(); }
    int64_t asInt64() const { return val_.as<int64_t>(); }
    double asDouble() const { return val_.as<double>(); }
    bool asBool() const { return val_.as<bool>(); }

    bool isNull() const { return val_.holds<std::nullptr_t>(); }
    bool isBool() const { return val_.holds<bool>(); }
    bool isObject() const { return val_.holds<object_t>(); }
    bool isArray() const { return val_.holds<array_t>(); }
    bool isString() const { return val_.holds<std::string>(); }
    bool isNumber() const { return val_.holds<double>(); }
    bool empty() const { return val_.empty(); }
    bool isMember(const std::string& key) const {
        if (!val_.holds<object_t>()) return false;
        return val_.get<object_t>().find(key) != val_.get<object_t>().end();
    }
    bool isMember(const char* key) const { return isMember(std::string(key)); }

    std::string toStyledString() const {
        auto result = val_.dump();
        return result.has_value() ? *result : "{}";
    }

    JsonValue asValue() const;

    glz::generic_json<>& impl() { return val_; }
    const glz::generic_json<>& impl() const { return val_; }

    template<class F>
    auto and_then(F&& f) const -> decltype(auto) {
        auto result = glz::write_json(val_);
        if (result) {
            return std::invoke(std::forward<F>(f), *result);
        }
        return std::invoke(std::forward<F>(f), std::string{});
    }

    template<class F>
    auto value_or(F&& f) const -> decltype(auto) {
        auto result = glz::write_json(val_);
        if (result) return *result;
        return std::forward<F>(f)();
    }

    JsonMemberRef operator[](const std::string& key) const {
        if (val_.holds<object_t>()) {
            auto& obj = val_.get<object_t>();
            auto it = obj.find(key);
            if (it != obj.end()) {
                return JsonMemberRef(it->second);
            }
        }
        return JsonMemberRef{};
    }

    JsonMemberRef operator[](const char* key) const {
        return (*this)[std::string(key)];
    }

    JsonMemberRef operator[](int index) const {
        if (val_.holds<array_t>()) {
            auto& arr = val_.get<array_t>();
            if (index >= 0 && static_cast<size_t>(index) < arr.size()) {
                return JsonMemberRef(arr[static_cast<size_t>(index)]);
            }
        }
        return JsonMemberRef{};
    }
};

class JsonValueProxy {
    mutable glz::generic_json<>* parent_;
    std::string key_;
    int index_;
    bool is_array_access_;
public:
    JsonValueProxy(glz::generic_json<>* parent, const std::string& key)
        : parent_(parent), key_(key), index_(-1), is_array_access_(false) {}
    JsonValueProxy(glz::generic_json<>* parent, int index)
        : parent_(parent), index_(index), is_array_access_(true) {}

    template<typename T>
    T as() const {
        if (!parent_) return T{};
        if constexpr (std::is_same_v<T, std::string>) {
            if (parent_->holds<std::string>()) return parent_->get<std::string>();
            return "";
        } else if constexpr (std::is_same_v<T, int>) {
            return parent_->as<int>();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return parent_->as<int64_t>();
        } else if constexpr (std::is_same_v<T, double>) {
            return parent_->as<double>();
        } else if constexpr (std::is_same_v<T, bool>) {
            return parent_->as<bool>();
        }
        return T{};
    }

    int asInt() const { return as<int>(); }
    int64_t asInt64() const { return as<int64_t>(); }
    double asDouble() const { return as<double>(); }
    bool asBool() const { return as<bool>(); }
    std::string asString() const { return as<std::string>(); }

    template<typename T>
    T get() const {
        if (!parent_) return T{};
        if constexpr (std::is_same_v<T, std::string>) {
            if (parent_->holds<std::string>()) return parent_->get<std::string>();
            return "";
        } else if constexpr (std::is_same_v<T, int>) {
            return parent_->as<int>();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return parent_->as<int64_t>();
        } else if constexpr (std::is_same_v<T, double>) {
            return parent_->as<double>();
        } else if constexpr (std::is_same_v<T, bool>) {
            return parent_->as<bool>();
        } else if constexpr (std::is_same_v<T, array_t>) {
            if (parent_->holds<array_t>()) return parent_->get<array_t>();
            return array_t{};
        } else if constexpr (std::is_same_v<T, object_t>) {
            if (parent_->holds<object_t>()) return parent_->get<object_t>();
            return object_t{};
        } else if constexpr (std::is_same_v<T, glz::generic_json<>>) {
            return *parent_;
        } else if constexpr (std::is_same_v<T, JsonValue>) {
            return JsonValue(*parent_);
        } else if constexpr (std::is_same_v<T, JsonMemberRef>) {
            return JsonMemberRef(*parent_);
        }
        return T{};
    }

    bool isArray() const { return parent_ && parent_->holds<array_t>(); }
    bool is_array() const { return isArray(); }
    bool isObject() const { return parent_ && parent_->holds<object_t>(); }
    bool is_object() const { return isObject(); }
    bool isString() const { return parent_ && parent_->holds<std::string>(); }
    bool isNumber() const { return parent_ && parent_->holds<double>(); }
    bool isBool() const { return parent_ && parent_->holds<bool>(); }
    bool isNull() const { return parent_ && parent_->holds<std::nullptr_t>(); }

    glz::generic_json<>* impl_ptr() const { return parent_; }

    bool holds_object() const { return parent_ && parent_->holds<object_t>(); }
    bool holds_array() const { return parent_ && parent_->holds<array_t>(); }

    JsonMemberRef get(int index) const {
        if (parent_ && parent_->holds<array_t>()) {
            auto& arr = parent_->get<array_t>();
            if (index >= 0 && static_cast<size_t>(index) < arr.size()) {
                return JsonMemberRef(arr[static_cast<size_t>(index)]);
            }
        }
        return JsonMemberRef{};
    }

    template<typename T>
    JsonValueProxy& operator=(T&& val) {
        if (!parent_) return *this;
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            *parent_ = std::forward<T>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, const char*>) {
            *parent_ = std::string(val);
        } else if constexpr (std::is_enum_v<std::decay_t<T>>) {
            *parent_ = static_cast<int64_t>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, int>) {
            *parent_ = val;
        } else if constexpr (std::is_same_v<std::decay_t<T>, short>) {
            *parent_ = static_cast<int>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
            *parent_ = val;
        } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
            *parent_ = val;
        } else if constexpr (std::is_same_v<std::decay_t<T>, float>) {
            *parent_ = static_cast<double>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            *parent_ = val;
        } else if constexpr (std::is_same_v<std::decay_t<T>, JsonValue>) {
            *parent_ = val.impl();
        } else if constexpr (std::is_same_v<std::decay_t<T>, glz::generic_json<>>) {
            *parent_ = std::forward<T>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, array_t>) {
            *parent_ = std::forward<T>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, object_t>) {
            *parent_ = std::forward<T>(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, JsonMemberRef>) {
            *parent_ = val.impl();
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type");
        }
        return *this;
    }

    JsonValueProxy& operator=(const char* val) {
        if (parent_) *parent_ = std::string(val);
        return *this;
    }

    template<typename T>
    void append(T&& item) const {
        if (parent_ && parent_->holds<array_t>()) {
            if constexpr (std::is_same_v<std::decay_t<T>, JsonValue>) {
                parent_->get<array_t>().push_back(item.impl());
            } else {
                parent_->get<array_t>().push_back(glz::generic_json<>(std::forward<T>(item)));
            }
        }
    }

    // Explicit overloads for common types to avoid ambiguity
    void append(int item) const {
        if (parent_ && parent_->holds<array_t>()) {
            parent_->get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
    void append(int64_t item) const {
        if (parent_ && parent_->holds<array_t>()) {
            parent_->get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
    void append(double item) const {
        if (parent_ && parent_->holds<array_t>()) {
            parent_->get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
    void append(const std::string& item) const {
        if (parent_ && parent_->holds<array_t>()) {
            parent_->get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
    void append(const char* item) const {
        if (parent_ && parent_->holds<array_t>()) {
            parent_->get<array_t>().push_back(glz::generic_json<>(std::string(item)));
        }
    }
    void append(const JsonValue& item) const;
    void append(JsonValue&& item) const;

    class ArrayIterator {
        glz::generic_json<>* parent_;
        size_t idx_;
    public:
        ArrayIterator(glz::generic_json<>* p, size_t i) : parent_(p), idx_(i) {}
        JsonMemberRef operator*() const {
            if (parent_ && parent_->holds<array_t>()) {
                auto& arr = parent_->get<array_t>();
                if (idx_ < arr.size()) {
                    return JsonMemberRef(arr[idx_]);
                }
            }
            return JsonMemberRef{};
        }
        ArrayIterator& operator++() { ++idx_; return *this; }
        bool operator!=(const ArrayIterator& other) const { return idx_ != other.idx_; }
    };

    ArrayIterator begin() const {
        if (parent_ && parent_->holds<array_t>()) {
            return ArrayIterator(parent_, 0);
        }
        return ArrayIterator(nullptr, 0);
    }
    ArrayIterator end() const {
        if (parent_ && parent_->holds<array_t>()) {
            return ArrayIterator(parent_, parent_->get<array_t>().size());
        }
        return ArrayIterator(nullptr, 0);
    }

    JsonValueProxy operator[](const std::string& key) {
        if (parent_) return JsonValueProxy(parent_, key);
        return JsonValueProxy(nullptr, std::string{});
    }
    JsonValueProxy operator[](const char* key) {
        return (*this)[std::string(key)];
    }
    JsonValueProxy operator[](int index) {
        if (parent_) return JsonValueProxy(parent_, index);
        return JsonValueProxy(nullptr, -1);
    }

    JsonMemberRef operator[](const std::string& key) const {
        if (parent_ && parent_->holds<object_t>()) {
            auto& obj = parent_->get<object_t>();
            auto it = obj.find(key);
            if (it != obj.end()) {
                return JsonMemberRef(it->second);
            }
        }
        return JsonMemberRef{};
    }
    JsonMemberRef operator[](const char* key) const {
        return (*this)[std::string(key)];
    }
    JsonMemberRef operator[](int index) const {
        if (parent_ && parent_->holds<array_t>()) {
            auto& arr = parent_->get<array_t>();
            if (index >= 0 && static_cast<size_t>(index) < arr.size()) {
                return JsonMemberRef(arr[static_cast<size_t>(index)]);
            }
        }
        return JsonMemberRef{};
    }
};

class JsonValue {
    glz::generic_json<> data_;
public:
    struct ObjectValueTag {};
    JsonValue() = default;
    JsonValue(const JsonValue& other) : data_(other.data_) {}
    JsonValue(JsonValue&& other) noexcept : data_(std::move(other.data_)) {}
    JsonValue(const glz::generic_json<>& other) : data_(other) {}
    JsonValue(glz::generic_json<>&& other) noexcept : data_(std::move(other)) {}
    JsonValue(const JsonMemberRef& ref) : data_(ref.impl()) {}
    explicit JsonValue(ObjectValueTag) : data_(object_t{}) {}

    template<typename T>
    JsonValue(T val) {
        if constexpr (std::is_same_v<T, std::string>) { data_ = val; }
        else if constexpr (std::is_same_v<T, const char*>) { data_ = std::string(val); }
        else if constexpr (std::is_same_v<T, int>) { data_ = val; }
        else if constexpr (std::is_same_v<T, int64_t>) { data_ = val; }
        else if constexpr (std::is_same_v<T, double>) { data_ = val; }
        else if constexpr (std::is_same_v<T, float>) { data_ = static_cast<double>(val); }
        else if constexpr (std::is_same_v<T, bool>) { data_ = val; }
        else if constexpr (std::is_same_v<T, array_t>) { data_ = val; }
        else if constexpr (std::is_same_v<T, object_t>) { data_ = val; }
        else if constexpr (std::is_same_v<T, decltype(nullptr)>) { data_ = nullptr; }
        else if constexpr (std::is_same_v<T, JsonValueProxy>) {
            if (auto* p = val.impl_ptr()) data_ = *p;
        }
        else { static_assert(sizeof(T) == 0, "Unsupported type"); }
    }

    const glz::generic_json<>& impl() const { return data_; }
    glz::generic_json<>& impl() { return data_; }

    JsonValue payload() const { return *this; }

    bool isMember(const std::string& key) const {
        if (!impl().holds<object_t>()) return false;
        return impl().get<object_t>().find(key) != impl().get<object_t>().end();
    }

    bool isObject() const { return impl().holds<object_t>(); }
    bool is_object() const { return impl().holds<object_t>(); }
    bool isArray() const { return impl().holds<array_t>(); }
    bool is_array() const { return impl().holds<array_t>(); }
    bool isString() const { return impl().holds<std::string>(); }
    bool isNumber() const { return impl().holds<double>(); }
    bool isBool() const { return impl().holds<bool>(); }
    bool isNull() const { return impl().holds<std::nullptr_t>(); }
    bool empty() const { return impl().empty(); }

    template<typename T>
    bool holds() const {
        return impl().template holds<T>();
    }

    bool holds_object() const { return impl().holds<object_t>(); }
    bool holds_array() const { return impl().holds<array_t>(); }
    bool holds_string() const { return impl().holds<std::string>(); }
    bool holds_number() const { return impl().holds<double>(); }
    bool holds_bool() const { return impl().holds<bool>(); }
    bool holds_null() const { return impl().holds<std::nullptr_t>(); }

    size_t size() const {
        if (impl().holds<array_t>()) return impl().get<array_t>().size();
        if (impl().holds<object_t>()) return impl().get<object_t>().size();
        return 0;
    }

    std::string toStyledString() const {
        auto result = impl().dump();
        return result.has_value() ? *result : "{}";
    }

    std::string asString() const {
        if (impl().holds<std::string>()) return impl().get<std::string>();
        return "";
    }

    int asInt() const { return impl().as<int>(); }
    int64_t asInt64() const { return impl().as<int64_t>(); }
    double asDouble() const { return impl().as<double>(); }
    bool asBool() const { return impl().as<bool>(); }

    JsonValueProxy operator[](int index) {
        auto& arr = data_.get<array_t>();
        if (index >= 0 && static_cast<size_t>(index) < arr.size()) {
            return JsonValueProxy(&arr[static_cast<size_t>(index)], index);
        }
        if (!data_.holds<array_t>()) data_ = array_t{};
        arr = data_.get<array_t>();
        if (static_cast<size_t>(index) >= arr.size()) {
            arr.resize(static_cast<size_t>(index) + 1);
        }
        return JsonValueProxy(&arr[static_cast<size_t>(index)], index);
    }

    JsonValue operator[](int index) const {
        const auto& arr = impl().get<array_t>();
        if (index >= 0 && static_cast<size_t>(index) < arr.size()) {
            return JsonValue(arr[static_cast<size_t>(index)]);
        }
        return JsonValue();
    }

    JsonValueProxy operator[](const std::string& key) {
        if (!data_.holds<object_t>()) data_ = object_t{};
        auto& obj = data_.get<object_t>();
        auto it = obj.find(key);
        if (it == obj.end()) {
            auto& ref = obj[key];
            return JsonValueProxy(&ref, key);
        }
        return JsonValueProxy(&it->second, key);
    }

    JsonValue operator[](const std::string& key) const {
        const auto& obj = impl().get<object_t>();
        auto it = obj.find(key);
        if (it == obj.end()) {
            return JsonValue();
        }
        return JsonValue(it->second);
    }

    JsonValueProxy operator[](const char* key) {
        return (*this)[std::string(key)];
    }

    const JsonValue operator[](const char* key) const {
        return (*this)[std::string(key)];
    }

    JsonMemberRef get(const std::string& key) const {
        if (!impl().holds<object_t>()) return JsonMemberRef{};
        const auto& obj = impl().get<object_t>();
        auto it = obj.find(key);
        if (it == obj.end()) return JsonMemberRef{};
        return JsonMemberRef(it->second);
    }

    JsonMemberRef get(const char* key) const {
        return get(std::string(key));
    }

    JsonMemberRef get(int index) const {
        if (!impl().holds<array_t>()) return JsonMemberRef{};
        const auto& arr = impl().get<array_t>();
        if (index < 0 || static_cast<size_t>(index) >= arr.size()) return JsonMemberRef{};
        return JsonMemberRef(arr[static_cast<size_t>(index)]);
    }

    template<typename T>
    JsonMemberRef get(const std::string& key, T default_val) const {
        if (!impl().holds<object_t>()) return JsonMemberRef{};
        const auto& obj = impl().get<object_t>();
        auto it = obj.find(key);
        if (it == obj.end()) {
            if constexpr (std::is_same_v<std::decay_t<T>, JsonValue>) {
                return JsonMemberRef(default_val.impl());
            } else {
                return JsonMemberRef(glz::generic_json<>(default_val));
            }
        }
        return JsonMemberRef(it->second);
    }

    template<typename T>
    JsonMemberRef get(const char* key, T default_val) const {
        return get(std::string(key), default_val);
    }

    template<typename T, typename F>
    JsonMemberRef get(const std::string& key, F default_fn) const {
        if (!impl().holds<object_t>()) return JsonMemberRef{};
        const auto& obj = impl().get<object_t>();
        auto it = obj.find(key);
        if (it == obj.end()) {
            return JsonMemberRef(glz::generic_json<>(default_fn()));
        }
        return JsonMemberRef(it->second);
    }

    template<class F>
    auto and_then(F&& f) const -> decltype(auto) {
        return glz::write_json(impl()).and_then(std::forward<F>(f));
    }

    template<class F>
    auto value_or(F&& f) const -> decltype(auto) {
        return glz::write_json(impl()).value_or(std::forward<F>(f)());
    }

    class iterator {
        typename array_t::iterator it_;
    public:
        iterator() = default;
        iterator(typename array_t::iterator it) : it_(it) {}
        iterator& operator++() { ++it_; return *this; }
        bool operator!=(const iterator& other) const { return it_ != other.it_; }
        bool operator==(const iterator& other) const { return it_ == other.it_; }
        JsonValue operator*() const { return JsonValue(*it_); }
    };

    class const_iterator {
        typename array_t::const_iterator it_;
    public:
        const_iterator() = default;
        const_iterator(typename array_t::const_iterator it) : it_(it) {}
        const_iterator& operator++() { ++it_; return *this; }
        bool operator!=(const const_iterator& other) const { return it_ != other.it_; }
        bool operator==(const const_iterator& other) const { return it_ == other.it_; }
        JsonValue operator*() const { return JsonValue(*it_); }
    };

    iterator begin() {
        if (!data_.holds<array_t>()) {
            static array_t empty;
            return iterator(empty.end());
        }
        return iterator(data_.get<array_t>().begin());
    }

    iterator end() {
        if (!data_.holds<array_t>()) {
            static array_t empty;
            return iterator(empty.end());
        }
        return iterator(data_.get<array_t>().end());
    }

    const_iterator begin() const {
        if (!data_.holds<array_t>()) {
            static array_t empty;
            return const_iterator(empty.end());
        }
        return const_iterator(data_.get<array_t>().begin());
    }

    const_iterator end() const {
        if (!data_.holds<array_t>()) {
            static array_t empty;
            return const_iterator(empty.end());
        }
        return const_iterator(data_.get<array_t>().end());
    }

    JsonValue& operator=(const std::nullptr_t v) { data_ = v; return *this; }
    JsonValue& operator=(const double v) { data_ = v; return *this; }
    JsonValue& operator=(const int v) { data_ = v; return *this; }
    JsonValue& operator=(const int64_t v) { data_ = v; return *this; }
    JsonValue& operator=(const std::string& v) { data_ = v; return *this; }
    JsonValue& operator=(std::string&& v) { data_ = std::move(v); return *this; }
    JsonValue& operator=(const char* v) { data_ = std::string(v); return *this; }
    JsonValue& operator=(const bool v) { data_ = v; return *this; }
    JsonValue& operator=(const array_t& v) { data_ = v; return *this; }
    JsonValue& operator=(const object_t& v) { data_ = v; return *this; }
    JsonValue& operator=(const JsonValue& v) { data_ = v.data_; return *this; }

    void append(const JsonValue& item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(item.impl());
    }

    void append(int item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(glz::generic_json<>(item));
    }

    void append(int64_t item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(glz::generic_json<>(item));
    }

    void append(double item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(glz::generic_json<>(item));
    }

    void append(const std::string& item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(glz::generic_json<>(item));
    }

    void append(const char* item) {
        if (!data_.holds<array_t>()) data_ = array_t{};
        data_.get<array_t>().push_back(glz::generic_json<>(std::string(item)));
    }
};

inline void JsonValueProxy::append(const JsonValue& item) const {
    if (parent_ && parent_->holds<array_t>()) {
        parent_->get<array_t>().push_back(item.impl());
    }
}

inline void JsonValueProxy::append(JsonValue&& item) const {
    if (parent_ && parent_->holds<array_t>()) {
        parent_->get<array_t>().push_back(std::move(item.impl()));
    }
}

inline JsonValue JsonMemberRef::asValue() const { return JsonValue(val_); }

class JsonCharReader {
public:
    bool parse(const char* begin, const char* end, JsonValue* root, std::string* err) {
        auto ec = glz::read_json(root->impl(), std::string_view(begin, end - begin));
        if (err && bool(ec)) {
            *err = glz::format_error(ec, std::string_view(begin, end - begin));
        }
        return !bool(ec);
    }

    bool parse(const std::string& str, JsonValue& root) {
        return parse(str.data(), str.data() + str.size(), &root, nullptr);
    }
};

struct JsonCharReaderBuilder {
    static JsonValue make() { return JsonValue(object_t{}); }
    JsonCharReaderBuilder() = default;
    JsonCharReader* newCharReader() const { return new JsonCharReader(); }
};

inline JsonCharReader* newCharReader(JsonCharReaderBuilder&) {
    return new JsonCharReader();
}

using JsonReader = JsonCharReader;

struct JsonStreamWriterBuilder {
    std::map<std::string, std::string> settings;
    JsonStreamWriterBuilder() {}

    struct ValueProxy {
        JsonStreamWriterBuilder& builder;
        std::string key;
        ValueProxy(JsonStreamWriterBuilder& b, const std::string& k) : builder(b), key(k) {}
        ValueProxy& operator=(const std::string& val) { builder.settings[key] = val; return *this; }
    };

    ValueProxy operator[](const char* key) { return ValueProxy(*this, key); }

    static JsonValue make() { return JsonValue(object_t{}); }
};

inline bool reader_parse(std::string_view json_str, JsonValue& out) {
    auto ec = glz::read_json(out.impl(), std::string(json_str));
    return !bool(ec);
}

inline bool reader_parse(const std::string& json_str, JsonValue& out) {
    return reader_parse(std::string_view(json_str), out);
}

inline bool glaze_parse(JsonValue& out, std::string_view json_str) {
    return reader_parse(json_str, out);
}

inline bool glaze_parse(JsonValue& out, const std::string& json_str, std::string* error_out) {
    auto ec = glz::read_json(out.impl(), std::string_view(json_str));
    if (error_out && bool(ec)) {
        *error_out = glz::format_error(ec, std::string_view(json_str));
    }
    return !bool(ec);
}

inline std::string glaze_stringify(const JsonValue& value) {
    auto result = value.impl().dump();
    return result.has_value() ? *result : "{}";
}

inline std::string glaze_json(const JsonValue& value) { return glaze_stringify(value); }

inline bool glaze_is_object(const JsonValue& val) { return val.isObject(); }
inline bool glaze_is_object(const glz::generic_json<>& val) { return val.holds<object_t>(); }
inline bool glaze_is_object(const JsonMemberRef& val) { return val.isObject(); }
inline bool glaze_is_object(const JsonValueProxy& val) { return val.isObject(); }

inline bool glaze_is_array(const JsonValue& val) { return val.isArray(); }
inline bool glaze_is_array(const glz::generic_json<>& val) { return val.holds<array_t>(); }
inline bool glaze_is_array(const JsonMemberRef& val) { return val.isArray(); }
inline bool glaze_is_array(const JsonValueProxy& val) { return val.isArray(); }

inline bool glaze_is_string(const JsonValue& val) { return val.isString(); }
inline bool glaze_is_string(const glz::generic_json<>& val) { return val.holds<std::string>(); }
inline bool glaze_is_string(const JsonMemberRef& val) { return val.isString(); }

inline bool glaze_is_number(const JsonValue& val) { return val.isNumber(); }
inline bool glaze_is_number(const glz::generic_json<>& val) { return val.holds<double>(); }
inline bool glaze_is_number(const JsonMemberRef& val) { return val.isNumber(); }

inline bool glaze_is_bool(const JsonValue& val) { return val.isBool(); }
inline bool glaze_is_bool(const glz::generic_json<>& val) { return val.holds<bool>(); }
inline bool glaze_is_bool(const JsonMemberRef& val) { return val.isBool(); }

inline bool glaze_is_null(const JsonValue& val) { return val.isNull(); }
inline bool glaze_is_null(const glz::generic_json<>& val) { return val.holds<std::nullptr_t>(); }
inline bool glaze_is_null(const JsonMemberRef& val) { return val.isNull(); }

inline JsonValue glaze_empty_object() { return JsonValue(object_t{}); }
inline JsonValue glaze_empty_array() { return JsonValue(array_t{}); }
inline JsonValue glaze_make_array() { return JsonValue(array_t{}); }

inline std::vector<std::string> getMemberNames(const JsonValue& val) {
    std::vector<std::string> names;
    if (!val.isObject()) return names;
    const auto& obj = val.impl().get<object_t>();
    for (const auto& [key, _] : obj) names.push_back(key);
    return names;
}

inline bool glaze_object_empty(const JsonValue& val) {
    if (!val.isObject()) return true;
    return val.impl().get<object_t>().empty();
}

inline bool glaze_array_empty(const JsonValue& val) {
    if (!val.isArray()) return true;
    return val.impl().get<array_t>().empty();
}

inline void append(const JsonValue& arr, const JsonValue& item) { const_cast<JsonValue&>(arr).append(item); }
inline void append(const JsonValue& arr, int item) { const_cast<JsonValue&>(arr).append(item); }
inline void append(const JsonValue& arr, int64_t item) { const_cast<JsonValue&>(arr).append(item); }
inline void append(const JsonValue& arr, double item) { const_cast<JsonValue&>(arr).append(item); }
inline void append(const JsonValue& arr, const std::string& item) { const_cast<JsonValue&>(arr).append(item); }
inline void append(const JsonValue& arr, const char* item) { const_cast<JsonValue&>(arr).append(item); }

inline void glaze_array_append(JsonValue& arr, const JsonValue& item) { append(arr, item); }
inline void glaze_array_append(JsonValue& arr, int item) { append(arr, item); }
inline void glaze_array_append(JsonValue& arr, int64_t item) { append(arr, item); }
inline void glaze_array_append(JsonValue& arr, double item) { append(arr, item); }
inline void glaze_array_append(JsonValue& arr, const std::string& item) { append(arr, item); }
inline void glaze_array_append(JsonValue& arr, const char* item) { append(arr, item); }

inline void glaze_array_append(JsonValueProxy& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy& arr, int item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy& arr, int64_t item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy& arr, double item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy& arr, const std::string& item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy& arr, const char* item) { arr.append(item); }

inline void glaze_array_append(const JsonValueProxy& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_array_append(const JsonValueProxy& arr, int item) { arr.append(item); }
inline void glaze_array_append(const JsonValueProxy& arr, int64_t item) { arr.append(item); }
inline void glaze_array_append(const JsonValueProxy& arr, double item) { arr.append(item); }
inline void glaze_array_append(const JsonValueProxy& arr, const std::string& item) { arr.append(item); }
inline void glaze_array_append(const JsonValueProxy& arr, const char* item) { arr.append(item); }

inline void glaze_array_append(JsonValueProxy&& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy&& arr, int item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy&& arr, int64_t item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy&& arr, double item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy&& arr, const std::string& item) { arr.append(item); }
inline void glaze_array_append(JsonValueProxy&& arr, const char* item) { arr.append(item); }

inline void glaze_append(JsonValueProxy& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_append(JsonValueProxy& arr, int item) { arr.append(item); }
inline void glaze_append(JsonValueProxy& arr, int64_t item) { arr.append(item); }
inline void glaze_append(JsonValueProxy& arr, double item) { arr.append(item); }
inline void glaze_append(JsonValueProxy& arr, const std::string& item) { arr.append(item); }
inline void glaze_append(JsonValueProxy& arr, const char* item) { arr.append(item); }

inline void glaze_append(const JsonValueProxy& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_append(const JsonValueProxy& arr, int item) { arr.append(item); }
inline void glaze_append(const JsonValueProxy& arr, int64_t item) { arr.append(item); }
inline void glaze_append(const JsonValueProxy& arr, double item) { arr.append(item); }
inline void glaze_append(const JsonValueProxy& arr, const std::string& item) { arr.append(item); }
inline void glaze_append(const JsonValueProxy& arr, const char* item) { arr.append(item); }

inline void glaze_append(JsonValueProxy&& arr, const JsonValue& item) { arr.append(item); }
inline void glaze_append(JsonValueProxy&& arr, int item) { arr.append(item); }
inline void glaze_append(JsonValueProxy&& arr, int64_t item) { arr.append(item); }
inline void glaze_append(JsonValueProxy&& arr, double item) { arr.append(item); }
inline void glaze_append(JsonValueProxy&& arr, const std::string& item) { arr.append(item); }
inline void glaze_append(JsonValueProxy&& arr, const char* item) { arr.append(item); }

inline void glaze_append(JsonMemberRef& arr, const JsonValue& item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(item.impl());
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, const glz::generic_json<>& item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(item);
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, const std::string& item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, int item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, int64_t item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, double item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(glz::generic_json<>(item));
        }
    }
}
inline void glaze_append(JsonMemberRef& arr, const char* item) {
    if (arr.isArray()) {
        auto& impl = arr.impl();
        if (impl.holds<array_t>()) {
            impl.get<array_t>().push_back(glz::generic_json<>(std::string(item)));
        }
    }
}

inline int64_t glaze_get_int64(const JsonValue& val) {
    return val.asInt64();
}

inline bool glaze_has_key(const JsonValue& val, const std::string& key) {
    return val.isMember(key);
}

inline bool glaze_has_key(const JsonValue& val, const char* key) {
    return val.isMember(std::string(key));
}

inline bool glaze_has_key(const glz::generic_json<>& val, const std::string& key) {
    if (!val.holds<object_t>()) return false;
    return val.get<object_t>().find(key) != val.get<object_t>().end();
}

inline bool glaze_has_key(const glz::generic_json<>& val, const char* key) {
    return glaze_has_key(val, std::string(key));
}

inline bool glaze_has_key(const JsonMemberRef& val, const std::string& key) {
    return glaze_has_key(val.impl(), key);
}

inline bool glaze_has_key(const JsonMemberRef& val, const char* key) {
    return glaze_has_key(val.impl(), key);
}

inline bool isMember(const JsonValue& val, const std::string& key) {
    return val.isMember(key);
}

inline bool isMember(const JsonValue& val, const char* key) {
    return val.isMember(std::string(key));
}

inline bool isMember(const glz::generic_json<>& val, const std::string& key) {
    return glaze_has_key(val, key);
}

inline bool isMember(const glz::generic_json<>& val, const char* key) {
    return glaze_has_key(val, key);
}

inline bool isMember(const JsonMemberRef& val, const std::string& key) {
    return val.isObject() && glaze_has_key(val.impl(), key);
}

inline bool isMember(const JsonMemberRef& val, const char* key) {
    return isMember(val, std::string(key));
}

inline bool isMember(const JsonValueProxy& val, const std::string& key) {
    return val.isObject();
}

inline bool isMember(const JsonValueProxy& val, const char* key) {
    return val.isObject();
}

inline JsonValue arrayValue{array_t{}};
inline JsonValue objectValue{JsonValue::ObjectValueTag{}};

inline JsonMemberRef make_member_ref(const JsonValue& val) {
    return JsonMemberRef(val.impl());
}

template<typename T>
inline T glaze_safe_get(JsonValue& val, T default_val) {
    return glaze_safe_get<T>(const_cast<const JsonValue&>(val), default_val);
}

template<typename T>
inline T glaze_safe_get(JsonValue& val, const std::string& key, T default_val) {
    return glaze_safe_get<T>(const_cast<const JsonValue&>(val), key, default_val);
}

template<typename T>
inline T glaze_safe_get(JsonValue& val, const char* key, T default_val) {
    return glaze_safe_get<T>(const_cast<const JsonValue&>(val), key, default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonValue& val, T default_val) {
    if constexpr (std::is_same_v<T, std::string>) { return val.asString(); }
    else if constexpr (std::is_same_v<T, int>) { return val.asInt(); }
    else if constexpr (std::is_same_v<T, int64_t>) { return val.asInt64(); }
    else if constexpr (std::is_same_v<T, double>) { return val.asDouble(); }
    else if constexpr (std::is_same_v<T, bool>) { return val.asBool(); }
    return default_val;
}

// 3-parameter overload: get value by key from JsonValue
template<typename T>
inline T glaze_safe_get(const JsonValue& val, const std::string& key, T default_val) {
    if (!val.isObject()) return default_val;
    const auto& object = val.impl().get<object_t>();
    auto it = object.find(key);
    if (it == object.end()) return default_val;
    return glaze_safe_get<T>(JsonValue(it->second), default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonValue& val, const char* key, T default_val) {
    return glaze_safe_get<T>(val, std::string(key), default_val);
}

template<typename T>
inline T glaze_safe_get(const glz::generic_json<>& val, T default_val) {
    if constexpr (std::is_same_v<T, std::string>) {
        if (val.holds<std::string>()) return val.get<std::string>();
        return "";
    } else if constexpr (std::is_same_v<T, int>) {
        return val.as<int>();
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return val.as<int64_t>();
    } else if constexpr (std::is_same_v<T, double>) {
        return val.as<double>();
    } else if constexpr (std::is_same_v<T, bool>) {
        return val.as<bool>();
    }
    return default_val;
}

template<typename T>
inline T glaze_safe_get(const JsonMemberRef& val, T default_val) {
    return glaze_safe_get<T>(val.impl(), default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonValueProxy& val, T default_val) {
    auto* p = val.impl_ptr();
    if (!p) return default_val;
    return glaze_safe_get<T>(*p, default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonValueProxy& val, const std::string& key, T default_val) {
    auto* p = val.impl_ptr();
    if (!p || !p->holds<object_t>()) return default_val;
    const auto& object = p->get<object_t>();
    auto iter = object.find(key);
    if (iter == object.end()) return default_val;
    return glaze_safe_get<T>(iter->second, default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonValueProxy& val, const char* key, T default_val) {
    return glaze_safe_get<T>(val, std::string(key), default_val);
}

template<typename T>
inline T glaze_safe_get(const glz::generic_json<>& val, const std::string& key, T default_val) {
    if (!val.holds<object_t>()) return default_val;
    const auto& object = val.get<object_t>();
    auto iter = object.find(key);
    if (iter == object.end()) return default_val;
    return glaze_safe_get<T>(iter->second, default_val);
}

template<typename T>
inline T glaze_safe_get(const glz::generic_json<>& val, const char* key, T default_val) {
    return glaze_safe_get<T>(val, std::string(key), default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonMemberRef& val, const std::string& key, T default_val) {
    return glaze_safe_get<T>(val.impl(), key, default_val);
}

template<typename T>
inline T glaze_safe_get(const JsonMemberRef& val, const char* key, T default_val) {
    return glaze_safe_get<T>(val.impl(), key, default_val);
}

template<typename T>
struct kvp {
    std::string key;
    mutable T value;
    kvp(const char* k, T v) : key(k), value(std::move(v)) {}
};

template<typename T>
inline void glaze_append(JsonValue& doc, const kvp<T>& pair) {
    if (!doc.isObject()) doc = object_t{};
    JsonValue new_val{};
    if constexpr (std::is_same_v<T, std::string>) { new_val = pair.value; }
    else if constexpr (std::is_same_v<T, int>) { new_val = static_cast<double>(pair.value); }
    else if constexpr (std::is_same_v<T, int64_t>) { new_val = static_cast<double>(pair.value); }
    else if constexpr (std::is_same_v<T, double>) { new_val = pair.value; }
    else { new_val = pair.value; }
    doc[pair.key] = std::move(new_val);
}

inline void glaze_append(JsonValue& arr, const JsonValue& item) {
    append(arr, item);
}

inline void glaze_append(JsonValue& arr, const std::string& item) {
    append(arr, item);
}

inline void glaze_clear(JsonValue& val) { val = nullptr; }

inline JsonValue glaze_get(JsonValue& val, const std::string& key) {
    if (!val.isObject()) return JsonValue(object_t{});
    return val[key];
}

inline JsonValue glaze_get(const JsonValue& val, const std::string& key) {
    if (!val.isObject()) return JsonValue{};
    const auto& object = val.impl().get<object_t>();
    auto it = object.find(key);
    if (it == object.end()) return JsonValue{};
    return JsonValue(it->second);
}

inline JsonValue glaze_get(JsonValue& val, int index) {
    if (!val.isArray()) return JsonValue{};
    const auto& arr = val.impl().get<array_t>();
    if (index < 0 || static_cast<size_t>(index) >= arr.size()) return JsonValue{};
    return JsonValue(arr[static_cast<size_t>(index)]);
}

inline JsonValue glaze_get(const JsonValue& val, int index) {
    if (!val.isArray()) return JsonValue{};
    const auto& arr = val.impl().get<array_t>();
    if (index < 0 || static_cast<size_t>(index) >= arr.size()) return JsonValue{};
    return JsonValue(arr[static_cast<size_t>(index)]);
}

inline JsonValue glaze_get(const JsonValueProxy& val, int index) {
    return val.get(index).asValue();
}

inline JsonValue glaze_get(const JsonValueProxy& val, const std::string& key) {
    return val[key].asValue();
}

inline JsonValue glaze_get(const JsonValueProxy& val, const char* key) {
    return glaze_get(val, std::string(key));
}

inline JsonValue glaze_get(const JsonValue& val) {
    return val;
}

inline JsonValue glaze_get(const glz::generic_json<>& val) {
    return JsonValue(val);
}

inline array_t* glaze_get_array(JsonValue& val) {
    if (!val.isArray()) return nullptr;
    return &val.impl().get<array_t>();
}

inline const array_t* glaze_get_array(const JsonValue& val) {
    if (!val.isArray()) return nullptr;
    return &val.impl().get<array_t>();
}

inline const array_t* glaze_get_array(const JsonMemberRef& val) {
    if (!val.isArray()) return nullptr;
    return &val.impl().get<array_t>();
}

inline const array_t* glaze_get_array(const JsonValueProxy& val) {
    if (!val.isArray()) return nullptr;
    auto* p = val.impl_ptr();
    if (!p) return nullptr;
    return &p->get<array_t>();
}

inline JsonValue glaze_get(const array_t& arr, int index) {
    if (index < 0 || static_cast<size_t>(index) >= arr.size()) return JsonValue{};
    return JsonValue(arr[static_cast<size_t>(index)]);
}

inline JsonValue glaze_get(const array_t* arr, int index) {
    if (!arr) return JsonValue{};
    return glaze_get(*arr, index);
}

template<typename T>
inline JsonValue glaze_get(const JsonValue& val, const std::string& key, T default_val) {
    if (!val.isObject()) return JsonValue(default_val);
    const auto& object = val.impl().get<object_t>();
    auto it = object.find(key);
    if (it == object.end()) return JsonValue(default_val);
    return JsonValue(it->second);
}

inline std::string glaze_get_string(const JsonValue& val, const std::string& key, const std::string& fallback = "") {
    return glaze_safe_get<std::string>(val, key, fallback);
}

template<typename... Args>
inline JsonValue make_document(Args&&... args) {
    JsonValue doc = glaze_empty_object();
    (..., glaze_append(doc, std::forward<Args>(args)));
    return doc;
}

inline std::string writeString(JsonStreamWriterBuilder&, const JsonValue& value) {
    return glaze_json(value);
}

inline std::string writeString(const JsonValue& value) {
    return glaze_json(value);
}

#define GLZ_REFLECT(...) static_assert(true, "GLZ_REFLECT is deprecated")

inline JsonValue ConvertJsonToGlaze(const glz::generic_json<>& src) {
    return JsonValue(src);
}

inline JsonValue ConvertJsonToGlaze(const JsonValue& src) {
    return src;
}

inline glz::generic_json<> ConvertGlazeToJson(const JsonValue& src) {
    return src.impl();
}

} // namespace memochat::json

// Only define Json namespace aliases if JsonCpp is NOT available
// JsonCpp defines JSONCPP_VERSION when included
#if !defined(JSONCPP_VERSION) && !defined(JSONCPP_VERSION_MAJOR)

// Don't define Json namespace if JsonCpp types are already defined
// This prevents conflicts when <json/json.h> is included after GlazeCompat.h

namespace Json {
    using memochat::json::JsonValue;
    using Value = memochat::json::JsonValue;
    using Int64 = int64_t;
    using StreamWriterBuilder = memochat::json::JsonStreamWriterBuilder;
    using CharReaderBuilder = memochat::json::JsonCharReaderBuilder;
    using memochat::json::JsonReader;
    using CharReader = memochat::json::JsonCharReader;
    using memochat::json::newCharReader;
    using memochat::json::writeString;
    using memochat::json::reader_parse;
    using memochat::json::glaze_parse;
    using memochat::json::glaze_stringify;
    using memochat::json::glaze_is_object;
    using memochat::json::glaze_is_array;
    using memochat::json::glaze_is_string;
    using memochat::json::glaze_is_number;
    using memochat::json::glaze_is_bool;
    using memochat::json::glaze_is_null;
    using memochat::json::glaze_object_empty;
    using memochat::json::glaze_array_empty;
    using memochat::json::glaze_empty_object;
    using memochat::json::glaze_empty_array;
    using memochat::json::glaze_get_string;
    using memochat::json::glaze_safe_get;
    using memochat::json::glaze_make_array;
    using memochat::json::make_document;
    using memochat::json::getMemberNames;
    using memochat::json::glaze_append;
    using memochat::json::isMember;

    inline JsonValue objectValue{};
    inline JsonValue arrayValue{};

    class Reader {
    public:
        bool parse(const std::string& json_str, JsonValue& out) {
            return memochat::json::reader_parse(json_str, out);
        }
    };
}

#endif // !JSONCPP_VERSION

#endif // MEMOCHAT_JSON_GLAZECOMPAT_INCLUDED

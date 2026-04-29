#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <functional>

using namespace std;

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    static std::shared_ptr<T> _instance;

    struct PrivateDeleter {
        void operator()(T* ptr) const {
            ptr->~T();
            ::operator delete(ptr);
        }
    };

    static std::mutex& InstanceMutex() {
        static std::mutex s_mutex;
        return s_mutex;
    }

public:
    static std::shared_ptr<T> GetInstance() {
        std::lock_guard<std::mutex> lock(InstanceMutex());
        if (!_instance) {
            T* raw = static_cast<T*>(::operator new(sizeof(T)));
            ::new (raw) T();
            _instance = std::shared_ptr<T>(raw, PrivateDeleter{});
        }
        return _instance;
    }

    template<typename... Args>
    static std::shared_ptr<T> GetInstance(Args&&... args) {
        std::lock_guard<std::mutex> lock(InstanceMutex());
        if (!_instance) {
            T* raw = static_cast<T*>(::operator new(sizeof(T)));
            ::new (raw) T(std::forward<Args>(args)...);
            _instance = std::shared_ptr<T>(raw, PrivateDeleter{});
        }
        return _instance;
    }

    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }

    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

// Macro to be used inside classes that inherit from Singleton<T>
// to declare Singleton<T> as a friend so GetInstance() can call the private constructor
// Usage: put DECLARE_SINGLETON_FRIEND(); inside your class definition
#define DECLARE_SINGLETON_FRIEND() \
    template<typename> friend class ::Singleton

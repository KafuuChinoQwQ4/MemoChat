#pragma once
#include <memory>
#include <mutex>

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }
    
    // 方便调试，打印地址
    void PrintAddress() {
        // std::cout << _instance.get() << std::endl; 
    }
    
    virtual ~Singleton() {
        // cout << "Singleton Destruct" << endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
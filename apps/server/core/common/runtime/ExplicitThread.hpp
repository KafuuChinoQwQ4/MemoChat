#pragma once

#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>

#include <cstring>
#endif

namespace memochat::runtime
{

// std::thread construction reports operating-system failures by throwing.
// ExplicitThread keeps that boundary in return values so no-exception builds can
// reject startup cleanly. Like jthread, destruction joins unless ownership was
// explicitly detached; owners must signal long-running loops before destruction.
class ExplicitThread
{
public:
    ExplicitThread() noexcept = default;

    ~ExplicitThread()
    {
        if (Joinable())
        {
            if (!Join())
            {
                std::abort();
            }
        }
    }

    ExplicitThread(const ExplicitThread&) = delete;
    ExplicitThread& operator=(const ExplicitThread&) = delete;

    ExplicitThread(ExplicitThread&& other) noexcept
    {
        Take(other);
    }

    ExplicitThread& operator=(ExplicitThread&& other) noexcept
    {
        if (this != &other)
        {
            if (Joinable())
            {
                if (!Join())
                {
                    std::abort();
                }
            }
            Take(other);
        }
        return *this;
    }

    template <typename Function> bool Start(Function&& function, std::string* error = nullptr) noexcept
    {
        ClearError(error);
        if (Joinable())
        {
            SetError(error, "thread is already running");
            return false;
        }

        using Task = ThreadTask<std::decay_t<Function>>;
        auto* task = new (std::nothrow) Task(std::forward<Function>(function));
        if (task == nullptr)
        {
            SetError(error, "thread task allocation failed");
            return false;
        }

#ifdef _WIN32
        const std::uintptr_t handle = ::_beginthreadex(nullptr, 0, &RunWindows<Task>, task, 0, &thread_id_);
        if (handle == 0)
        {
            const int code = errno;
            delete task;
            SetError(error, "_beginthreadex failed with code " + std::to_string(code));
            return false;
        }
        handle_ = reinterpret_cast<HANDLE>(handle);
#else
        const int code = ::pthread_create(&thread_, nullptr, &RunPosix<Task>, task);
        if (code != 0)
        {
            delete task;
            SetError(error, std::string("pthread_create failed: ") + std::strerror(code));
            return false;
        }
        joinable_ = true;
#endif
        return true;
    }

    [[nodiscard]] bool Joinable() const noexcept
    {
#ifdef _WIN32
        return handle_ != nullptr;
#else
        return joinable_;
#endif
    }

    bool Join(std::string* error = nullptr) noexcept
    {
        ClearError(error);
        if (!Joinable())
        {
            SetError(error, "thread is not joinable");
            return false;
        }

#ifdef _WIN32
        if (::GetThreadId(handle_) == ::GetCurrentThreadId())
        {
            SetError(error, "thread cannot join itself");
            return false;
        }
        const DWORD wait_result = ::WaitForSingleObject(handle_, INFINITE);
        if (wait_result != WAIT_OBJECT_0)
        {
            SetError(error, "WaitForSingleObject failed with code " + std::to_string(::GetLastError()));
            return false;
        }
        ::CloseHandle(handle_);
        handle_ = nullptr;
        thread_id_ = 0;
#else
        if (::pthread_equal(thread_, ::pthread_self()) != 0)
        {
            SetError(error, "thread cannot join itself");
            return false;
        }
        const int code = ::pthread_join(thread_, nullptr);
        if (code != 0)
        {
            SetError(error, std::string("pthread_join failed: ") + std::strerror(code));
            return false;
        }
        joinable_ = false;
        thread_ = {};
#endif
        return true;
    }

    bool Detach(std::string* error = nullptr) noexcept
    {
        ClearError(error);
        if (!Joinable())
        {
            SetError(error, "thread is not joinable");
            return false;
        }

#ifdef _WIN32
        if (::CloseHandle(handle_) == 0)
        {
            SetError(error, "CloseHandle failed with code " + std::to_string(::GetLastError()));
            return false;
        }
        handle_ = nullptr;
        thread_id_ = 0;
#else
        const int code = ::pthread_detach(thread_);
        if (code != 0)
        {
            SetError(error, std::string("pthread_detach failed: ") + std::strerror(code));
            return false;
        }
        joinable_ = false;
        thread_ = {};
#endif
        return true;
    }

    [[nodiscard]] static unsigned int HardwareConcurrency() noexcept
    {
#ifdef _WIN32
        const DWORD count = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
        return count == 0 ? 1U : static_cast<unsigned int>(count);
#else
        const long count = ::sysconf(_SC_NPROCESSORS_ONLN);
        return count > 0 ? static_cast<unsigned int>(count) : 1U;
#endif
    }

private:
    template <typename Function> struct ThreadTask
    {
        template <typename Source>
        explicit ThreadTask(Source&& source)
            : function(std::forward<Source>(source))
        {
        }

        Function function;
    };

#ifdef _WIN32
    template <typename Task> static unsigned __stdcall RunWindows(void* context) noexcept
    {
        auto* task = static_cast<Task*>(context);
        task->function();
        delete task;
        return 0;
    }
#else
    template <typename Task> static void* RunPosix(void* context) noexcept
    {
        auto* task = static_cast<Task*>(context);
        task->function();
        delete task;
        return nullptr;
    }
#endif

    static void ClearError(std::string* error) noexcept
    {
        if (error != nullptr)
        {
            error->clear();
        }
    }

    static void SetError(std::string* error, std::string message) noexcept
    {
        if (error != nullptr)
        {
            *error = std::move(message);
        }
    }

    void Take(ExplicitThread& other) noexcept
    {
#ifdef _WIN32
        handle_ = other.handle_;
        thread_id_ = other.thread_id_;
        other.handle_ = nullptr;
        other.thread_id_ = 0;
#else
        thread_ = other.thread_;
        joinable_ = other.joinable_;
        other.thread_ = {};
        other.joinable_ = false;
#endif
    }

#ifdef _WIN32
    HANDLE handle_ = nullptr;
    unsigned int thread_id_ = 0;
#else
    pthread_t thread_{};
    bool joinable_ = false;
#endif
};

} // namespace memochat::runtime

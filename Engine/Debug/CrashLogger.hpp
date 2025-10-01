#pragma once
#include <string>
#include <functional>

class CrashLogger {
public:
    CrashLogger(std::string dir, std::string file, std::string tag);
    void SetDir(std::string dir);
    void SetFile(std::string file);
    void SetTag(std::string tag);
    void Write(std::string reason, std::string extra);
    void Mirror(std::string line);
    std::string LogPath() const;
    static std::string Now();
#if defined(__ANDROID__)
    void InitAndroid(void* env, void* context);
#endif
private:
    std::string dir_;
    std::string file_;
    std::string tag_;
};

extern CrashLogger* g_crashLogger;

void InstallTerminateHandler();
void InstallSignalHandlers();

struct TryGuard {
    template <typename F>
    static void Run(F&& f, std::string where) {
        try { f(); }
        catch (const std::exception& e) { if (g_crashLogger) { g_crashLogger->Write("std::exception", where + "|" + e.what()); g_crashLogger->Mirror(where + "|" + e.what()); } throw; }
        catch (...) { if (g_crashLogger) { g_crashLogger->Write("unknown_exception", where); g_crashLogger->Mirror(where); } throw; }
    }
};

template <typename T, typename D = std::function<void(T*)>>
class SafePtr {
public:
    explicit SafePtr(T* p, D d) : p_(p), d_(d) {}
    SafePtr(const SafePtr&) = delete;
    SafePtr& operator=(const SafePtr&) = delete;
    SafePtr(SafePtr&& o) noexcept : p_(o.p_), d_(std::move(o.d_)) { o.p_ = nullptr; }
    SafePtr& operator=(SafePtr&& o) noexcept { if (this != &o) { reset(); p_ = o.p_; d_ = std::move(o.d_); o.p_ = nullptr; } return *this; }
    ~SafePtr() { reset(); }
    T* get() const { return p_; }
    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }
    void reset(T* p = nullptr) { if (p_) d_(p_); p_ = p; }
private:
    T* p_;
    D d_;
};

#include "CrashLogger.hpp"
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <filesystem>
#if defined(__ANDROID__)
#include <android/log.h>
#include <jni.h>
#endif

CrashLogger* g_crashLogger;

static void WriteLine(const std::string& path, const std::string& line) {
    std::ofstream ofs(path, std::ios::out | std::ios::app);
    ofs << line << "\n";
}

CrashLogger::CrashLogger(std::string dir, std::string file, std::string tag)
    : dir_(std::filesystem::absolute(std::filesystem::path(std::move(dir))).string()),
    file_(std::move(file)),
    tag_(std::move(tag)) {
    std::filesystem::create_directories(dir_);
}

void CrashLogger::SetDir(std::string dir) {
    dir_ = std::filesystem::absolute(std::filesystem::path(std::move(dir))).string();
    std::filesystem::create_directories(dir_);
}
void CrashLogger::SetFile(std::string file) { file_ = std::move(file); }
void CrashLogger::SetTag(std::string tag) { tag_ = std::move(tag); }

std::string CrashLogger::LogPath() const {
    return (std::filesystem::path(dir_) / file_).string();
}

std::string CrashLogger::Now() {
    auto t = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tmv;
#if defined(_WIN32)
    gmtime_s(&tmv, &tt);
#else
    gmtime_r(&tt, &tmv);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

void CrashLogger::Write(std::string reason, std::string extra) {
    WriteLine(LogPath(), Now() + "|" + reason + "|" + extra);
}

void CrashLogger::Mirror(std::string line) {
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_FATAL, tag_.empty() ? "ENGINE/CRASH" : tag_.c_str(), line.c_str());
#endif
}

#if defined(__ANDROID__)
void CrashLogger::InitAndroid(void* env, void* context) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    jobject ctx = reinterpret_cast<jobject>(context);
    jclass ctxCls = e->GetObjectClass(ctx);
    jmethodID mid = e->GetMethodID(ctxCls, "getFilesDir", "()Ljava/io/File;");
    jobject fileObj = e->CallObjectMethod(ctx, mid);
    jclass fileCls = e->GetObjectClass(fileObj);
    jmethodID p = e->GetMethodID(fileCls, "getAbsolutePath", "()Ljava/lang/String;");
    jstring jpath = (jstring)e->CallObjectMethod(fileObj, p);
    const char* cpath = e->GetStringUTFChars(jpath, nullptr);
    dir_ = std::string(cpath);
    e->ReleaseStringUTFChars(jpath, cpath);
    std::filesystem::create_directories(dir_);
}
#endif

static void OnTerminate() {
    if (g_crashLogger) {
        g_crashLogger->Write("std_terminate", "");
        g_crashLogger->Mirror("std_terminate");
    }
    std::_Exit(1);
}

static void OnSignal(int sig) {
    if (g_crashLogger) {
        std::string s = "signal_" + std::to_string(sig);
        g_crashLogger->Write(s, "");
        g_crashLogger->Mirror(s);
    }
    std::_Exit(1);
}

void InstallTerminateHandler() { std::set_terminate(OnTerminate); }

void InstallSignalHandlers() {
    std::signal(SIGSEGV, OnSignal);
    std::signal(SIGABRT, OnSignal);
    std::signal(SIGFPE, OnSignal);
    std::signal(SIGILL, OnSignal);
    std::signal(SIGTERM, OnSignal);
}

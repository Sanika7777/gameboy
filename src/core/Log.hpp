#pragma once

// Role: process-wide diagnostic log (console, plus neobrick.log natively).
// Demonstrates: Singleton (with caveats); variadic templates and fold expressions; append-mode file output

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace nb {

// Singletons hide dependencies and make tests share state, so the rest of the code passes
// objects explicitly. Logging is the exception: every layer may need it, it holds no game
// state, and threading a logger through every constructor would add noise for no benefit.
class Log {
public:
    enum class Level { Info, Warn, Error };

    static Log& get() {
        static Log instance;   // Meyers singleton: constructed on first use, thread-safe init
        return instance;
    }
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    void openFile(const std::filesystem::path& path) {
        file_.open(path, std::ios::app);
    }

    template<class... Args>
    void write(Level level, Args&&... args) {
        std::ostringstream os;
        (os << ... << std::forward<Args>(args));
        emit(level, os.str());
    }

private:
    Log() = default;
    void emit(Level level, const std::string& msg);

    std::ofstream file_;
};

template<class... Args> void logInfo(Args&&... a) { Log::get().write(Log::Level::Info, std::forward<Args>(a)...); }
template<class... Args> void logWarn(Args&&... a) { Log::get().write(Log::Level::Warn, std::forward<Args>(a)...); }
template<class... Args> void logError(Args&&... a) { Log::get().write(Log::Level::Error, std::forward<Args>(a)...); }

} // namespace nb

#include "core/Log.hpp"

#include <iostream>

namespace nb {
void Log::emit(Level level, const std::string& msg) {
    static constexpr const char* kTags[] = {"[info] ", "[warn] ", "[error] "};
    const char* tag = kTags[static_cast<int>(level)];
    (level == Level::Info ? std::cout : std::cerr) << tag << msg << std::endl;
    if (file_.is_open()) file_ << tag << msg << '\n' << std::flush;
}

}

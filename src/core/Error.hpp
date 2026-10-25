#pragma once

// Role: every recoverable failure the app can report, organised so callers catch by category.
// Demonstrates: custom exception hierarchy

#include <stdexcept>
#include <string>

namespace nb {

// what() is a short headline for a 26-column screen; context() carries the detail.
class Error : public std::runtime_error {
public:
    explicit Error(const std::string& what, std::string context = {})
        : std::runtime_error(what), context_(std::move(context)) {}
    [[nodiscard]] const std::string& context() const noexcept { return context_; }

private:
    std::string context_;
};

class PlatformError : public Error { using Error::Error; };

class AssetError : public Error { using Error::Error; };
class FileNotFoundError : public AssetError { using AssetError::AssetError; };

class ParseError : public AssetError {
public:
    ParseError(std::string file, int line, const std::string& message)
        : AssetError("PARSE ERROR", message), file_(std::move(file)), line_(line) {}
    [[nodiscard]] const std::string& file() const noexcept { return file_; }
    [[nodiscard]] int line() const noexcept { return line_; }

private:
    std::string file_;
    int line_;
};

class SaveError : public Error { using Error::Error; };
class CorruptSaveError : public SaveError { using SaveError::SaveError; };
class VersionMismatchError : public SaveError { using SaveError::SaveError; };

class GameError : public Error { using Error::Error; };
class UnknownGameError : public GameError { using GameError::GameError; };

// Programmer error (bad arguments, misuse), not a runtime condition.
class ContractViolation : public std::logic_error { using std::logic_error::logic_error; };

} // namespace nb

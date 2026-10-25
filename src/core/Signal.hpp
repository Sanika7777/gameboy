#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace nb {
class Connection {
public:
    Connection() = default;
    explicit Connection(std::shared_ptr<void> slot) noexcept : slot_(std::move(slot)) {}
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection() = default;

private:
    std::shared_ptr<void> slot_;
};

template<class... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    [[nodiscard]] Connection connect(Slot s) {
        auto owned = std::make_shared<Slot>(std::move(s));
        slots_.push_back(owned);
        return Connection{std::move(owned)};
    }

    void emit(const Args&... args) {
        std::erase_if(slots_, [](const auto& w) { return w.expired(); });
        for (std::size_t i = 0; i < slots_.size(); ++i)
            if (auto s = slots_[i].lock()) (*s)(args...);
    }

    [[nodiscard]] std::size_t listenerCount() const noexcept {
        return static_cast<std::size_t>(std::count_if(slots_.begin(), slots_.end(), [](const auto& w) { return !w.expired(); }));
    }

private:
    std::vector<std::weak_ptr<Slot>> slots_;
};

struct EventBus {
    Signal<std::string_view> playSfx;
    Signal<int> vibrate;
    Signal<std::string_view> toast;
};

}

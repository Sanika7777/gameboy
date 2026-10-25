#pragma once

// Role: decoupled notifications (play a sound, vibrate, show a toast) between layers.
// Demonstrates: Observer; variadic class template; shared_ptr / weak_ptr; RAII (Connection)

#include <algorithm>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace nb {

// Move-only handle: destroying it disconnects the slot, so a listener can't outlive its owner.
class Connection {
public:
    Connection() = default;
    explicit Connection(std::shared_ptr<void> slot) noexcept : slot_(std::move(slot)) {}
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection() = default;

    void disconnect() noexcept { slot_.reset(); }
    [[nodiscard]] bool connected() const noexcept { return slot_ != nullptr; }

private:
    std::shared_ptr<void> slot_;   // the only owner of the slot
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
        // Index loop: a slot may connect another listener while we iterate.
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
    Signal<std::string_view> playSfx;   // sound effect name from sfx.txt
    Signal<int> vibrate;                // milliseconds
    Signal<std::string_view> toast;     // short message strip on the LCD
};

} // namespace nb

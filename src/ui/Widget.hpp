#pragma once

#include "gfx/Renderer.hpp"
#include "input/InputState.hpp"

#include <string>
#include <vector>

namespace nb::ui {
class Widget {
public:
    explicit Widget(Rect bounds) : bounds_(bounds) {}
    virtual ~Widget() = default;
    virtual void draw(gfx::Renderer& r) const = 0;
    virtual bool handle(const input::InputState&) { return false; }
    [[nodiscard]] const Rect& bounds() const noexcept { return bounds_; }

protected:
    Rect bounds_;
};

class ListView final : public Widget {
public:
    ListView(Rect bounds, std::vector<std::string> items) : Widget(bounds), items_(std::move(items)) {}

    ListView& operator++() noexcept { cursor_ = (cursor_ + 1) % count(); return *this; }
    ListView& operator--() noexcept { cursor_ = (cursor_ + count() - 1) % count(); return *this; }

    [[nodiscard]] int cursor() const noexcept { return cursor_; }
    void setCursor(int c) noexcept { cursor_ = c % count(); }
    [[nodiscard]] int count() const noexcept { return static_cast<int>(items_.size()); }
    void setItem(int i, std::string text) { items_[static_cast<std::size_t>(i)] = std::move(text); }
    void setItems(std::vector<std::string> items) {
        items_ = std::move(items);
        cursor_ = std::min(cursor_, count() - 1);
    }
    void setDisabled(int i, bool off) { disabled_ = off ? i : (disabled_ == i ? -1 : disabled_); }
    void tick() noexcept { ++frames_; }

    bool handle(const input::InputState& in) override;
    void draw(gfx::Renderer& r) const override;

private:
    std::vector<std::string> items_;
    int cursor_ = 0, disabled_ = -1, frames_ = 0;
};

class Toast {
public:
    void show(std::string text, int frames = 150) {
        text_ = std::move(text);
        frames_ = frames;
    }
    void update() noexcept {
        if (frames_ > 0) --frames_;
    }
    void draw(gfx::Renderer& r) const;

private:
    std::string text_;
    int frames_ = 0;
};

}

#pragma once

// Role: small retained UI for menus, pause and settings lists.
// Demonstrates: Composite (Widget / Panel); const correctness and mutable (Label width cache);
//               operator overloading (ListView operator++/--)

#include "gfx/Renderer.hpp"
#include "input/InputState.hpp"

#include <memory>
#include <string>
#include <vector>

namespace nb::ui {

class Widget {
public:
    explicit Widget(Rect bounds) : bounds_(bounds) {}
    virtual ~Widget() = default;
    virtual void draw(gfx::Renderer& r) const = 0;
    // Returns true if the input was consumed.
    virtual bool handle(const input::InputState&) { return false; }
    [[nodiscard]] const Rect& bounds() const noexcept { return bounds_; }

protected:
    Rect bounds_;
};

class Label final : public Widget {
public:
    Label(Vec2i at, std::string text, gfx::TextStyle style = {})
        : Widget(Rect(at.x, at.y, 0, 8)), text_(std::move(text)), style_(style) {}
    void setText(std::string t) {
        text_ = std::move(t);
        width_ = -1;
    }
    [[nodiscard]] int width(const gfx::Renderer& r) const {
        if (width_ < 0) width_ = r.font(style_.font()).textWidth(text_);   // cached: logically const
        return width_;
    }
    void draw(gfx::Renderer& r) const override { r.text(text_, {bounds_.x, bounds_.y}, style_); }

private:
    std::string text_;
    gfx::TextStyle style_;
    mutable int width_ = -1;
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

class Panel final : public Widget {
public:
    explicit Panel(Rect bounds) : Widget(bounds) {}
    template<class W, class... Args>
    W& add(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W& ref = *w;
        children_.push_back(std::move(w));
        return ref;
    }
    bool handle(const input::InputState& in) override {
        for (auto& c : children_)
            if (c->handle(in)) return true;
        return false;
    }
    void draw(gfx::Renderer& r) const override;

private:
    std::vector<std::unique_ptr<Widget>> children_;
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

} // namespace nb::ui

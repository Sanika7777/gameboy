#pragma once

// Role: one screen of the app (boot, menu, game, pause…) and the stack that runs them.
// Demonstrates: abstract class, pure virtuals; State pattern; perfect forwarding (push<T>(Args&&...));
//               unique_ptr ownership

#include "core/Seconds.hpp"
#include "gfx/Renderer.hpp"
#include "input/InputState.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace nb::scenes {

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() {}
    virtual void onExit() {}
    // Became the top scene again because the one above it was popped.
    virtual void onResume() {}
    virtual void update(const input::InputState& in, Seconds dt) = 0;
    virtual void draw(gfx::Renderer& r) const = 0;
    [[nodiscard]] virtual bool isOverlay() const { return false; }
    // The page was hidden (tab switched, phone locked).
    virtual void onHidden() {}
};

class SceneStack {
public:
    SceneStack() { pending_.reserve(8); }

    template<class S, class... Args>
    void push(Args&&... args) {
        pending_.push_back({Op::Push, std::make_unique<S>(std::forward<Args>(args)...)});
    }
    template<class S, class... Args>
    void replace(Args&&... args) {
        pending_.push_back({Op::Replace, std::make_unique<S>(std::forward<Args>(args)...)});
    }
    // Clear everything and show this scene (error recovery, back to menu).
    template<class S, class... Args>
    void resetTo(Args&&... args) {
        pending_.push_back({Op::Reset, std::make_unique<S>(std::forward<Args>(args)...)});
    }
    void pop() { pending_.push_back({Op::Pop, nullptr}); }

    // Updates the top scene, then applies requests: a scene may safely remove itself.
    void update(const input::InputState& in, Seconds dt);
    void draw(gfx::Renderer& r) const;
    void applyPending();
    void hidden() {
        if (!stack_.empty()) stack_.back()->onHidden();
    }

    // Applied immediately: used by App when an exception escaped a scene.
    void resetNow(std::unique_ptr<Scene> s);

    [[nodiscard]] bool empty() const noexcept { return stack_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return stack_.size(); }
    [[nodiscard]] Scene* top() const noexcept { return stack_.empty() ? nullptr : stack_.back().get(); }
    // True once after the visible screen changed (drives the palette fade).
    [[nodiscard]] bool takeChanged() noexcept { return std::exchange(changed_, false); }

private:
    enum class Op { Push, Pop, Replace, Reset };
    struct Request {
        Op op;
        std::unique_ptr<Scene> scene;
    };
    void popOne();

    std::vector<std::unique_ptr<Scene>> stack_;
    std::vector<Request> pending_;
    bool changed_ = false;
};

} // namespace nb::scenes

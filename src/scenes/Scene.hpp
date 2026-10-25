#pragma once

#include "core/Clock.hpp"
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
    virtual void onResume() {}
    virtual void update(const input::InputState& in, double dt) = 0;
    virtual void draw(gfx::Renderer& r) const = 0;
    [[nodiscard]] virtual bool isOverlay() const { return false; }
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
    template<class S, class... Args>
    void resetTo(Args&&... args) {
        pending_.push_back({Op::Reset, std::make_unique<S>(std::forward<Args>(args)...)});
    }
    void pop() { pending_.push_back({Op::Pop, nullptr}); }

    void update(const input::InputState& in, double dt);
    void draw(gfx::Renderer& r) const;
    void applyPending();
    void hidden() {
        if (!stack_.empty()) stack_.back()->onHidden();
    }

    void resetNow(std::unique_ptr<Scene> s);

    [[nodiscard]] bool empty() const noexcept { return stack_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return stack_.size(); }
    [[nodiscard]] Scene* top() const noexcept { return stack_.empty() ? nullptr : stack_.back().get(); }
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

}

#include "scenes/Scene.hpp"

namespace nb::scenes {

void SceneStack::update(const input::InputState& in, Seconds dt) {
    if (!stack_.empty()) stack_.back()->update(in, dt);
    applyPending();
}

void SceneStack::popOne() {
    if (stack_.empty()) return;
    stack_.back()->onExit();
    stack_.pop_back();
    if (!stack_.empty()) stack_.back()->onResume();
}

void SceneStack::applyPending() {
    // Swap out first: onEnter/onExit may queue further requests for next frame.
    auto requests = std::move(pending_);
    pending_.clear();
    pending_.reserve(8);
    for (auto& r : requests) {
        switch (r.op) {
        case Op::Pop: popOne(); break;
        case Op::Replace: popOne(); [[fallthrough]];
        case Op::Push:
            stack_.push_back(std::move(r.scene));
            stack_.back()->onEnter();
            break;
        case Op::Reset:
            while (!stack_.empty()) popOne();
            stack_.push_back(std::move(r.scene));
            stack_.back()->onEnter();
            break;
        }
        changed_ = true;
    }
}

void SceneStack::resetNow(std::unique_ptr<Scene> s) {
    pending_.clear();
    stack_.clear();   // no onExit: we're recovering from an exception inside a scene
    stack_.push_back(std::move(s));
    stack_.back()->onEnter();
    changed_ = true;
}

void SceneStack::draw(gfx::Renderer& r) const {
    if (stack_.empty()) return;
    // Overlays show the scene beneath them: start from the highest non-overlay.
    std::size_t first = stack_.size() - 1;
    while (first > 0 && stack_[first]->isOverlay()) --first;
    for (std::size_t i = first; i < stack_.size(); ++i) stack_[i]->draw(r);
}

} // namespace nb::scenes

#pragma once

// Role: things that live in a physics world: they update and they draw.
// Demonstrates: multiple inheritance (interfaces); hybrid/diamond inheritance with virtual base

#include "core/Seconds.hpp"
#include "gfx/Renderer.hpp"

#include <string>

namespace nb::games {

class Identifiable {
public:
    explicit Identifiable(std::string debugName) : id_(++next_), name_(std::move(debugName)) {}
    virtual ~Identifiable() = default;
    [[nodiscard]] int id() const noexcept { return id_; }
    [[nodiscard]] const std::string& debugName() const noexcept { return name_; }

private:
    static inline int next_ = 0;
    int id_;
    std::string name_;
};

// Both interfaces inherit Identifiable *virtually*, so a GameObject holds exactly one id.
class IUpdatable : public virtual Identifiable {
public:
    using Identifiable::Identifiable;
    virtual void update(Seconds dt) = 0;
};

class IDrawable : public virtual Identifiable {
public:
    using Identifiable::Identifiable;
    virtual void draw(gfx::Renderer& r) const = 0;
};

// The most-derived class constructs the virtual base, so every concrete object writes
// `Identifiable("name")` in its own constructor's initializer list.
class GameObject : public IUpdatable, public IDrawable {
public:
    GameObject() : Identifiable("object"), IUpdatable("object"), IDrawable("object") {}
    bool alive = true;
};

} // namespace nb::games

#pragma once

#include "core/Clock.hpp"
#include "gfx/Renderer.hpp"

#include <string>

namespace nb::games {
class Identifiable {
public:
    explicit Identifiable(std::string debugName) : name_(std::move(debugName)) {}
    virtual ~Identifiable() = default;
    [[nodiscard]] const std::string& debugName() const noexcept { return name_; }

private:
    std::string name_;
};

class IUpdatable : public virtual Identifiable {
public:
    using Identifiable::Identifiable;
    virtual void update(double dt) = 0;
};

class IDrawable : public virtual Identifiable {
public:
    using Identifiable::Identifiable;
    virtual void draw(gfx::Renderer& r) const = 0;
};

class GameObject : public IUpdatable, public IDrawable {
public:
    GameObject() : Identifiable("object"), IUpdatable("object"), IDrawable("object") {}
    bool alive = true;
};

}

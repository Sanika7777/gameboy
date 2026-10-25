#include "games/silverball/TableLoader.hpp"

#include "io/TextReader.hpp"

#include <functional>
#include <map>

namespace nb::games::silverball {

namespace {

Vec2f point(const io::TextReader& r, std::string_view s) {
    const auto comma = s.find(',');
    if (comma == std::string_view::npos) r.fail("expected x,y but got '" + std::string(s) + "'");
    return {io::parseFloat(r, s.substr(0, comma)), io::parseFloat(r, s.substr(comma + 1))};
}

template<class T>
T& add(Table& t, std::unique_ptr<T> e) {
    T& ref = *e;
    t.elements.push_back(std::move(e));
    return ref;
}

using Factory = std::function<void(const io::TextReader&, const io::Attrs&, Table&)>;

// Each keyword maps to the lambda that builds it. Adding an element type = one new entry.
const std::map<std::string, Factory, std::less<>>& factories() {
    static const std::map<std::string, Factory, std::less<>> m{
        {"table", [](const io::TextReader&, const io::Attrs& a, Table& t) {
             t.name = std::string(a.str("name", "Table"));
             t.size = {a.integer("width"), a.integer("height")};
             t.gravity = a.real("gravity", 300.0f);
         }},
        {"wall", [](const io::TextReader& r, const io::Attrs& a, Table& t) {
             const auto& pts = a.list("points");
             if (pts.size() < 2) r.fail("a wall needs at least 2 points");
             for (std::size_t i = 1; i < pts.size(); ++i) add(t, std::make_unique<Wall>(point(r, pts[i - 1]), point(r, pts[i])));
         }},
        {"bumper", [](const io::TextReader&, const io::Attrs& a, Table& t) {
             add(t, std::make_unique<Bumper>(Vec2f{a.real("x"), a.real("y")}, a.real("r"), a.integer("score", 100)));
         }},
        {"sling", [](const io::TextReader&, const io::Attrs& a, Table& t) {
             add(t, std::make_unique<Slingshot>(Vec2f{a.real("x1"), a.real("y1")}, Vec2f{a.real("x2"), a.real("y2")}, a.integer("score", 10)));
         }},
        {"target", [](const io::TextReader& r, const io::Attrs& a, Table& t) {
             const auto g = a.str("group");
             if (g.size() != 1) r.fail("group must be one letter");
             t.targets.push_back(&add(t, std::make_unique<DropTarget>(Vec2f{a.real("x"), a.real("y")}, Vec2f{a.real("w"), a.real("h")},
                                                                      a.integer("score", 500), g[0])));
         }},
        {"rollover", [](const io::TextReader&, const io::Attrs& a, Table& t) {
             t.rollovers.push_back(&add(t, std::make_unique<Rollover>(Vec2f{a.real("x"), a.real("y")}, a.integer("lane", 0))));
         }},
        {"flipper", [](const io::TextReader& r, const io::Attrs& a, Table& t) {
             const auto side = a.str("side");
             if (side != "left" && side != "right") r.fail("side must be left or right");
             auto& f = add(t, std::make_unique<Flipper>(side == "left", point(r, a.str("pivot")), a.real("length")));
             (side == "left" ? t.leftFlipper : t.rightFlipper) = &f;
         }},
        {"plunger", [](const io::TextReader&, const io::Attrs& a, Table& t) {
             t.plunger = &add(t, std::make_unique<Plunger>(Vec2f{a.real("x"), a.real("y")}));
         }},
        {"drain", [](const io::TextReader&, const io::Attrs& a, Table& t) { t.drainY = a.real("y"); }},
    };
    return m;
}

} // namespace

Table loadTable(const std::filesystem::path& file) {
    io::TextReader r(file);
    Table t;
    while (r.next()) {
        const auto tokens = r.tokens();
        const auto it = factories().find(tokens[0]);
        if (it == factories().end()) r.fail("unknown '" + std::string(tokens[0]) + "'");
        const io::Attrs attrs(r, std::span(tokens).subspan(1));
        it->second(r, attrs, t);
    }
    if (!t.leftFlipper || !t.rightFlipper || !t.plunger) r.fail("table needs two flippers and a plunger");
    return t;
}

} // namespace nb::games::silverball

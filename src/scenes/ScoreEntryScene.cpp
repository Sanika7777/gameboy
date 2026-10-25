#include "scenes/ScoreEntryScene.hpp"

#include "scenes/Common.hpp"

#include <ctime>

namespace nb::scenes {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

ScoreEntryScene::ScoreEntryScene(Services& s, std::string key, io::ScoreOrder order, std::uint32_t value, std::string title)
    : s_(s), key_(std::move(key)), title_(std::move(title)), order_(order), value_(value) {}

void ScoreEntryScene::submit() {
    io::ScoreEntry e;
    e.initials = initials_;
    e.value = value_;
    e.time = static_cast<std::uint32_t>(std::time(nullptr));
    rank_ = s_.scores.table(key_, order_).submit(e);
    s_.saveScores();
    done_ = true;
    s_.sfx("score");
}

void ScoreEntryScene::update(const input::InputState& in, double) {
    ++frames_;
    if (done_) {
        if (in.pressed(Button::A) || in.pressed(Button::Start) || in.pressed(Button::B) || in.lcdTap()) {
            s_.sfx("back");
            s_.scenes.pop();
        }
        return;
    }
    char& c = initials_[static_cast<std::size_t>(pos_)];
    if (in.pressed(Button::Up)) { c = c == 'Z' ? 'A' : static_cast<char>(c + 1); s_.sfx("move"); }
    if (in.pressed(Button::Down)) { c = c == 'A' ? 'Z' : static_cast<char>(c - 1); s_.sfx("move"); }
    if (in.pressed(Button::Left) && pos_ > 0) { --pos_; s_.sfx("move"); }
    if (in.pressed(Button::Right) && pos_ < 2) { ++pos_; s_.sfx("move"); }
    if (in.pressed(Button::B) && pos_ > 0) { --pos_; s_.sfx("back"); }
    if (in.pressed(Button::A)) {
        if (pos_ < 2) { ++pos_; s_.sfx("select"); }
        else submit();
    }
    if (in.pressed(Button::Start)) submit();
}

void ScoreEntryScene::draw(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    titleBar(r, done_ ? "HIGH SCORES" : "NEW HIGH SCORE!");
    const bool time = order_ == io::ScoreOrder::LowerIsBetter;
    if (!done_) {
        r.text(title_, {80, 22}, TextStyle{}.align(Align::Center));
        r.text((time ? fmt("%u SECONDS", value_) : fmt("%u POINTS", value_)).data(), {80, 36}, TextStyle{}.align(Align::Center));
        r.text("ENTER YOUR INITIALS", {80, 56}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center));
        for (int i = 0; i < 3; ++i) {
            const Vec2i at{62 + i * 14, 72};
            const char letter[2] = {initials_[static_cast<std::size_t>(i)], '\0'};
            const bool cur = i == pos_;
            r.text(letter, at, TextStyle{}.inverted(cur && (frames_ / 15) % 2 == 0));
            r.line({at.x - 1, at.y + 10}, {at.x + 5, at.y + 10}, cur ? Shade::Ink : Shade::Light);
        }
        r.text("^", {62 + pos_ * 14, 86});
        footer(r, "UP/DOWN: LETTER  A: NEXT  START: OK");
        return;
    }
    const auto* t = s_.scores.find(key_);
    if (!t) return;
    int row = 0;
    for (const auto& e : t->entries()) {
        const int y = 16 + row * 11;
        const bool mine = rank_ && *rank_ == static_cast<std::size_t>(row);
        const auto line = fmt(time ? "%2d %c%c%c %6us" : "%2d %c%c%c %7u", row + 1, e.initials[0], e.initials[1], e.initials[2], e.value);
        r.text(line.data(), {20, y}, TextStyle{}.inverted(mine));
        ++row;
    }
    footer(r, "A: CONTINUE");
}

}

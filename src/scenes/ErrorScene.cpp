#include "scenes/ErrorScene.hpp"

#include "core/Error.hpp"
#include "core/Log.hpp"
#include "scenes/Common.hpp"
#include "scenes/MenuScene.hpp"

namespace nb::scenes {
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

namespace {
constexpr std::size_t kColumns = 26;

void wrapInto(std::vector<std::string>& out, std::string text, const std::string& prefix) {
    for (auto& c : text) c = static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c);
    std::string line = prefix;
    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t room = kColumns - line.size();
        if (text.size() - i <= room) {
            line += text.substr(i);
            break;
        }
        auto cut = text.rfind(' ', i + room);
        if (cut == std::string::npos || cut <= i) cut = i + room;
        line += text.substr(i, cut - i);
        out.push_back(line);
        line = "  ";
        i = cut + (text[cut] == ' ' ? 1 : 0);
    }
    out.push_back(line);
}

}

ErrorScene::ErrorScene(Services& s, std::exception_ptr error) : s_(s) {
    try {
        if (error) std::rethrow_exception(error);
    } catch (const std::exception& e) {
        describe(e);
    } catch (...) {
        lines_.push_back("UNKNOWN ERROR");
    }
    for (const auto& l : lines_) logError("error screen: ", l);
}

void ErrorScene::describe(const std::exception& e) {
    wrapInto(lines_, e.what(), lines_.empty() ? "" : "> ");
    if (const auto* pe = dynamic_cast<const ParseError*>(&e)) {
        wrapInto(lines_, pe->file(), "> ");
        wrapInto(lines_, "LINE " + std::to_string(pe->line()), "> ");
        wrapInto(lines_, pe->context(), "> ");
    } else if (const auto* ne = dynamic_cast<const Error*>(&e); ne && !ne->context().empty()) {
        wrapInto(lines_, ne->context(), "> ");
    }
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& inner) {
        describe(inner);
    } catch (...) {
        lines_.push_back("> UNKNOWN");
    }
}

void ErrorScene::onEnter() { s_.sfx("error"); }

void ErrorScene::update(const input::InputState& in, double) {
    if (in.pressed(Button::A) || in.pressed(Button::Start)) {
        s_.sfx("back");
        s_.scenes.resetTo<MenuScene>(s_);
    }
}

void ErrorScene::draw(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    titleBar(r, "ERROR");
    int y = 16;
    for (const auto& l : lines_) {
        if (y > 120) break;
        r.text(l, {2, y});
        y += 9;
    }
    footer(r, "A: MENU");
}

}

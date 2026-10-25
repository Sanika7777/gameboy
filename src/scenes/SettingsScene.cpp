#include "scenes/SettingsScene.hpp"

#include "core/Error.hpp"
#include "device/DeviceShell.hpp"
#include "gfx/Palette.hpp"
#include "scenes/Common.hpp"

#include <algorithm>

namespace nb::scenes {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

namespace {
constexpr const char* kVersion = "V1.0";

std::string cycle(const std::vector<std::string>& names, const std::string& cur, int dir) {
    auto it = std::find(names.begin(), names.end(), cur);
    auto i = it == names.end() ? 0 : static_cast<int>(it - names.begin());
    const int n = static_cast<int>(names.size());
    return names[static_cast<std::size_t>(((i + dir) % n + n) % n)];
}

std::string upper(std::string s) {
    for (auto& c : s) c = static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c);
    return s;
}

std::string row(const std::string& label, const std::string& value) {
    std::string out = label;
    out.resize(11, ' ');
    return out + "<" + upper(value) + ">";
}

const char* title(SettingsScene::Page p) {
    switch (p) {
    case SettingsScene::Page::Main: return "SETTINGS";
    case SettingsScene::Page::Display: return "DISPLAY";
    case SettingsScene::Page::Sound: return "SOUND";
    case SettingsScene::Page::Controls: return "CONTROLS";
    case SettingsScene::Page::Reset: return "RESET DATA";
    case SettingsScene::Page::About: return "ABOUT";
    }
    return "";
}

}

SettingsScene::SettingsScene(Services& s) : s_(s), list_(Rect(8, 18, 150, 110), {}) { open(Page::Main); }

void SettingsScene::open(Page p) {
    pages_.push(p);
    list_.setCursor(0);
    rebuild();
}

void SettingsScene::rebuild() {
    const auto& v = s_.settings.get();
    switch (pages_.top()) {
    case Page::Main: list_.setItems({"DISPLAY", "SOUND", "CONTROLS", "RESET DATA", "ABOUT", "BACK"}); break;
    case Page::Display:
        list_.setItems({row("PALETTE", v.palette), row("CASE", v.caseTheme), row("LCD FX", v.lcdEffect),
                        row("FLASHING", v.reduceFlashing ? "reduced" : "full"), "BACK"});
        break;
    case Page::Sound: list_.setItems({row("VOLUME", std::to_string(v.volume)), "BACK"}); break;
    case Page::Controls:
        list_.setItems({row("HAPTICS", v.haptics ? "on" : "off"), row("LAYOUT", v.layout),
                        row("HANDED", v.leftHanded ? "left" : "right"), "BACK"});
        break;
    case Page::Reset: list_.setItems({"NO, KEEP THEM", "YES, ERASE"}); break;
    case Page::About: list_.setItems({"BACK"}); break;
    }
}

void SettingsScene::change(int item, int dir) {
    auto v = s_.settings.get();
    const auto page = pages_.top();
    if (page == Page::Display) {
        if (item == 0) v.palette = cycle(gfx::Palette::presetNames(), v.palette, dir);
        else if (item == 1) v.caseTheme = cycle(device::DeviceShell::themeNames(), v.caseTheme, dir);
        else if (item == 2) v.lcdEffect = cycle({"none", "ghosting", "grid"}, v.lcdEffect, dir);
        else if (item == 3) v.reduceFlashing = !v.reduceFlashing;
        else return;
    } else if (page == Page::Sound && item == 0) {
        v.volume = std::clamp(v.volume + dir, 0, 7);
    } else if (page == Page::Controls) {
        if (item == 0) v.haptics = !v.haptics;
        else if (item == 1) v.layout = cycle({"auto", "portrait", "landscape"}, v.layout, dir);
        else if (item == 2) v.leftHanded = !v.leftHanded;
        else return;
    } else {
        return;
    }
    try {
        s_.settings.apply(std::move(v));
    } catch (const SaveError&) {
        s_.toast("SETTINGS NOT SAVED");
    }
    s_.sfx("move");
    rebuild();
}

void SettingsScene::update(const input::InputState& in, double) {
    list_.tick();
    if (list_.handle(in)) {
        s_.sfx("move");
        return;
    }
    const int item = list_.cursor();
    const bool isBack = item == list_.count() - 1 && pages_.top() != Page::Reset;
    if (in.pressed(Button::Left)) change(item, -1);
    else if (in.pressed(Button::Right)) change(item, +1);
    else if (in.pressed(Button::B) || (in.pressed(Button::A) && isBack) || in.pressed(Button::Select)) {
        s_.sfx("back");
        pages_.pop();
        if (pages_.empty()) s_.scenes.pop();
        else { list_.setCursor(0); rebuild(); }
    } else if (in.pressed(Button::A)) {
        switch (pages_.top()) {
        case Page::Main:
            s_.sfx("select");
            open(static_cast<Page>(item + 1));
            break;
        case Page::Reset:
            if (item == 1) {
                s_.scores.clear();
                s_.saveScores();
                s_.toast("SCORES ERASED");
            }
            s_.sfx(item == 1 ? "explode" : "back");
            pages_.pop();
            rebuild();
            break;
        default: change(item, +1); break;
        }
    }
}

void SettingsScene::draw(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    titleBar(r, title(pages_.top()));
    if (pages_.top() == Page::Reset) {
        r.text("ERASE ALL HIGH SCORES?", {80, 64}, TextStyle{}.font(gfx::Font::Mini).align(Align::Center));
    }
    if (pages_.top() == Page::About) {
        const auto mini = TextStyle{}.font(gfx::Font::Mini).align(Align::Center);
        r.text("NEOBRICK", {80, 46}, TextStyle{}.align(Align::Center));
        r.text(kVersion, {80, 58}, mini);
        r.text("A POCKET CONSOLE IN C++20", {80, 70}, mini);
        r.text("SDL2 + WEBASSEMBLY", {80, 78}, mini);
        r.text("ALL GAMES, ART AND SOUND", {80, 90}, mini);
        r.text("ARE ORIGINAL", {80, 98}, mini);
    }
    list_.draw(r);
    footer(r, "UP/DN: PICK  L/R: CHANGE  B: BACK");
}

}

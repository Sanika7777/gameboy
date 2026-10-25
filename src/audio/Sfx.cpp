#include "audio/Sfx.hpp"

#include "io/TextReader.hpp"

namespace nb::audio {
SfxLibrary SfxLibrary::load(const std::filesystem::path& file) {
    io::TextReader r(file);
    SfxLibrary lib;
    while (r.next()) {
        const auto t = r.tokens();
        if (t.size() < 3 || t[0] != "sfx") r.fail("expected: sfx <name> <pulse|wave|noise> key=value...");
        SfxDef d;
        if (t[2] == "pulse") d.voice = Voice::Pulse;
        else if (t[2] == "wave") d.voice = Voice::Wave;
        else if (t[2] == "noise") d.voice = Voice::Noise;
        else r.fail("unknown voice '" + std::string(t[2]) + "'");
        const io::Attrs a(r, std::span(t).subspan(3));
        const int duty = a.integer("duty", 50);
        if (duty != 12 && duty != 25 && duty != 50 && duty != 75) r.fail("duty must be 12, 25, 50 or 75");
        d.duty = duty == 12 ? 0.125f : duty / 100.0f;
        d.fromHz = a.real("from");
        d.toHz = a.real("to", d.fromHz);
        d.ms = a.integer("ms");
        d.volume = a.integer("vol", 12);
        if (d.ms <= 0 || d.ms > 5000 || d.volume < 0 || d.volume > 15 || d.fromHz <= 0 || d.toHz <= 0)
            r.fail("value out of range");
        const auto decay = a.str("decay", "fast");
        if (decay == "none") d.decay = Decay::None;
        else if (decay == "slow") d.decay = Decay::Slow;
        else if (decay == "fast") d.decay = Decay::Fast;
        else r.fail("decay must be none, slow or fast");
        d.shortNoise = a.str("mode", "long") == "short";
        if (!lib.defs_.emplace(std::string(t[1]), d).second) r.fail("duplicate sfx '" + std::string(t[1]) + "'");
    }
    return lib;
}

const SfxDef* SfxLibrary::find(std::string_view name) const {
    const auto it = defs_.find(name);
    return it == defs_.end() ? nullptr : &it->second;
}

}

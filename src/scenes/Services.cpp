#include "scenes/Services.hpp"

#include "core/Error.hpp"
#include "core/Log.hpp"

namespace nb::scenes {
void Services::saveScores() const {
    try {
        scores.save();
    } catch (const SaveError& e) {
        logWarn("scores not saved: ", e.what(), " ", e.context());
        toast("SCORES NOT SAVED");
    }
}

}

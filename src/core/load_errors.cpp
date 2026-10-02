#include "core/load_errors.h"

#include <cassert>

namespace stg {

void LoadErrors::add(const char* path, const char* reason) {
    assert(path != nullptr && reason != nullptr);
    errors_.push_back(LoadError{path, reason});
}

bool LoadErrors::empty() const {
    return errors_.empty();
}

const std::vector<LoadError>& LoadErrors::entries() const {
    return errors_;
}

}  // namespace stg

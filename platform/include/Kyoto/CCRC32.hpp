// Upstream includes this header as "Kyoto/CCRC32.hpp" but ships it as
// include/Kyoto/CCrc32.hpp; MWCC's case-insensitive lookup hides the mismatch.
// On a case-insensitive file system (Windows, macOS) "Kyoto/CCrc32.hpp" finds this
// file again, so the real header is named by its path relative to this one.
#include "../../../include/Kyoto/CCrc32.hpp"

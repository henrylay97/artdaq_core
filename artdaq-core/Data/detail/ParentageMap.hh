#include "canvas/Persistency/Provenance/Parentage.h"
#include "canvas/Persistency/Provenance/ParentageID.h"

#include <map>

namespace art {
/// An art::ParentageMap, defined using a std::map
typedef std::map<ParentageID const, Parentage> ParentageMap;
}  // namespace art

#include "Metadata.h"
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
struct TrackedFramework : ASDCP::MXF::CryptographicFramework {
  int& destroyed;
  TrackedFramework(const ASDCP::Dictionary* dict, int& count, byte_t id)
    : CryptographicFramework(dict), destroyed(count) {
    byte_t uuid[16] = {}; uuid[15] = id;
    InstanceUID = Kumu::UUID(uuid);
  }
  ~TrackedFramework() { ++destroyed; }
};

template<class Container> void check_deletion() {
  const ASDCP::Dictionary& dict = ASDCP::DefaultCompositeDict();
  int destroyed = 0;
  {
    Container container(&dict);
    TrackedFramework* first = new TrackedFramework(&dict, destroyed, 1);
    TrackedFramework* second = new TrackedFramework(&dict, destroyed, 2);
    const Kumu::UUID first_id = first->InstanceUID;
    container.AddChildObject(first); container.AddChildObject(second);
    // Exercise the supported alias: the argument refers into the deleted object.
    require(ASDCP_SUCCESS(container.DeleteMDObjectByID(first->InstanceUID)), "delete failed");
    require(destroyed == 1, "object was not destroyed exactly once");
    ASDCP::MXF::InterchangeObject* object = NULL;
    require(ASDCP_FAILURE(container.GetMDObjectByID(first_id, &object)), "deleted ID still resolves");
    require(ASDCP_FAILURE(container.DeleteMDObjectByID(first_id)), "second delete succeeded");
    std::list<ASDCP::MXF::InterchangeObject*> remaining;
    require(ASDCP_SUCCESS(container.GetMDObjectsByType(dict.ul(ASDCP::MDD_CryptographicFramework), remaining)), "type enumeration failed");
    require(remaining.size() == 1 && remaining.front() == second, "deleted object remains in packet list");
    require(ASDCP_SUCCESS(container.GetMDObjectByType(dict.ul(ASDCP::MDD_CryptographicFramework), &object)) && object == second, "type lookup returned deleted object");
  }
  require(destroyed == 2, "container destruction did not destroy only the remaining object");
}
int main() {
  try {
    check_deletion<ASDCP::MXF::OP1aHeader>();
    check_deletion<ASDCP::MXF::OPAtomIndexFooter>();
    std::cout << "metadata deletion: header and footer passed\n";
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

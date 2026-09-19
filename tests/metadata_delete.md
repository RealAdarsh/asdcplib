# Metadata deletion leaves both indexes consistent

`Partition::PacketList::DeleteMDObjectByID` removed the map entry and deleted
its object but left the pointer in `m_List`. Type lookup and enumeration walk
that list; its destructor deletes every remaining pointer. A successful delete
therefore caused use-after-free during lookup and double deletion on destruction.

The regression creates two objects, deletes the first (passing its own InstanceUID
by reference), checks ID and type lookups, repeats the deletion, and destroys the
container. It runs for both OP1aHeader and OPAtomIndexFooter and checks destructor
counts. Before the fix, CTest reported `metadata-delete (SEGFAULT)`.

The fix removes the pointer from the list before deleting the object. Afterward:

- Debug static build without SSL/XML: CTest 1/1 passed.
- AddressSanitizer + UndefinedBehaviorSanitizer debug build: CTest 1/1 passed.
- Full release build with OpenSSL 3 encryption enabled, XML disabled: all CLI/library
  targets built; CTest 1/1 passed.

Configure with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` for current CMake, then build
and run `ctest --test-dir build --output-on-failure`. Sanitizer configuration used
`-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"`.

The Vellum port does not call this C++ deletion method, so no runtime
dependency substitution is required.

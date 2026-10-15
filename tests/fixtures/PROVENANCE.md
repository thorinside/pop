# Frozen Internal oracle

Source contract is accepted requirements artifact b7200762-9537-43dd-93f2-d8cf02b7d6fb, SHA-256 2dff2c06aadbf384f478b2b682b8d63fd6d972375166822ed5652b859e3c38d8, R1-R4 and AC-01/02/03/04/10/11. Owner approval supersedes historical draft labels, not formulas.

`reference.py` was authored and executed before any production engine or adapter existed. It uses Python Decimal at 60 digits for exponentiation, integer modulo-32-bit xorshift, integer ceil, and no production imports. Its 17 cases include hand-checked AC-01 and contour literals. `internal.json` retains the complete input, expected rise/fall, Active end and every generation state. `internal.h` is the same frozen data for native C++ consumers. Normal verification never regenerates expected values.

Initial hashes, frozen before implementation:

- reference.py `08f7e0012179625173dfd5b1378df60b2d642d9b2d5b3cad95261e58faf89634`.
- internal.json `cd843bc791be152a58e8e1378803ea8d4c4d039662c747cb8fc669ea138291c9`.
- internal.h `78bca7d45488ed946ea6bf4bfa9a114682f6c264f99483a4b1eb1c06c3542884`.

Additional hand-derived boundaries used in tests are width240/gap48 at Fs48000, restart requests100/200 during an existing high0..239 giving activation288/first rise5088, and later requested gaps287/288/289 giving288/288/289. These are contract literals, not captured implementation output. Statistical predicates are the accepted AC-03/11 inequalities, not fitted thresholds.

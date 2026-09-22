<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Release gate

Do not tag this development snapshot as completed v0.1.0.

- [x] Implement the requested software follow-up in STATUS.md.
- [ ] Verify every production rule independently and retain primary-source evidence.
- [ ] Review all application, SDK, binary radio, runtime-library and tooling licenses; archive a complete SPDX binary SBOM.
- [ ] Confirm every original asset's author/license and generated artifact's reproducibility.
- [ ] Review text similarity without importing prohibited detector source.
- [x] Run host regression tests, passive/static invariants, ASan/UBSan and a bounded seeded mutation run; detailed scope is in VERIFICATION.md.
- [ ] Pass all physical-board checks and eight-hour soak; record power/capture loss/heap results.
- [ ] Review passive behavior with independent over-the-air capture.
- [ ] Audit private/research logs and sanitized exports.
- [ ] Record all acceptance evidence and explicit maintainer sign-off in release-acceptance.json.
- [ ] Package hashes, exact SDK/toolchain, flash command and complete notices.
- [ ] Create a signed Git tag only with the maintainer's signing credentials and authorization.

`tools/make_release.py --development` makes local development artifacts. It neither pushes nor signs anything. The source SPDX inventory deliberately reports the SDK's complete transitive license conclusion as `NOASSERTION` until reviewed.

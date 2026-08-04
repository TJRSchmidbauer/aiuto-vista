# ScopeBuddy licensing

ScopeBuddy is **source-available**, but it is not Open Source as defined by the
Open Source Initiative. In particular, commercial sales of ScopeBuddy devices
and kits require a separate license.

## ScopeBuddy contributions

Copyright (c) 2026 Johannes Börnsen.

Original ScopeBuddy software, documentation, artwork, and enclosure-design
contributions are licensed under the
[ScopeBuddy Community License 1.0](LICENSES/LicenseRef-ScopeBuddy-Community-1.0.txt).
This includes original contributions in the following locations, but only to
the extent that Johannes Börnsen owns the applicable rights:

- `main/`
- `docs/`
- `hardware/`
- `tools/`
- `.github/`
- top-level documentation and project configuration

The license permits private use, education, research, modification, and
publication. Published modified versions must provide their corresponding
source, retain attribution to ScopeBuddy, and use a distinct project name.
Devices and kits may be passed on at direct cost, but may not be sold for
profit without a separate commercial license from the copyright holder.

## Elecrow-derived material

The ScopeBuddy Community License does **not** apply to Elecrow material or to
portions derived from Elecrow material. This includes in particular:

- `peripheral/`
- Elecrow-derived board initialization in `main/main.c` and
  `main/include/main.h`
- Elecrow-derived board/build configuration where present

ScopeBuddy modifications within mixed files are offered under the ScopeBuddy
Community License only to the extent separable copyright exists in those
modifications. No license to the underlying Elecrow material is granted by
this repository. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and
[`peripheral/README.md`](peripheral/README.md).

The upstream Elecrow repository did not contain an explicit repository license
when this notice was prepared. Consequently, this repository cannot grant or
confirm permission to use, modify, or redistribute Elecrow's material. Users
must independently determine whether they have the necessary rights.

## Other third-party material

Generated font data, framework components, generated code, and other
third-party material remain under their respective terms. In particular, the
Montserrat-derived font data is covered by the SIL Open Font License 1.1; its
license text is included at [LICENSES/OFL-1.1.txt](LICENSES/OFL-1.1.txt).

Downloaded ESP-IDF managed components are not committed to this repository.
Their own license files and notices apply when they are downloaded or included
in a build.

## Commercial licensing

For permission to sell ScopeBuddy devices or kits commercially, contact the
repository owner through
[the ScopeBuddy GitHub repository](https://github.com/johannesboernsen/ScopeBuddy).

## Contributions

Contributions submitted for inclusion in ScopeBuddy require acceptance of the
[ScopeBuddy Contributor License Agreement 1.0](CLA.md). Contributors retain
their copyright while granting the repository owner the rights required to
maintain the community edition and offer separate commercial licenses.

This overview is intended to make the repository's licensing boundaries clear.
The complete license texts control in case of a conflict.

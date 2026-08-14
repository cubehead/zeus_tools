# Third-party notices

Zeus Tools depends on the following open-source projects. The dependency source
and its complete license text remain authoritative.

| Component | Version/reference | License |
| --- | --- | --- |
| [EUI-NEO](https://github.com/sudoevolve/EUI-NEO) | pinned commit `f2a3b72104bd946988f8ebe0a13dda956f3455ae` | Apache License 2.0 |
| [pugixml](https://github.com/zeux/pugixml) | v1.15, commit `ee86beb30e4973f5feffe3ce63bfa4fbadf72f38` | MIT License |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | yaml-cpp-0.9.0, commit `56e3bb550c91fd7005566f19c079cb7a503223cf` | MIT License |
| [Font Awesome Free](https://fontawesome.com/) | 7.2.0 desktop font, supplied by EUI-NEO | Font: SIL OFL 1.1 |
| [Primer Octicons](https://github.com/primer/octicons) | `mark-github-24` | MIT License; GitHub mark subject to GitHub logo guidelines |

EUI-NEO in turn includes or links open-source libraries such as FreeType, GLFW,
libpng, md4c, yyjson and zlib. Their license texts are present in the EUI-NEO
source tree fetched for a build. Binary release preparation must review the
exact packaged dependency set and include any notices required by those
licenses.

The current EUI-NEO build statically uses FreeType (FreeType License or GPLv2),
GLFW (zlib/libpng license), libpng (PNG Reference Library License 2.0), md4c
(MIT), yyjson (MIT), zlib (zlib license) and tray (MIT), depending on platform.
Complete license texts for the locked direct dependencies and bundled runtime
components are stored in [`docs/licenses/`](docs/licenses/) and included in
binary packages under `licenses/`.

Zeus Tools application artwork under `resources/icons/` is distributed under
the project's MIT License.

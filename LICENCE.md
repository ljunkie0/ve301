# Licence

VE301 source code is published under the GNU Lesser General Public License,
version 3 or later (`LGPL-3.0-or-later`).

This licence applies only to source code written for VE301, unless a file
explicitly says otherwise. It does not relicense third-party libraries, fonts,
logos, or other assets that are used together with VE301. Those components keep
their own licences and notices.

See `COPYING.LESSER` for the LGPL text. `COPYING` contains the GNU GPL text
referenced by the LGPL.

Unless a file states otherwise, source files written for this project are
licensed as:

```text
VE301

Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.
```

## Third-party libraries

VE301 links against a number of external libraries. These libraries are not
published under the VE301 LGPL licence by this repository. They remain under
their own licences, and the VE301 licence is not intended to add restrictions
to those licences.

| Component | Used for | Licence information |
| --- | --- | --- |
| SDL2 | Windowing, input, rendering | zlib/libpng-style licence |
| SDL2_image | Image loading | zlib/libpng-style licence; bundled image loader code may include additional permissive licences |
| SDL2_ttf | TrueType font rendering | zlib/libpng-style licence |
| SDL2_gfx | Rotozoom and drawing helpers | zlib/libpng-style licence |
| libmpdclient | MPD client access | BSD-3-Clause |
| libcurl | HTTP requests | curl licence |
| cJSON | JSON parsing | MIT |
| libxml2 | Podcast RSS/Atom XML parsing | MIT-style libxml2 licence, with some ISC/MIT-style files upstream |
| libmnl | Netlink support used by wifi scanning | LGPL-2.1 |
| libwebsockets | Optional Spotify integration | libwebsockets package metadata includes LGPL-2.1 and MIT/permissive upstream source notices |
| D-Bus | Optional Bluetooth integration | GPL-2.0-or-later or AFL-2.1 for the shared library, with additional permissive notices |
| ALSA lib | Optional ALSA audio integration | LGPL-2.1-or-later |
| WiringPi | Raspberry Pi rotary encoder support | Third-party WiringPi licence; see the installed WiringPi package/release used for Raspberry builds |
| POSIX/pthreads/C runtime | Platform services | System library licences of the target platform |
| Java/JNI headers | JNI wrapper build | Covered by the JDK used to build the JNI target |

The Makefile can also clone/use `wifi-scan` from
`https://github.com/bmegli/wifi-scan.git`. Its library source files are under
the Mozilla Public License 2.0. The example programs in that project carry
separate GPL-3.0 notices, but VE301 uses the library source.

When distributing binaries, include the licence notices required by the
specific library builds and platform packages you ship.

## Bundled fonts

The files in `sample-config/` include fonts that remain under their own font
licences. The VE301 LGPL licence does not apply to these font files and does
not change the permissions or obligations granted by the font licences:

| File | Font | Copyright / author information from font metadata | Licence |
| --- | --- | --- | --- |
| `sample-config/BarlowCondensed-Medium.ttf` | Barlow Condensed Medium | Copyright 2017 The Barlow Project Authors; Jeremy Tribby / The Barlow Project | SIL Open Font License 1.1 |
| `sample-config/weathericons-regular-webfont.ttf` | Weather Icons Regular | Weather Icons; helloerik.com / artill.de metadata | SIL Open Font License 1.1 for the font; associated Weather Icons code is MIT-licensed |

The SIL Open Font License allows bundling, use, study, modification, and
redistribution of the fonts subject to the OFL terms, including the reserved
font name rules where applicable. Keep the font licence notices when
redistributing the sample configuration.

## Sample assets and logos

The remaining files in `sample-config/` are sample configuration files and
visual assets used by the demo setup. They are listed separately from the
source-code licence to avoid implying that third-party marks or separately
licensed assets are relicensed under the LGPL.

`spotify_logo_48.png` and `bluetooth_logo_48.png` depict third-party marks.
Trademark rights are separate from copyright licensing; do not assume the LGPL
grants rights to use those marks outside nominative or otherwise permitted use.

For `ve301_logo_48.png`, `light.png`, and the `skala_bg_*_grid.jpg` sample
backgrounds, no separate third-party licence metadata is present in this
repository. Treat them as VE301 sample assets unless replaced by your own
assets or accompanied by a more specific notice.

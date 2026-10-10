# as_admins (Plugify)

Admins module of the Pisex admin_system (Metamod plugin as_admins), ported to Plugify. Adds the 'mm_admins' console command and the '!admins' chat command (@admin/admins): the former prints the online admins (ID, name, SteamID) to the caller's console, the latter opens the read-only 'Admins_Title' menu (or prints Admins_NoAdmins). Admins hidden through the Hide module's hide_on action are omitted.

Plugify port (fork) of **Admins** by Pisex (Metamod (C++)).
Original: https://github.com/Pisex/cs2-admin_system_modules
The code was rewritten for Plugify / s2sdk; behaviour follows the original.

## Requirements
- [Plugify](https://github.com/untrustedmodders/plugify) with [s2sdk](https://github.com/untrustedmodders/plugify-plugin-s2sdk)
- Plugify dependencies: s2sdk, admin_system, hudmenu (optional)

## Installation
Download `as_admins-1.0.3.zip` from Releases and extract it into `game/csgo/`.

## Building
Requires Docker. `./build.sh` builds in the Steam Runtime 3 (sniper) SDK image; the package
is written to `build/package/as_admins`.

`plugins/admin_system` carry only the API headers of the plugins this one uses.

## License
GPL-3.0, see LICENSE.


// as_admins — Plugify C++ port of the Pisex admin_system module "Admins" (Metamod plugin
// as_admins).
//
// What it does (unchanged from the original):
//   * "mm_admins"  console command (@admin/admins) - prints the online admins to the caller's
//                  console with the original layout ("ID. Name - SteamID"), always in the
//                  console layout, exactly like the original (which passed bConsole = true for
//                  both the server console and a client);
//   * "!admins"    chat command (@admin/admins) - the same list as a read-only admin menu
//                  (Admins_Title, disabled entries), or the Admins_NoAdmins phrase when no
//                  admin is online;
//   * OnAction("hide_on"/"hide_off", ...) - admins that asked the Hide module to hide them are
//                  omitted from the list (the original's g_bHide[64]).
//
// Metamod objects dropped in the port:
//   * CCSPlayerController::FromSlot -> pisexcompat's GetControllerEntity (both return the slot's
//     controller entity; the wrapper returns 0 before a map is up, which is when the original's
//     entity system did not exist either);
//   * CCSPlayerController::m_steamID -> pisexcompat IPlayersApi::GetSteamID64 (s2sdk
//     GetClientSteamID64);
//   * g_pUtils/g_pMenus/g_pPlayers -> pisexcompat;
//   * g_pAdminApi (the Metamod IAdminApi) -> plugins/admin_system/include/admin_api.hpp.
//
// Everything runs on the game thread: the two command handlers and the OnAction callback are
// dispatched by the engine/admin core. Nothing here touches a client from OnPluginStart, and the
// module has no config file (like the original, which only copied its .vdf to the package).

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>

#include <plg/plugin.hpp>
#include <plg/string.hpp>
#include <plugin_export.h>

#include <admin_api.hpp>
#include <pisex/pisex.hpp>

namespace {

using namespace pisex;

// The Pisex globals, same call sites: pisexcompat restores IUtilsApi/IMenusApi/IPlayersApi and
// admin_api.hpp restores IAdminApi (std::function callbacks, std::string, default arguments).
admin_system::AdminApi* g_pAdminApi = &admin_system::AdminApi::Get();

// CS2 has 64 player slots; the original looped over the same fixed 64.
constexpr int kMaxSlots = 64;
// The log tag of the original plugin (GetLogTag()).
constexpr const char* kLogTag = "Admins";

// Pisex's g_bHide[64]: slot -> the admin asked Hide to make them invisible, so they are skipped
// by the admins list.
std::array<bool, kMaxSlots> g_bHide{};

// The binding slots exist only when the admin_system plugin was loaded before us (it is declared
// as a dependency, so normally it was). Everything that has to do real work is behind this check
// because a module must never return Failed from OnPluginStart (that stops every plugin).
bool AdminApiAvailable() {
	return __admin_system_OnAction_Register != nullptr && __admin_system_OnAction_Unregister != nullptr &&
		   __admin_system_HasPermission != nullptr && __admin_system_IsAdmin != nullptr &&
		   __admin_system_GetTranslation != nullptr;
}

// The original OnAdminAction: the Hide module's SendAction(iSlot, "hide_on"/"hide_off") flips the
// flag of the acting admin. Pisex indexed g_bHide[iSlot] unguarded - a slot outside 0..63 (the
// console, -1) was an out-of-bounds write there; it is ignored here.
void OnAdminAction(int iSlot, const char* szAction, const char* szParam) {
	(void)szParam;
	if (iSlot < 0 || iSlot >= kMaxSlots || !szAction) {
		return;
	}
	if (std::strcmp(szAction, "hide_on") == 0) {
		g_bHide[static_cast<std::size_t>(iSlot)] = true;
	} else if (std::strcmp(szAction, "hide_off") == 0) {
		g_bHide[static_cast<std::size_t>(iSlot)] = false;
	}
}

// The original Get_Admins(iSlot, bConsole). bConsole = the flat console list ("ADMINS\nID. Name -
// SteamID"), otherwise the read-only menu. mm_admins always passes true (even for a client
// caller, whose list is then printed to that client's console - g_pUtils->PrintToConsole sends a
// negative slot to the server console), !admins always passes false.
void Get_Admins(int iSlot, bool bConsole) {
	if (bConsole) {
		g_pUtils->PrintToConsole(iSlot, "ADMINS\nID. Name - SteamID\n");
		for (int i = 0; i < kMaxSlots; i++) {
			// 0 when the slot has no controller entity.
			if (GetControllerEntity(i) == 0) {
				continue;
			}
			if (!g_pAdminApi->IsAdmin(i)) {
				continue;
			}
			if (g_bHide[static_cast<std::size_t>(i)]) {
				continue;
			}
			// The original printed the steamid as a signed 64-bit value; the SteamID64 is a
			// uint64_t (unsigned long on Linux), so it is printed as %llu.
			g_pUtils->PrintToConsole(iSlot, "%i. %s - %llu\n", i, g_pPlayers->GetPlayerName(i),
									 static_cast<unsigned long long>(g_pPlayers->GetSteamID64(i)));
		}
		return;
	}

	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Admins_Title"));
	bool bShow = false;
	for (int i = 0; i < kMaxSlots; i++) {
		if (GetControllerEntity(i) == 0) {
			continue;
		}
		if (!g_pAdminApi->IsAdmin(i)) {
			continue;
		}
		if (g_bHide[static_cast<std::size_t>(i)]) {
			continue;
		}
		bShow = true;
		g_pMenus->AddItemMenu(hMenu, "", g_pPlayers->GetPlayerName(i), ITEM_DISABLED);
	}
	g_pMenus->SetExitMenu(hMenu, true);
	if (!bShow) {
		// The original passed the phrase as the printf format; "%s" keeps the phrase text intact
		// even when a translation contains a % character.
		g_pUtils->PrintToChat(iSlot, "%s", g_pAdminApi->GetTranslation("Admins_NoAdmins"));
		return;
	}
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

class AsAdminsPlugin final : public plg::Plugin {
public:
	plg::PluginResult OnPluginStart() override;
	plg::PluginResult OnPluginUpdate(std::chrono::milliseconds dt) override;
	plg::PluginResult OnPluginEnd() override;

private:
	bool m_actionRegistered = false;
};

AsAdminsPlugin g_asAdminsPlugin;

plg::PluginResult AsAdminsPlugin::OnPluginStart() {
	try {
		// Config/translations dirs (<configs>/admin_system) and the s2sdk listeners pisexcompat
		// needs. Never fails hard: a missing s2sdk binding leaves the library in no-op mode.
		pisex::Init("admin_system");

		if (!AdminApiAvailable()) {
			// Fail soft: returning Failed from OnPluginStart would stop every plugify plugin on
			// the server. Without the admin_system bindings there is nothing to register, so the
			// plugin stays loaded but idle (no commands, no action listener).
			g_pUtils->ErrorLog("[%s] the admin_system bindings are not available (is the admin_system plugin loaded?)", kLogTag);
			return {};
		}

		// mm_admins — the Metamod CON_COMMAND_F(mm_admins, "", FCVAR_GAMEDLL): server console only.
		g_pUtils->RegServerCommand({"mm_admins"}, [](int iSlot, const char* szContent) {
			(void)szContent;
			if (!g_pAdminApi->HasPermission(iSlot, "@admin/admins") && iSlot != -1) {
				return true; // handled (the original returned without doing anything)
			}
			Get_Admins(iSlot, true);
			return true;
		});

		// !admins — chat-only, like the original's registration.
		g_pUtils->RegCommand({}, {"!admins"}, [](int iSlot, const char* szContent) {
			(void)szContent;
			if (g_pAdminApi->HasPermission(iSlot, "@admin/admins")) {
				Get_Admins(iSlot, false);
			}
			return true;
		});

		// OnAction: the Hide module's hide_on/hide_off notifications (the original registered
		// this in AllPluginsLoaded).
		g_pAdminApi->OnAction([](int iSlot, const char* szAction, const char* szParam) {
			OnAdminAction(iSlot, szAction, szParam);
		});
		m_actionRegistered = true;

		return {};
	} catch (const std::exception& ex) {
		g_pUtils->ErrorLog("[%s] OnPluginStart failed: %s", kLogTag, ex.what());
	} catch (...) {
		g_pUtils->ErrorLog("[%s] OnPluginStart failed with an unknown exception", kLogTag);
	}
	return {};
}

plg::PluginResult AsAdminsPlugin::OnPluginUpdate(std::chrono::milliseconds dt) {
	(void)dt;
	try {
		// Pumps the deferred work of pisexcompat (timers, next-frame queue, deferred startup).
		pisex::Poll();
	} catch (const std::exception& ex) {
		g_pUtils->ErrorLog("[%s] update failed: %s", kLogTag, ex.what());
	} catch (...) {
		g_pUtils->ErrorLog("[%s] update failed with an unknown exception", kLogTag);
	}
	return {};
}

plg::PluginResult AsAdminsPlugin::OnPluginEnd() {
	try {
		// The admin core keeps a pointer to our trampoline: drop it before the library goes away.
		if (m_actionRegistered) {
			m_actionRegistered = false;
			if (__admin_system_OnAction_Unregister) {
				g_pAdminApi->OnActionUnregister();
			}
		}
		pisex::Shutdown();
	} catch (const std::exception& ex) {
		g_pUtils->ErrorLog("[%s] OnPluginEnd failed: %s", kLogTag, ex.what());
	} catch (...) {
		g_pUtils->ErrorLog("[%s] OnPluginEnd failed with an unknown exception", kLogTag);
	}
	return {};
}

} // namespace

PLUGIFY_PLUGIN(PLUGIN_API, &g_asAdminsPlugin)

/*
  xdrv_95_gogo_portal.ino - GoGoBoard: open the Wi-Fi setup portal on demand

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef CUSTOM_CONFIG_GOGOBOARD
#ifdef USE_WEBSERVER

/*********************************************************************************************\
 * Portal [1|0]
 *
 * With GOGO_NO_AUTO_PORTAL Tasmota never starts its setup access point by itself. This command
 * is the only way to open it. The command itself stores nothing: a restart always closes the portal,
 * and sta_config keeps the value it had (4, WIFI_RETRY, unless someone set another). A Save in the
 * portal does store the typed SSId1 and password, even when its join test fails.
 *
 *   Portal 1  Open the access point (open network, SSID = hostname, 192.168.4.1) for
 *             WIFI_CONFIG_SEC seconds. Asking again while it is open restarts the countdown.
 *             Needs Wi-Fi on (Wifi 1); with Wi-Fi off nothing opens, and Wifi 0 while it is open closes it.
 *   Portal 0  Close it. Tasmota restarts, as it does when the countdown ends.
 *   Portal    Report the state.
 *
 * Every form answers one line: {"Portal":"ON","Seconds":170} or {"Portal":"OFF","Seconds":0}.
 * Seconds is what is left before the portal closes by itself.
\*********************************************************************************************/

#define XDRV_95 95

void CmndGogoPortal(void);

const char kGogoPortalCommands[] PROGMEM = "|"
  "Portal";

void (* const GogoPortalCommand[])(void) PROGMEM = {
  &CmndGogoPortal };

bool GogoPortalIsOpen(void) {
  return (WIFI_MANAGER == Wifi.config_type) || (WIFI_MANAGER_RESET_ONLY == Wifi.config_type);
}

bool GogoPortalIsPending(void) {
  return Settings->flag4.network_wifi && (WIFI_MANAGER == TasmotaGlobal.wifi_state_flag);
}

void GogoPortalResponse(bool open, uint32_t seconds) {
  Response_P(PSTR("{\"Portal\":\"%s\",\"Seconds\":%u}"), GetStateText(open), seconds);
}

void CmndGogoPortal(void) {
  if (1 == XdrvMailbox.payload) {
    if (GogoPortalIsOpen()) {
      WifiConfigCounter();
    } else if (!Wifi.config_type && Settings->flag4.network_wifi) {
      TasmotaGlobal.wifi_state_flag = WIFI_MANAGER;
    }
  } else if (0 == XdrvMailbox.payload) {
    if (GogoPortalIsOpen()) {
      TasmotaGlobal.restart_flag = 2;
    } else if (GogoPortalIsPending()) {
      TasmotaGlobal.wifi_state_flag = WIFI_RESTART;
    }
    GogoPortalResponse(false, 0);
    return;
  }

  if (GogoPortalIsOpen()) {
    GogoPortalResponse(true, Wifi.config_counter);
  } else if (GogoPortalIsPending()) {
    GogoPortalResponse(true, WIFI_CONFIG_SEC);
  } else {
    GogoPortalResponse(false, 0);
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv95(uint32_t function) {
  bool result = false;

  switch (function) {
    case FUNC_EVERY_SECOND:
      if (GogoPortalIsOpen() && !Settings->flag4.network_wifi && !TasmotaGlobal.restart_flag) {
        TasmotaGlobal.restart_flag = 2;
      }
      break;
    case FUNC_SAVE_BEFORE_RESTART:
      if (GogoPortalIsOpen() && (MAX_WIFI_OPTION != Wifi.old_wificonfig)) {
        Settings->sta_config = Wifi.old_wificonfig;
      }
      break;
    case FUNC_COMMAND:
      result = DecodeCommand(kGogoPortalCommands, GogoPortalCommand);
      break;
  }
  return result;
}

#endif  // USE_WEBSERVER
#endif  // CUSTOM_CONFIG_GOGOBOARD

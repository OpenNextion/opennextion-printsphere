# A1 Mini LAN Bring-Up

This file defines the first functional bring-up path for Bambu Lab A1 mini on
the ONX3248G035 PrintSphere port.

## Scope

Goal:

- Connect PrintSphere to the local Wi-Fi network.
- Open Web Config from the home-network device IP.
- Configure the A1 mini local printer path.
- Verify the on-device PIN unlock flow after provisioning.
- Decide whether failures belong to user configuration, App/Protocol, UI, or
  Env/Serial/Flash.

Current firmware baseline for this bring-up:

- `75148f0 Align ONX printer select layout`
- Do not treat uncommitted UI changes as flashed behavior.
- Do not rebuild or flash during this bring-up unless the main thread assigns a
  new firmware validation step.

## Recommended Mode

Use `Local only` for the first A1 mini LAN-mode bring-up.

Reason:

- A1 mini supports the local status path.
- A1 mini supports the local JPEG camera snapshot path.
- A1 mini does not require Developer Mode for local status.
- `Local only` starts local MQTT as soon as Wi-Fi is connected.
- `Hybrid` is useful later, but it introduces cloud-session and online-presence
  gating that can hide a basic LAN setup problem.

Do not configure Bambu Cloud during this first pass.

## User-Prepared Fields

The user should prepare these values from Bambu Handy, the printer UI, or the
router DHCP page:

- Home Wi-Fi SSID.
- Home Wi-Fi password.
- A1 mini IP address or hostname on the same LAN as the ONX board.
- A1 mini serial number.
- A1 mini LAN/local access code.
- Confirmation that LAN/local mode is enabled for the printer.

Do not paste Wi-Fi passwords or printer access codes into Codex chat. Enter
them directly in Web Config.

## Web Config Flow

First provisioning:

1. Connect a phone or Mac to `PrintSphere-Setup`.
2. Use password `printsphere`.
3. Open `http://192.168.4.1`.
4. Save home Wi-Fi credentials.
5. Let the ESP restart and join the home network.

After home Wi-Fi is connected:

1. Find the ONX board IP from the display, router DHCP table, or serial logs.
2. Open `http://<device-ip>`.
3. Set `Connection Mode` to `Local only`.
4. In `Local Printer Path`, enter printer IP/hostname, serial number, and access
   code.
5. Save/connect the local path.

PIN unlock:

- During initial provisioning, Web Config stays open without PIN until the
  selected source path is working.
- After provisioning is complete, Web Config on the home network uses the
  on-device PIN/session flow.
- Hold anywhere on the display for about one second to request a six-digit PIN.
- Enter the PIN in the browser unlock page.
- PIN lifetime is about 2 minutes.
- Browser session lifetime is about 10 minutes.

## Expected Device Behavior

First pass acceptance:

- Wi-Fi shows connected and Web Config is reachable from the home-network IP.
- Source mode is `Local only`.
- Local path reports connected.
- The display status page shows A1 mini state from the local path.
- Reopening Web Config after provisioning requires PIN unlock.
- Long-pressing the display shows a PIN and the browser unlock succeeds.

Optional follow-up:

- Camera page can refresh a local A1 mini JPEG snapshot.
- Hybrid mode can be tested only after Local-only is proven.

## Serial Log Signals

Wi-Fi and portal:

- `Connecting to Wi-Fi SSID=...`
- `Wi-Fi connected, IP=...`
- `Setup AP disabled after station connect`
- Web Config requests to `/api/config`, `/api/source-mode`, `/api/local/connect`,
  or `/api/unlock`

Local MQTT:

- `Connecting to printer MQTT <host>:8883 (serial=..., user=bblp)`
- `Using embedded local Bambu CA bundle for MQTT TLS verification`
- `Connected to <host>`
- `MQTT subscribe acknowledged`
- `Printer info received (A1MINI)`
- `[DIAG] local mqtt: ...`
- `[DIAG] local resolved: ...`

Common failure signals:

- `Need printer host, serial and access code`
- `Invalid printer host`
- `Wi-Fi not ready`
- `MQTT auth rejected; verify access code`
- `MQTT username rejected`
- `MQTT transport error`
- Reboot, panic, or backtrace

## Failure Routing

User configuration:

- Wrong Wi-Fi SSID/password.
- ONX board and printer are not on the same LAN.
- Printer LAN/local mode is disabled.
- Wrong printer IP/hostname.
- Wrong printer serial number.
- Wrong or stale access code.
- Browser is opened against the setup AP address after the board already joined
  home Wi-Fi.

App/Protocol:

- A1 mini local MQTT connects but model/status is not parsed.
- Local path reports connected but UI receives no meaningful printer state.
- Access code is correct but local MQTT auth handling looks wrong.
- Camera readiness is reported incorrectly for A1 mini.
- `Local only` does not start local MQTT after Wi-Fi is connected.

UI:

- PIN is generated but not visible/readable on the display.
- Device IP or portal hint is missing or unreadable.
- Local connected state is present in snapshots but not visible on screen.
- Touch long-press does not request a PIN despite the app loop seeing touch.

Env/Serial/Flash:

- Serial capture cannot open the standard port.
- `/dev/cu.wchusbserial10` or `/dev/tty.wchusbserial10` is missing.
- `lsof` shows a serial owner that blocks capture or flash.
- Standard capture/monitor commands time out before execution because of
  Codex/macOS approval.
- Any build or flash issue if a new firmware validation step is assigned.

## Completion Evidence

This bring-up is complete only when all of these are proven:

- Home Wi-Fi connection works on the ONX board.
- Web Config is reachable on the home-network IP.
- A1 mini Local-only path connects using LAN/local mode credentials.
- Screen shows local printer status from the A1 mini.
- Web Config locks after provisioning and unlocks with a display PIN.
- Any observed failures are routed to the correct owner with exact evidence.

## 2026-06-04 A1 Mini Validation

Observed device state:

- ONX Web Config IP: `192.168.3.88`.
- Home Wi-Fi SSID: `XZOffice`.
- A1 mini IP: `192.168.3.101`.
- A1 mini serial: `0309AA552000300`.
- Source mode saved as `local_only`.
- Web Config `/api/config` reported `local_configured=true`.
- Web Config `/api/health` reported `local_connected=true`.
- Local detail reported `Idle`.
- Portal grace period expired and Web Config locked as expected.
- Long-press on the display showed a six-digit PIN.
- `/api/unlock` accepted the PIN, set `printsphere_portal_session`, and reported
  a 10 minute unlocked session.
- A cookie-authenticated `/api/health` request then reported
  `portal_locked=false` and `portal_session_active=true`.

Host-side note:

- Use `curl --noproxy '*'` for direct LAN calls to `192.168.3.88`. A previous
  local-connect attempt without bypassing proxy returned a misleading host-side
  `502 Bad Gateway` and did not save configuration.

Current conclusion:

- Wi-Fi provisioning, Web Config access, A1 mini Local-only MQTT setup, and PIN
  unlock all passed for this validation.
- No App/Protocol repair is indicated for the basic Local-only and PIN flow.
- No UI repair is indicated for PIN display based on this run.
- If a future long-press causes black screen or reboot again, collect a bounded
  serial capture around the long-press before assigning a software fix.

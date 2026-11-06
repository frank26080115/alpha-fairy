# Sony A1 II clock investigation

Tested on the physically connected **ILCE-1M2, firmware 2.01**, on November 4,
2026, using USB and the existing Windows WPD/MTP driver. No USB driver was
replaced. All new files are confined to this directory.

**Two clock-setting paths work.** The simpler one for this project's existing
`PtpIpSonyAlphaCamera` class is `0x9205` + property `0xD223`, using a PTP string
containing **`YYYYMMDDThhmmss.s`**. The fractional second is significant: omitting
it returned `0x2001` but did not change the clock in the controlled test.

| Path | Observed result |
| --- | --- |
| `0x1014` / `0x1015`, property `0x5011` | Windows returned `0x8007001f`; no data. |
| `0x1016`, property `0x5011` | Windows returned `0x8007001f`; no change observed. |
| `0x9205`, property `0xD223`, `YYYYMMDDThhmmss` | `0x2001`, but a requested +30-second change was **not applied**. |
| `0x9205`, property `0xD223`, `YYYYMMDDThhmmss.s` | `0x2001`; +30-second test read back as **+29.961 seconds** relative to the computer. |
| `0x9248`, no parameters, data IN | **Reads date/time, base UTC offset, DST flag.** |
| `0x9249`, no parameters, data OUT | **Sets date/time, base UTC offset, DST flag.** Readback within a second of the computer. |

`GetDeviceInfo` advertises `0x9248` and `0x9249`, but does **not** advertise
`0x1014`, `0x1015`, or `0x1016`. Its standard device-property list is empty.
Sony's mode-300 property list includes `0xD223`; the all-properties dataset
reports it as string type `0xffff`, with empty values. Individual legacy
`0x9203` / `0x9204` reads were unhelpful: the former failed and the latter
timed out before Sony initialization. They are unnecessary for the working path.
The Windows HRESULTs above are driver errors, not captured PTP response codes;
they do not establish what another transport or camera mode would return.

The clock was initially about 45 seconds ahead of the computer. Every offset
test ended by restoring current computer time. After the final D223 sync,
closing/reopening the probe and reading six times over ten seconds showed the
clock continuing to tick, within approximately one second of the host.
**Power-cycle retention and the underlying RTC fault were not tested.**

## Run the scripts

From the repository root, with the camera on and connected as it was during
this investigation:

```powershell
.\tools\cam_time_get_set\build.cmd
python tools/cam_time_get_set/cam_time.py get
python tools/cam_time_get_set/cam_time.py sync-d223
python tools/cam_time_get_set/cam_time.py sync
```

- `get`: read the camera clock, without setting it.
- `sync-d223`: set the camera's wall time from this computer using `0xD223`.
  This preserves the camera's area/time-zone and DST settings.
- `sync`: set date/time, area and DST from this computer using `0x9249`.
- `watch`: read six samples over ten seconds.
- `probe`: read standard support information and all Sony property descriptors.
- `test-d223`: temporarily request a +30-second offset, verify it, then restore
  correct computer time through `0x9249`. Default format is the working
  fractional-second format. Add `--d223-format seconds` to reproduce the no-op.
- `test-5011`: try the standard setter with current computer time, read back,
  and finish with a `0x9249` synchronization.

Python requires only its standard library. Building the bridge requires Visual
Studio's C++ tools and the Windows SDK, both present on this computer. The bridge
selects **ILCE-1M2** by model name and refuses multiple matching devices. It uses
the Windows driver's existing PTP session; do not send an extra OpenSession.
Default initialization matches this project's Sony mode-300 SDIO sequence.
`--no-init` skips that sequence. Each run writes a timestamped JSONL transcript;
`--log PATH` sets an explicit filename. Camera readbacks are checked after sync.
There is no automatic retry of a clock write.

A native MSYS2 installation was also found at
`C:\msys64\ucrt64\bin\gphoto2.exe`: gphoto2 2.5.32, libgphoto2 2.5.34.
Its `--auto-detect` found no devices with the current Windows driver. WSL's
Ubuntu-22.04 had neither gphoto2 nor an attached USB camera. WPD allowed the
experiments without changing drivers or USB attachment.

## Exact wire formats

### Write-only Sony property `0xD223`

Send **operation `0x9205`**, one UINT32 parameter **`0x0000D223`**, data OUT.
The data is a standard PTP STR, **not an ASCII C string**:

```text
u8 count = 18                         # 17 characters plus UTF-16 NUL
UTF-16LE "YYYYMMDDThhmmss.s\0"         # 36 bytes
                                     # 37 bytes total
```

For example, `20261104T235216.0` produced the verified +30-second change.
`.0` is sufficient; it must be present in the tested format. `0xD223` is
write-only, so verify via `0x9248`. An OK response alone is insufficient evidence.

### Read operation `0x9248`

No parameters; data IN. The observed response is **29 bytes**:

```text
u32le version = 100                   # 64 00 00 00
ASCII "YYYYMMDDThhmmss.s\0"            # 18 bytes
ASCII "+hhmm\0" or "-hhmm\0"          # 6 bytes, base offset
u8 DST                               # 0 or 1
```

The area is the base UTC offset; DST adds an hour. Actual tests used `-0800`,
DST=0. DST=1 behavior has not been exercised against this camera.

### Write operation `0x9249`

No parameters; data OUT. The successful payload is **32 bytes**:

```text
u32le version = 100
u8 set_datetime = 1
ASCII "YYYYMMDDThhmmss.s\0"
u8 set_area = 1
ASCII "+hhmm\0" or "-hhmm\0"
u8 set_dst = 1
u8 DST
```

The setter has three extra enable bytes compared with the getter. Do not echo
the getter's 29 bytes directly into the setter. The script's encoding follows
the primary implementation linked below and was verified with all flags=1;
selective flags=0 were not tested.

## Applying this to PtpIpCamera

The camera-side Sony commands and payloads above were **verified over USB**.
The firmware now implements `get_time()` and `set_time()` in `lib/PtpIpCamera`,
with serial clock commands described in [clock_commands.md](../../doc/clock_commands.md).
The new PTP/IP path has simulated-transport tests; physical PTP/IP operation
remains to be verified. No firmware was flashed during the USB investigation.

The shortest setter using the existing public API is:

```cpp
// Generate this 17-character LOCAL wall-time string at the time of the write.
// Example only; do not leave a fixed timestamp in firmware.
const char* value = "20261104T235216.0";
uint8_t payload[37] = {};
payload[0] = 18;
for (unsigned i = 0; i < 17; ++i) {
    payload[1 + 2 * i] = static_cast<uint8_t>(value[i]);
}
uint32_t property = 0xD223;
camera.wait_while_busy(0, DEFAULT_BUSY_TIMEOUT);
bool queued = camera.isOperating() && camera.canSend()
    && camera.send_oper_req(0x9205, &property, 1, payload, sizeof(payload));
// queued only means sent; poll for the response and verify with a clock read.
```

Read with `send_oper_req(0x9248, nullptr, 0, nullptr, -1)`. A firmware
implementation must track the pending clock transaction and decode its data
before normal polling overwrites the shared buffer, as the new `get_time()`
implementation does. `0x9249` can use the existing outgoing-payload API with the 32-byte
payload above. Add opcode/property constants and response handling when
integrating, rather than putting a dated payload into `init_table`.

## Evidence and sources

The JSONL files contain requests, payload bytes, response codes and, for the
Python runs, host receive timestamps and decoded clock values. The `size` /
`data` fields on a **write** record describe the bytes sent, not a camera reply.

- [Fractional D223 +30-second test and restoration](test_d223_fraction.jsonl)
- [Seconds-only D223 no-op and restoration](test_d223_offset.jsonl)
- [D223 current-time sync and readback](sync_d223_verified.jsonl)
- [0x9249 current-time sync and readback](sync_9249.jsonl)
- [Standard 0x5011 write failure](test_5011.jsonl)
- [Clock ticking after reopening the client](watch_reopened.jsonl)
- [Initialization and supported-properties probe](probe_with_timestamps.jsonl)
- [libgphoto2 Sony property definitions](https://github.com/gphoto/libgphoto2/blob/master/camlibs/ptp2/ptp.h)
  identify `0xD223` as write-only DateTimeSet.
- [alpha2go v0.3.0 clock reader](https://github.com/Knallli/alpha2go/blob/v0.3.0/ptpip/sonyinfo.go)
  and [writer](https://github.com/Knallli/alpha2go/blob/v0.3.0/ptpip/sonyadmin.go)
  provided the starting point for `0x9248` / `0x9249` layouts.
- [Microsoft WPD MTP pass-through documentation](https://learn.microsoft.com/en-us/windows/win32/wpd_sdk/supporting-mtp-extensions)
  describes the native transport used by `wpd_probe.cpp`.

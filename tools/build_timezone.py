"""Record the compiler host's UTC offset for __DATE__/__TIME__ RTC fallback."""
from datetime import datetime

Import("env")

# __DATE__/__TIME__ contain host local wall time, not UTC. Capture its offset at
# build time so a later config timezone change cannot make a valid RTC appear
# older than the firmware or seed the fallback in the wrong timezone.
offset = int(datetime.now().astimezone().utcoffset().total_seconds() // 60)
env.Append(CPPDEFINES=[("ALFY_BUILD_UTC_OFFSET_MINUTES", offset)])

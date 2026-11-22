' Started by the Windows task "RONE Release Check". No window, nothing takes focus, no prompts.
Set sh = CreateObject("WScript.Shell")
sh.Run """D:\RONE PLUGINS\rone-plugins\scripts\release-check\release-check.cmd"" --nightly --quiet", 0, True

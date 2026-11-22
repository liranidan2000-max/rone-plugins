# Registers the Windows task "RONE Release Check": every morning 07:40 (the Test Bench report lands at 08:00),
# as the signed-in user, no Claude in the loop. It runs release-check.mjs --nightly: Telegram to Liran (Kalifa bot,
# same path as the bench) only when a station turns red that was not red the run before. Read-only.
$action  = New-ScheduledTaskAction -Execute "wscript.exe" -Argument '"D:\RONE PLUGINS\rone-plugins\scripts\release-check\nightly.vbs"'
$trigger = New-ScheduledTaskTrigger -Daily -At 7:40am
$set = New-ScheduledTaskSettingsSet -StartWhenAvailable -MultipleInstances IgnoreNew `
         -ExecutionTimeLimit (New-TimeSpan -Minutes 15) -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
$who = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" -LogonType Interactive -RunLevel Limited
Register-ScheduledTask -TaskName "RONE Release Check" -Action $action -Trigger $trigger -Settings $set -Principal $who `
    -Description "Every product x every release station; Telegram when something new is missing. rone-plugins\scripts\release-check\README.md" -Force

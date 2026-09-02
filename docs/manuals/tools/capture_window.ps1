param([string]$Exe, [string]$Out, [int]$WaitSec = 8, [int]$W = 0, [int]$H = 0, [string]$Args = "", [switch]$Maximize)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class Win32 {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint nFlags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int w, int h, bool repaint);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr hWnd);
  public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
  public static IntPtr FindByPid(uint pid) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p); if (p == pid && IsWindowVisible(h)) { RECT r; GetWindowRect(h, out r); if (r.Right - r.Left > 200 && r.Bottom - r.Top > 200) { found = h; return false; } } return true; }, IntPtr.Zero);
    return found;
  }
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int cmd);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
[Win32]::SetProcessDPIAware() | Out-Null
if ($Args -ne "") { $p = Start-Process -FilePath $Exe -ArgumentList $Args -PassThru } else { $p = Start-Process -FilePath $Exe -PassThru }
$deadline = (Get-Date).AddSeconds($WaitSec)
$h = [IntPtr]::Zero
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Milliseconds 500
  $p.Refresh()
  if ($p.HasExited) { break }
  if ($p.MainWindowHandle -ne [IntPtr]::Zero) { $h = $p.MainWindowHandle }
  if ($h -eq [IntPtr]::Zero) { $h = [Win32]::FindByPid([uint32]$p.Id) }
}
if ($h -eq [IntPtr]::Zero) {
  # single-instance apps hand off to an already running process - look for any window with that name
  $others = Get-Process | Where-Object { $_.ProcessName -eq $p.ProcessName -and $_.Id -ne $p.Id }
  foreach ($o in $others) { $hh = [Win32]::FindByPid([uint32]$o.Id); if ($hh -ne [IntPtr]::Zero) { $h = $hh; $p = $o; break } }
}
if ($h -eq [IntPtr]::Zero) { Write-Output "NO WINDOW"; try { $p.Kill() } catch {}; exit 1 }
if ($W -gt 0 -and $H -gt 0) { foreach ($i in 1..4) { [Win32]::MoveWindow($h, 40, 40, $W, $H, $true) | Out-Null; Start-Sleep -Milliseconds 1200 } }
if ($Maximize) { [Win32]::ShowWindow($h, 3) | Out-Null; Start-Sleep -Milliseconds 1500 }
Write-Output ("zoomed=" + [Win32]::IsZoomed($h))
[Win32]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 800
$r = New-Object Win32+RECT
[Win32]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$ok = [Win32]::PrintWindow($h, $hdc, 2)   # PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output ("captured {0}x{1} ok={2} title={3}" -f $w, $hh, $ok, $p.MainWindowTitle)
try { $p.CloseMainWindow() | Out-Null; Start-Sleep -Milliseconds 1500; if (-not $p.HasExited) { $p.Kill() } } catch {}

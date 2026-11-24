// A stand-in for signtool.exe that PACE's wraptool runs (wraptool sign --signtool <this>).
// wraptool only knows a certificate whose private key sits on this PC. Ours sits in Azure Key
// Vault's HSM and cannot leave it, so "sign" is handed to scripts\ci\sign-windows.ps1 (the same
// signer CI uses, here with the `az login` session); every other command goes to the real
// signtool unchanged. Compiled by sign-aax.ps1 with the .NET Framework's csc.
//   RONE_REAL_SIGNTOOL  path of Microsoft's signtool.exe
//   RONE_SIGN_SCRIPT    path of sign-windows.ps1
//   RONE_SIGNTOOL_LOG   optional: one line per call
using System;
using System.Diagnostics;
using System.IO;
using System.Text;

static class SignToolAkv
{
    static string Quote(string a)
    {
        if (a.Length > 0 && a.IndexOfAny(new[] { ' ', '\t', '"' }) < 0) return a;
        var sb = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in a)
        {
            if (c == '\\') { slashes++; continue; }
            if (c == '"') { sb.Append('\\', slashes * 2 + 1); sb.Append('"'); slashes = 0; continue; }
            sb.Append('\\', slashes); slashes = 0; sb.Append(c);
        }
        sb.Append('\\', slashes * 2).Append('"');
        return sb.ToString();
    }

    static int Run(string exe, string args)
    {
        var p = Process.Start(new ProcessStartInfo(exe, args) { UseShellExecute = false });
        p.WaitForExit();
        return p.ExitCode;
    }

    static int Main(string[] args)
    {
        string all = string.Join(" ", Array.ConvertAll(args, Quote));
        string log = Environment.GetEnvironmentVariable("RONE_SIGNTOOL_LOG");
        if (!string.IsNullOrEmpty(log)) File.AppendAllText(log, DateTime.Now.ToString("s") + " " + all + Environment.NewLine);

        if (args.Length > 1 && args[0].Equals("sign", StringComparison.OrdinalIgnoreCase))
        {
            string file = args[args.Length - 1];
            if (!File.Exists(file)) { Console.Error.WriteLine("signtool-akv: no such file: " + file); return 1; }
            // The vendor's valid signature is kept by sign-windows.ps1; ours is replaced, never doubled.
            return Run("powershell.exe", "-NoProfile -ExecutionPolicy Bypass -File "
                + Quote(Environment.GetEnvironmentVariable("RONE_SIGN_SCRIPT")) + " -Require " + Quote(Path.GetFullPath(file)));
        }
        return Run(Environment.GetEnvironmentVariable("RONE_REAL_SIGNTOOL"), all);
    }
}

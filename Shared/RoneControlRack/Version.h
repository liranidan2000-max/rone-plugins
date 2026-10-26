#pragma once

// ============================================================================
// Shared/RoneControlRack - the one number every build of this folder carries.
//
// Several plugin binaries (RONE Control and every plugin with a rack) compile this folder in and meet at run
// time through RONE Control's data folder: pairing.json, settings.json, link-status.json, rack-armed.json,
// the hands over TCP 47932 (owner -> peers). Bump kCoreProtocol when one of those files or the wire format
// changes shape; a peer that reads a status written by another build can then tell, instead of guessing.
// ============================================================================

namespace RoneCtl
{
    constexpr int kCoreProtocol = 1;     // 2026-10-26: Core moved here from RoneControl/Source/Core (unchanged shapes)
}

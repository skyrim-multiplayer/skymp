#pragma once

constexpr auto g_maxPlayers = MAX_PLAYERS;

// This value should be increased each time messaging protocol changes
// regardless of is it a major or minor change. Every change is considered
// incompatible to keep protocol versions system maintainable.
constexpr auto g_messagingProtocolVersion = "1";

// Users with g_messagingProtocolVersion different to server's one must not be
// able to connect. So we use this value as SLikeNet password by default.
constexpr auto g_networkingPassword = g_messagingProtocolVersion;

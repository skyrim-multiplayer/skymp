// ==========================================
// Local SkyMP Custom Server Gamemode
// ==========================================

console.log("");
console.log("==================================================");
console.log("  SERVER IST ONLINE! WARTE AUF SPIELER...         ");
console.log("==================================================");
console.log("");

const getPlayerName = (player) => {
  if (!player) return "Unknown";
  if (typeof player.name === "string" && player.name.length > 0) return player.name;
  if (typeof player.getName === "function") {
    try {
      return String(player.getName());
    } catch (_) {
      return "Unknown";
    }
  }
  return "Unknown";
};

if (!globalThis.__skympLocalJoinLeaveHandlersRegistered) {
  globalThis.__skympLocalJoinLeaveHandlersRegistered = true;

  // Event: player joins the server.
  mp.on("playerJoin", (player) => {
    const playerName = getPlayerName(player);
    console.log(`[Login] Spieler ${playerName} hat den Server betreten!`);

    // Optional in-game welcome message (depends on current API availability).
    // if (typeof mp.sendChatMessage === "function") {
    //   mp.sendChatMessage(player, "Willkommen auf meinem neuen SkyMP Server!");
    // }
  });

  // Event: player leaves the server.
  mp.on("playerLeave", (player) => {
    const playerName = getPlayerName(player);
    console.log(`[Logout] Spieler ${playerName} hat den Server verlassen.`);
  });
}

// Keep an active interval so the runtime never becomes idle and emit heartbeat logs.
if (!globalThis.__skympLocalHeartbeatIntervalId) {
  globalThis.__skympLocalHeartbeatIntervalId = setInterval(() => {
    console.log("[Status] Server laeuft stabil...");
  }, 1800000);
}

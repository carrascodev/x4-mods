package main

import (
	"context"
	"database/sql"

	"github.com/heroiclabs/nakama-common/runtime"
)

// SectorMatchHandler implements the match handler interface for sector-based matches
type SectorMatchHandler struct{}

// SectorMatchState holds the state for a sector match
type SectorMatchState struct {
	SectorName string          `json:"sector_name"`
	Players    map[string]bool `json:"players"` // userId -> active
	Label      string          `json:"label"`
}

// MatchInit is called when a match is created or when the server starts and needs to re-initialize a match
func (m *SectorMatchHandler) MatchInit(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, params map[string]interface{}) (interface{}, int, string) {
	sectorName := "unknown"

	if sector, ok := params["sector"].(string); ok {
		sectorName = sector
		logger.Info("Creating sector match from params: %s", sectorName)
	} else {
		// Try to extract from any available context
		// In practice, the client should use metadata when calling joinMatch()
		logger.Warn("No sector name in params, using unknown")
	}

	logger.Info("Initializing sector match for: %s", sectorName)

	state := &SectorMatchState{
		SectorName: sectorName,
		Players:    make(map[string]bool),
		Label:      sectorName,
	}

	// Label for match listing (so we can find matches by sector)
	label := sectorName

	// Tick rate in Hz (10 times per second for position updates)
	tickRate := 10

	return state, tickRate, label
}

// MatchJoinAttempt is called when a player tries to join
func (m *SectorMatchHandler) MatchJoinAttempt(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, presence runtime.Presence, metadata map[string]string) (interface{}, bool, string) {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		logger.Error("Invalid match state type")
		return state, false, "internal error"
	}

	logger.Info("Player %s attempting to join sector %s", presence.GetUserId(), matchState.SectorName)

	// Allow the join
	return state, true, ""
}

// MatchJoin is called when a player successfully joins
func (m *SectorMatchHandler) MatchJoin(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, presences []runtime.Presence) interface{} {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		return state
	}

	for _, presence := range presences {
		userID := presence.GetUserId()
		matchState.Players[userID] = true
		logger.Info("Player %s joined sector %s (total: %d)", userID, matchState.SectorName, len(matchState.Players))
	}

	return matchState
}

// MatchLeave is called when a player leaves
func (m *SectorMatchHandler) MatchLeave(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, presences []runtime.Presence) interface{} {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		return state
	}

	for _, presence := range presences {
		userID := presence.GetUserId()
		delete(matchState.Players, userID)
		logger.Info("Player %s left sector %s (remaining: %d)", userID, matchState.SectorName, len(matchState.Players))
	}

	return matchState
}

// MatchLoop is called every tick
func (m *SectorMatchHandler) MatchLoop(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, messages []runtime.MatchData) interface{} {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		return state
	}

	// Forward position updates to all other players in the match
	for _, message := range messages {
		// OpCode 1 = position update
		if message.GetOpCode() == 1 {
			// Broadcast to all other players except sender
			dispatcher.BroadcastMessage(1, message.GetData(), []runtime.Presence{message}, nil, true)
		}
	}

	return matchState
}

// MatchTerminate is called when the match should end
func (m *SectorMatchHandler) MatchTerminate(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, graceSeconds int) interface{} {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		return state
	}

	logger.Info("Terminating sector match: %s", matchState.SectorName)
	return state
}

// MatchSignal handles external signals sent to the match
func (m *SectorMatchHandler) MatchSignal(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, dispatcher runtime.MatchDispatcher, tick int64, state interface{}, data string) (interface{}, string) {
	matchState, ok := state.(*SectorMatchState)
	if !ok {
		return state, "error"
	}

	logger.Info("Match signal received for sector %s: %s", matchState.SectorName, data)
	return matchState, "ok"
}

// GetSectorMatchID generates a deterministic match ID for a sector
func GetSectorMatchID(sectorName string) string {
	// Use a simple prefix to make it clear this is a sector match
	return "sector." + sectorName
}

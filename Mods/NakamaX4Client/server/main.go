package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"errors"
	"time"

	"github.com/heroiclabs/nakama-common/runtime"
)

func getServerTime(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, payload string) (string, error) {
	serverTime := map[string]int64{
		"time": time.Now().UTC().Unix(),
	}

	response, err := json.Marshal(serverTime)
	if err != nil {
		logger.Error("failed to marshal response: %v", response)
		return "", errors.New("internal error; see logs")
	}
	return string(response), nil
}

func InitModule(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule, initializer runtime.Initializer) error {
	// Register the sector match handler
	// This allows matches to be created automatically when players join with sector-based match IDs
	if err := initializer.RegisterMatch("sector_match", func(ctx context.Context, logger runtime.Logger, db *sql.DB, nk runtime.NakamaModule) (runtime.Match, error) {
		return &SectorMatchHandler{}, nil
	}); err != nil {
		logger.Error("Failed to register sector_match handler: %v", err)
		return err
	}
	logger.Info("Registered sector_match handler")

	// Register RPC functions
	if err := initializer.RegisterRpc("get_time", getServerTime); err != nil {
		return err
	}

	logger.Info("modules loaded (including sector_match handler)")
	return nil
}

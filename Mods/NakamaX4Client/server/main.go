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
	// Example
	if err := initializer.RegisterRpc("get_time", getServerTime); err != nil {
		return err
	}

	// if err := initializer.RegisterRpc("list_quests", quests.ListQuests); err != nil {
	// 	return err
	// }

	// if err := initializer.RegisterRpc("create_quest", quests.CreateQuest); err != nil {
	// 	return err
	// }

	logger.Info("modules loaded")
	return nil
}

#!/usr/bin/env python3
"""
Nakama Bridge for X4 Foundations
Connects X4 (via Named Pipes) to Nakama server (via HTTP)

This script acts as a bridge between X4's Named Pipes API and a Nakama server.
It receives commands from X4 through named pipes and translates them to HTTP requests.

Requirements:
- Python 3.6+
- requests library: pip install requests
- pywin32 library: pip install pywin32
- sn_mod_support_apis extension in X4
- Running Nakama server

Usage:
1. Start your Nakama server
2. Configure the NAKAMA_* constants below
3. Run this script: python nakama_bridge.py
4. Start X4 with the Nakama mod loaded
"""

import win32pipe
import win32file
import pywintypes
import requests
import json
import time
import threading
import logging
from typing import Dict, Optional

# Configuration
NAKAMA_HOST = "127.0.0.1"
NAKAMA_PORT = 7350
NAKAMA_SERVER_KEY = "defaultkey"
NAKAMA_BASE_URL = f"http://{NAKAMA_HOST}:{NAKAMA_PORT}"

PIPE_NAME = r"\\.\pipe\nakama_pipe"
BUFFER_SIZE = 512

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class NakamaBridge:
    def __init__(self):
        self.pipe_handle = None
        self.session_tokens: Dict[str, str] = {}  # player_name -> session_token
        self.running = False
        
    def start_pipe_server(self):
        """Start the named pipe server to communicate with X4"""
        logger.info(f"Starting named pipe server: {PIPE_NAME}")
        
        try:
            # Create named pipe
            self.pipe_handle = win32pipe.CreateNamedPipe(
                PIPE_NAME,
                win32pipe.PIPE_ACCESS_DUPLEX,
                win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
                1, BUFFER_SIZE, BUFFER_SIZE, 0, None
            )
            
            if self.pipe_handle == win32file.INVALID_HANDLE_VALUE:
                logger.error("Failed to create named pipe")
                return
                
            self.running = True
            logger.info("Named pipe created successfully, waiting for X4 connection...")
            
            while self.running:
                try:
                    # Wait for client connection
                    win32pipe.ConnectNamedPipe(self.pipe_handle, None)
                    logger.info("X4 client connected to pipe")
                    
                    # Send connection confirmation
                    self.write_to_pipe("CONNECTED")
                    
                    # Handle client messages
                    while self.running:
                        try:
                            message = self.read_from_pipe()
                            if message:
                                response = self.process_message(message)
                                if response:
                                    self.write_to_pipe(response)
                        except pywintypes.error as e:
                            if e.winerror == 109:  # Broken pipe
                                logger.info("Client disconnected")
                                break
                            else:
                                logger.error(f"Pipe error: {e}")
                                break
                                
                    # Disconnect client
                    win32pipe.DisconnectNamedPipe(self.pipe_handle)
                    
                except pywintypes.error as e:
                    logger.error(f"Pipe server error: {e}")
                    time.sleep(1)
                    
        except Exception as e:
            logger.error(f"Failed to start pipe server: {e}")
        finally:
            if self.pipe_handle:
                win32file.CloseHandle(self.pipe_handle)
                
    def read_from_pipe(self) -> Optional[str]:
        """Read message from named pipe"""
        try:
            result, data = win32file.ReadFile(self.pipe_handle, BUFFER_SIZE)
            return data.decode('utf-8').strip()
        except pywintypes.error as e:
            if e.winerror != 109:  # Ignore broken pipe
                logger.error(f"Failed to read from pipe: {e}")
            return None
            
    def write_to_pipe(self, message: str):
        """Write message to named pipe"""
        try:
            win32file.WriteFile(self.pipe_handle, message.encode('utf-8'))
        except pywintypes.error as e:
            logger.error(f"Failed to write to pipe: {e}")
            
    def process_message(self, message: str) -> Optional[str]:
        """Process message from X4 and return response"""
        logger.info(f"Received from X4: {message}")
        
        parts = message.split('|')
        command = parts[0]
        
        try:
            if command == "CONNECT":
                return "CONNECTED"
                
            elif command == "AUTH":
                player_name = parts[1]
                player_age = parts[2]
                return self.authenticate_player(player_name, player_age)
                
            elif command == "SYNC":
                player_name = parts[1]
                credits = int(parts[2])
                playtime = int(parts[3])
                ships = int(parts[4])
                faction = parts[5]
                return self.sync_player_data(player_name, credits, playtime, ships, faction)
                
            elif command == "LEADERBOARD":
                leaderboard_type = parts[1]
                player_name = parts[2]
                score = int(parts[3])
                return self.submit_leaderboard(leaderboard_type, player_name, score)
                
            elif command == "GET_LEADERBOARD":
                leaderboard_type = parts[1]
                return self.get_leaderboard(leaderboard_type)
                
            else:
                logger.warning(f"Unknown command: {command}")
                return f"ERROR|Unknown command: {command}"
                
        except Exception as e:
            logger.error(f"Error processing command {command}: {e}")
            return f"ERROR|{str(e)}"
            
    def authenticate_player(self, player_name: str, player_age: str) -> str:
        """Authenticate player with Nakama"""
        try:
            # Generate device ID
            device_id = f"{player_name}_x4_nakama_{player_age}"
            
            # Authentication request
            auth_url = f"{NAKAMA_BASE_URL}/v2/account/authenticate/device"
            headers = {
                "Content-Type": "application/json",
                "Authorization": f"Bearer {NAKAMA_SERVER_KEY}"
            }
            
            payload = {
                "id": device_id,
                "create": True,
                "username": player_name
            }
            
            response = requests.post(auth_url, headers=headers, json=payload, timeout=10)
            
            if response.status_code == 200:
                data = response.json()
                session_token = data.get("token")
                if session_token:
                    self.session_tokens[player_name] = session_token
                    logger.info(f"Player {player_name} authenticated successfully")
                    return "AUTH_SUCCESS"
                else:
                    return "ERROR|No session token in response"
            else:
                logger.error(f"Authentication failed: {response.status_code} - {response.text}")
                return f"ERROR|Authentication failed: {response.status_code}"
                
        except requests.RequestException as e:
            logger.error(f"Network error during authentication: {e}")
            return f"ERROR|Network error: {str(e)}"
            
    def sync_player_data(self, player_name: str, credits: int, playtime: int, ships: int, faction: str) -> str:
        """Sync player data to Nakama storage"""
        if player_name not in self.session_tokens:
            return "ERROR|Not authenticated"
            
        try:
            session_token = self.session_tokens[player_name]
            
            # Storage request
            storage_url = f"{NAKAMA_BASE_URL}/v2/storage"
            headers = {
                "Content-Type": "application/json",
                "Authorization": f"Bearer {session_token}"
            }
            
            player_data = {
                "credits": credits,
                "playtime": playtime,
                "ship_count": ships,
                "faction": faction,
                "last_update": int(time.time())
            }
            
            payload = {
                "objects": [{
                    "collection": "player_data",
                    "key": "stats",
                    "value": player_data
                }]
            }
            
            response = requests.put(storage_url, headers=headers, json=payload, timeout=10)
            
            if response.status_code == 200:
                logger.info(f"Player data synced for {player_name}")
                return "SYNC_SUCCESS"
            else:
                logger.error(f"Sync failed: {response.status_code} - {response.text}")
                return f"ERROR|Sync failed: {response.status_code}"
                
        except requests.RequestException as e:
            logger.error(f"Network error during sync: {e}")
            return f"ERROR|Network error: {str(e)}"
            
    def submit_leaderboard(self, leaderboard_type: str, player_name: str, score: int) -> str:
        """Submit score to Nakama leaderboard"""
        if player_name not in self.session_tokens:
            return "ERROR|Not authenticated"
            
        try:
            session_token = self.session_tokens[player_name]
            leaderboard_id = f"x4_{leaderboard_type}_leaderboard"
            
            # Leaderboard submission
            leaderboard_url = f"{NAKAMA_BASE_URL}/v2/leaderboard/{leaderboard_id}"
            headers = {
                "Content-Type": "application/json",
                "Authorization": f"Bearer {session_token}"
            }
            
            payload = {
                "score": score,
                "subscore": int(time.time())
            }
            
            response = requests.post(leaderboard_url, headers=headers, json=payload, timeout=10)
            
            if response.status_code == 200:
                logger.info(f"Leaderboard score submitted: {player_name} - {score}")
                return "LEADERBOARD_SUCCESS"
            else:
                logger.error(f"Leaderboard submission failed: {response.status_code} - {response.text}")
                return f"ERROR|Leaderboard failed: {response.status_code}"
                
        except requests.RequestException as e:
            logger.error(f"Network error during leaderboard submission: {e}")
            return f"ERROR|Network error: {str(e)}"
            
    def get_leaderboard(self, leaderboard_type: str) -> str:
        """Get leaderboard data from Nakama"""
        try:
            leaderboard_id = f"x4_{leaderboard_type}_leaderboard"
            leaderboard_url = f"{NAKAMA_BASE_URL}/v2/leaderboard/{leaderboard_id}"
            
            # Use server key for leaderboard reading
            headers = {
                "Authorization": f"Bearer {NAKAMA_SERVER_KEY}"
            }
            
            response = requests.get(leaderboard_url, headers=headers, timeout=10)
            
            if response.status_code == 200:
                data = response.json()
                # Process leaderboard data and return formatted string
                records = data.get("records", [])
                if records:
                    top_scores = []
                    for i, record in enumerate(records[:5]):  # Top 5
                        username = record.get("username", "Unknown")
                        score = record.get("score", 0)
                        top_scores.append(f"{i+1}. {username}: {score}")
                    
                    return f"LEADERBOARD_DATA|{leaderboard_type}|" + "|".join(top_scores)
                else:
                    return f"LEADERBOARD_DATA|{leaderboard_type}|No entries"
            else:
                return f"ERROR|Failed to get leaderboard: {response.status_code}"
                
        except requests.RequestException as e:
            logger.error(f"Network error getting leaderboard: {e}")
            return f"ERROR|Network error: {str(e)}"
            
    def stop(self):
        """Stop the bridge server"""
        self.running = False
        logger.info("Stopping Nakama bridge...")

def main():
    bridge = NakamaBridge()
    
    try:
        logger.info("Starting Nakama Bridge for X4 Foundations")
        logger.info(f"Nakama server: {NAKAMA_BASE_URL}")
        logger.info("Waiting for X4 connection...")
        
        bridge.start_pipe_server()
        
    except KeyboardInterrupt:
        logger.info("Received interrupt signal")
    except Exception as e:
        logger.error(f"Fatal error: {e}")
    finally:
        bridge.stop()

if __name__ == "__main__":
    main()
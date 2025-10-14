const Nakama = require('@heroiclabs/nakama-js');

async function test() {
  try {
    console.log('🔧 Testing Nakama server connection...\n');
    const client = new Nakama.Client('defaultkey', '127.0.0.1', '7350', false);
    console.log('✅ Client created');
    
    const session = await client.authenticateCustom('test-device-id', true);

    console.log('✅ Authenticated with device ID');
    console.log('   User ID:', session.user_id);
    
    const socket = client.createSocket();
    console.log('✅ Socket created');
    
    await socket.connect(session, true);
    console.log('✅ Socket connected to Nakama realtime\n');

    console.log('📡 Testing Lua RPC: get_sector_match_id...');
    const matchResponse = await client.rpc(session, 'get_sector_match_id', { sector: 'TestSector' });
    const matchId = matchResponse.payload.match_id;
    
    console.log('🎮 Joining match:', matchId);
    const match = await socket.joinMatch(matchId);
    console.log('✅ Joined match successfully');
    console.log('   Match ID:', match.match_id);
    
    // Wait a bit before disconnecting
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    console.log('🚪 Leaving match...');
    await socket.leaveMatch(matchId);
    
    socket.disconnect();
    console.log('✅ ALL TESTS PASSED! Sector match handler is working correctly! 🎉');
    return 0; // Success
  } catch (error) {
    console.error('❌ Test failed:', error.message);
  }
}


test();
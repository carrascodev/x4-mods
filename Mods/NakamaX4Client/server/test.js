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
    
    console.log('🎮 Testing sector match handler...');
    const match = await socket.joinMatch('sector_match.TestSector',undefined, {sector: 'TestSector'});
    console.log('✅ Joined sector match successfully!');
    console.log('   Match ID:', match.match_id);
    console.log('   Label:', match.label);
    console.log('   Presences:', match.presences.length, 'player(s)\n');
    
    socket.disconnect();
    console.log('✅ ALL TESTS PASSED! Sector match handler is working correctly! 🎉');
  } catch (error) {
    console.error('❌ Test failed:', error.message);
  }
}
test();
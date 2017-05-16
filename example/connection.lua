require 'ssdb'

local ssdb = ssdb
local ssdb_client, bconnected = ssdb.connect('127.0.0.1', 8888 )
if ( bconnected == false ) then
    print("server not available")
    os.exit(0)
end

ssdb_client:set( "testx", "test2" )
ssdb_client:del("testx")


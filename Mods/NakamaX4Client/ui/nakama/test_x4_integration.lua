
local function try_load(path)
    package.cpath = package.cpath .. ";./?.dll"
    local lib, err = package.loadlib(
        path, 
        "luaopen_nakama_x4")
    
    if lib then
        print("[Nakama] DLL loaded successfully, calling luaopen_nakama_x4")
        return pcall(lib(),"")  -- Call the function to get the table of functions
    else
        print("[Nakama] DLL load failed: " .. tostring(err))
        return nil
    end
end

local function load_nakama()
    
    local mod, err = try_load("D:\\Games\\X4 Foundations\\extensions\\NakamaX4Client\\ui\\nakama\\nakama_x4.dll")
        if mod then
            print("[Nakama] Loaded nakama_x4 from: " .. tostring(p))
            return mod
        else
            print("[Nakama] load attempt failed for: " .. tostring(p) .. " -> " .. tostring(err))
        end

    print("[Nakama] Failed to load nakama_x4.dll from candidates")
    return nil
end

local function safe_call(fn, ...)
    if type(fn) ~= "function" then return false, "not a function" end
    return pcall(fn, ...)
end

local function test_nakama_direct()
    local nakama = load_nakama()
    if not nakama then
        print("[Nakama] could not load module; aborting test")
        return false
    end

    -- Try basic status
    if nakama.get_status then
        local ok, s = safe_call(nakama.get_status)
        print("get_status -> " .. tostring(ok and s or s))
    end

    -- Try factory-based client
    if nakama.new_client then
        local ok, client = safe_call(nakama.new_client, {host = "127.0.0.1", port = 7350})
        if not ok or not client then
            print("ERROR: new_client failed: " .. tostring(client))
            return false
        end
        print("Client object: " .. tostring(client))
        if client.connect then
            local okc, cres = safe_call(client.connect)
            print("client.connect -> " .. tostring(okc and cres or cres))
        else
            print("client has no connect() method")
        end
        return true
    elseif nakama.init then
        local ok, r = safe_call(nakama.init, "127.0.0.1", 7350, "defaultkey")
        if not ok then
            print("ERROR: nakama.init failed: " .. tostring(r))
            return false
        end
        print("nakama.init -> " .. tostring(r))
        return true
    else
        print("ERROR: module exposes no new_client or init")
        return false
    end
end

local ok = test_nakama_direct()
print("\n=== FINAL RESULT: " .. (ok and "SUCCESS" or "FAILED") .. " ===")
return ok
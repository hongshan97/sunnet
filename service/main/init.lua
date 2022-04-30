print("[lua] run lua init.lua! version: " .. _VERSION)

function OnInit(id)
    print("[lua] main OnInit id: " .. id)

    -- print("\n")
    -- local ping1 = sunnet.NewService("ping")
    -- print("[lua] NewService ping1: " .. ping1 .. "\n")
    
    -- print("\n")
    -- local ping2 = sunnet.NewService("ping")
    -- print("[lua] NewService ping2: " .. ping2 .. "\n")

    -- sunnet.Send(ping1, pong, "start")
    
    -- print("\n")

    print("\n")
    local chat = sunnet.NewService("chat")
    print("[lua] NewService chat: " .. chat .. "\n")
end

function OnServiceMsg(source, buff)
    print("[lua] main OnServiceMsg: " .. buff)
end

function OnExit() 
    print("[lua]" .. "main OnExit")
end
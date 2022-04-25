print("[lua] run lua init.lua! version: " .. _VERSION)

function OnInit(id)
    print("[lua] main OnInit id: " .. id)

    local ping1 = sunnet.NewService("ping")
    print("[lua] NewService ping1: " .. ping1)
    
    local ping2 = sunnet.NewService("ping")
    print("[lua] NewService ping2: " .. ping2)
    
    local pong = sunnet.NewService("ping")
    print("[lua] NewService pong: " .. pong)
end

function OnServiceMsg(source, buff)
    print("[lua] main OnServiceMsg: " .. buff)
end

function OnExit() 
    print("[lua]" .. "main OnExit")
end
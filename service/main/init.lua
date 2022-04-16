print("[lua] run lua init.lua! version: " .. _VERSION)

function OnInit(id)
    print("[lua] main OnInit id: " .. id)
end

function OnExit() 
    print("[lua]" .. "main OnExit")
end
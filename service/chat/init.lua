local serviceId
local conns = {}

function OnInit(id)
    serviceId = id
    print("[lua] char OnInit id:" .. id)
    sunnet.Listen(8001, id)
end

function OnAcceptMsg(listenFd, clientFd)
    print("[lua] char OnAcceptMsg clentFd: " .. clientFd .. "  listenFd: " .. listenFd)
    conns[clientFd] = true
end

function OnSocketData(clientFd, buff)
    print("[lua] char OnScoketData " .. buff)
    for fd, _ in pairs(conns) do
        if fd ~= clientFd then
            sunnet.Write(fd, buff)
        end
    end
end

function OnSocketClose(fd)
    print("[lua] char OnSocketClose " .. fd)
    conns[fd] = nil
end
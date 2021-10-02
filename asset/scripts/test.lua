function test()
    local f = function(a, b)
        return a + b;
    end
    return f;
end

function onCreate()
    print("Creating test.lua");
end

function onDestroy()
    print("Destroying test.lua");
end

function onUpdate(deltaTime)
    print(test()(3, 4) * deltaTime .."test.lua");
end

local function fib(n)
    local a = 0
    local b = 1
    local next = b

    for _=0, n, 1 do
        next = a + b
            a = b
            b = next
    end

end

fib(99999999);

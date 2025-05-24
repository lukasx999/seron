def fib(n: int):
    a, b = 0, 1
    next = b
    for _ in range(0, n):
        # print(a)
        next = a + b
        a, b = b, next

fib(99999);

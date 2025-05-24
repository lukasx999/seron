package main

func fib(n int) {

	a := 0
	b := 1
	next := b
	for i := 0; i < n; i++ {
		next = a + b
		a = b
		b = next
	}

}

func main() {
    fib(999999999);
}

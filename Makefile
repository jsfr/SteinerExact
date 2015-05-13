all: build

build: main.go edge.go point.go tree.go cli.go
	go build -race .

clean:
	go clean

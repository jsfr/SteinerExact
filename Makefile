all: build

build: main.go
	go build -race main.go

clean:
	go clean

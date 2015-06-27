all: build

build: main.go config.go
	go build -o steinertree main.go config.go

clean:
	go clean

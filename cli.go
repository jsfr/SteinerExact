package main

import (
	"encoding/json"
	"flag"
	"os"
)

type Config struct {
	Points []Point
}

func InitConfig() Config {
	c := Config{}

	// Specifiy any flags here
	flag.Parse()
	path := flag.Arg(0)

	file, err := os.Open(path)

	if err != nil {
		panic("Error parsing file: " + err.Error())
	}

	decoder := json.NewDecoder(file)
	err = decoder.Decode(&c)

	if err != nil {
		panic("Error decoding file: " + err.Error())
	}

	return c
}

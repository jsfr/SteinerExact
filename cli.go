package main

import "flag"

type Config struct {
	Files []string
}

func InitCLI() Config {
	c := Config{}
	// Specifiy any flags here
	flag.Parse()
	c.Files = flag.Args()
	return c
}

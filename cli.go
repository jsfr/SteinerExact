package main

import "flag"

import "encoding/json"
import "os"
import "strconv"

type Config struct {
	File string
}

type JSON struct {
	N      int
	Dim    int
	Points []Point
}

func InitConfig() Config {
	c := Config{}
	// Specifiy any flags here
	flag.Parse()
	c.File = flag.Arg(0)
	return c
}

func (c *Config) ParseTree() Tree {
	file, err := os.Open(c.File)

	if err != nil {
		panic("Error parsing file: " + err.Error())
	}

	parsedJSON := JSON{}
	decoder := json.NewDecoder(file)
	err = decoder.Decode(&parsedJSON)

	if err != nil {
		panic("Error decoding file: " + err.Error())
	}

	// Check the number of points
	if parsedJSON.N != len(parsedJSON.Points) {
		panic("The number of points does not equal the given N")
	}

	// Check the dimension of the points
	for i, p := range parsedJSON.Points {
		if len(p) != parsedJSON.Dim {
			panic("Length of the point " +
				strconv.Itoa(i) +
				" did not match the given dimension")
		}
	}

	tree := Tree{}
	tree.Init(parsedJSON.Points)

	return tree
}

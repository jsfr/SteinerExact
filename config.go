package main

import (
	"encoding/json"
	"flag"
	"os"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

type config struct {
	Points     smt.Points
	MaxThreads bool
	Offset     bool
	CPUProfile string
	SortPoints bool
	Iteration  int
}

// These constants define the different ways of using the optimizer
const (
	IterationConstSmith = iota
	IterationConstSimple
)

func initConfig() config {
	c := config{}

	// Specifiy any flags here
	maxThreads := flag.Bool("maxThreads", false,
		"Sets GOMAXPROCS to the number of CPUs available."+
			" Otherwise it will be set to the default which is 1")
	offset := flag.Bool("1", false,
		"If enabled will 1-index printed points, topology vectors etc.")
	cpuprofile := flag.String("cpuprofile", "",
		"Enable and write cpu profile to file specified file")
	sort := flag.Bool("sort", false,
		"if set the terminals will be sorted such that each "+
			"conescutive pair of terminals have the "+
			"maximum distance to each other.")
	iteration := flag.String("iteration", "",
		"Select the type of iteration to use, possibilities are: "+
			"smith, simple.")

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

	switch *iteration {
	case "smith":
		c.Iteration = IterationConstSmith
	case "simple":
		c.Iteration = IterationConstSimple
	default:
		c.Iteration = IterationConstSmith

	}

	c.MaxThreads = *maxThreads
	c.Offset = *offset
	c.CPUProfile = *cpuprofile
	c.SortPoints = *sort

	return c
}
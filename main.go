package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"math"
	"os"
	"runtime"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

type config struct {
	Points     []smt.Point
	MaxThreads bool
	Offset     bool
}

func main() {
	c := initConfig()
	if c.MaxThreads {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}

	t := smt.InitTree(&c.Points)

	offset := 0
	if c.Offset {
		offset = 1
	}

	optimize(t, offset)
}

func initConfig() config {
	c := config{}

	// Specifiy any flags here
	maxThreads := flag.Bool("maxThreads", false,
		"Sets GOMAXPROCS to the number of CPUs available."+
			" Otherwise it will be set to the default which is 1")
	offset := flag.Bool("1", false,
		"If enabled will 1-index printed points, topology vectors etc.")
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

	c.MaxThreads = *maxThreads
	c.Offset = *offset

	return c
}

func optimize(t *smt.Tree, offset int) {
	w := bufio.NewWriter(os.Stdout)
	topvec := []int{0}
	upperBound := math.Inf(1)
	maxPoints := 2*t.N() - 2

	for {
		i := len(topvec) - 1
		t.Sprout(topvec[i])
		q := t.Length()
		r := t.Error()

		if q-r < upperBound {
			for r > 0.05*q {
				t.SimpleIteration()
				// t.SmithsIteration()
				q = t.Length()
				r = t.Error()
				smt.PrintTree(w, t, topvec, offset)
			}

			if len(t.Points()) >= maxPoints {
				for r > 0.05*q {
					t.SimpleIteration()
					// t.SmithsIteration()
					q = t.Length()
					r = t.Error()
				}
				if q < upperBound {
					smt.PrintTree(w, t, topvec, offset)
					upperBound = q
				}
			}
		} else {
			if topvec[i] < 2*(i+1) {
				t.Restore(topvec[i])
				topvec[i]++
				continue
			}
		}

		topvec = nextTopvec(t, topvec)
		if len(topvec) == 0 {
			os.Exit(0)
		}
	}
}

func nextTopvec(t *smt.Tree, topvec []int) []int {
	maxPoints := 2*t.N() - 2
	if len(t.Points()) < maxPoints {
		topvec = append(topvec, 0)
	} else {
		// pop all points being 2i
		for i := len(topvec) - 1; i >= 0; i-- {
			t.Restore(topvec[i])
			if topvec[i] < 2*(i+1) {
				// increment element and break
				topvec[i]++
				break
			} else {
				// remove element
				topvec = topvec[:i]
			}
		}
	}
	return topvec
}

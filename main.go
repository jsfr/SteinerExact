package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"runtime"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

type config struct {
	points []smt.Point
	procs  bool
}

func initConfig() config {
	c := config{}

	// Specifiy any flags here
	procs := flag.Bool("procs", false, "Sets GOMAXPROCS to the number of"+
		" CPUs available. Otherwise it will be set"+
		" to the default which is 1")
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

	c.procs = *procs

	return c
}

func main() {
	cfg := initConfig()
	if cfg.procs {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}
	tree := smt.InitTree(&cfg.points)
	topvec := []int{0}
	maxPoints := 2*tree.N() - 2
	w := bufio.NewWriter(os.Stdout)

	for {
		fmt.Fprintln(w, "### topvec:", topvec)

		edgeIdx := topvec[len(topvec)-1]

		tree.Sprout(edgeIdx)

		tree.DebugPrint(w)

		if len(tree.Points()) < maxPoints {
			topvec = append(topvec, 0)
		} else { // pop all points being 2i
			for i := len(topvec); i > 0; i-- {
				tree.Restore(topvec[i-1])
				if topvec[i-1] >= 2*i {
					// remove element
					topvec = topvec[:i-1]
				} else {
					// increment element and break
					topvec[i-1] = topvec[i-1] + 1
					break
				}
			}
			if len(topvec) == 0 {
				// if topvec is empty break as we have done all
				break
			}
		}
	}
}

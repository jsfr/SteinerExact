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
	Points []smt.Point
	Procs  bool
	Offset int
}

func main() {
	c := initConfig()
	if c.Procs {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}

	t := smt.InitTree(&c.Points)

	optimize(t, c.Offset)
}

func initConfig() config {
	c := config{}

	// Specifiy any flags here
	procs := flag.Bool("procs", false, "Sets GOMAXPROCS to the number of"+
		" CPUs available. Otherwise it will be set"+
		" to the default which is 1")
	offset := flag.Int("offset", 0, "Offset on indices of points, edges etc.")
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

	c.Procs = *procs
	c.Offset = *offset

	return c
}

func optimize(t *smt.Tree, offset int) {
	w := bufio.NewWriter(os.Stdout)
	maxPoints := 2*t.N() - 2
	topvec := []int{0}
	upperBound := math.Inf(1)

	for {
		edgeIdx := topvec[len(topvec)-1]
		t.Sprout(edgeIdx)

		q := t.Length()
		r := t.Error()

		if q-r < upperBound {
			for r > 0.005*q {
				t.SmithsIteration()
				q = t.Length()
				r = t.Error()
			}

			if len(t.Points()) >= maxPoints {
				for r > 0.0001*q {
					t.SmithsIteration()
					q = t.Length()
					r = t.Error()
				}
				if q < upperBound {
					smt.PrintTree(w, t, topvec, offset)
					upperBound = q
				}
			}
		}

		if len(t.Points()) < maxPoints && (q-r < upperBound) {
			topvec = append(topvec, 0)
		} else { // pop all points being 2i
			for i := len(topvec) - 1; i >= 0; i-- {
				t.Restore(topvec[i])
				if topvec[i] >= 2*(i+1) {
					// remove element
					topvec = topvec[:i]
				} else {
					// increment element and break
					topvec[i]++
					break
				}
			}
		}

		if len(topvec) == 0 {
			// if topvec is empty break as we have done all
			break
		}
	}
}

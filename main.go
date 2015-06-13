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
	Points []smt.Point
	Procs  bool
}

func main() {
	cfg := initConfig()
	if cfg.Procs {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}

	tree := smt.InitTree(&cfg.Points)

	// listAllTopologies(tree)
	// testOptimize(tree)
	optimize(tree)
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

	c.Procs = *procs

	return c
}

// func listAllTopologies(tree *smt.Tree) {
// 	w := bufio.NewWriter(os.Stdout)
// 	fmt.Fprintln(w, "### topvec: []")
// 	smt.DebugTree(w, tree)

// 	maxPoints := 2*tree.N() - 2
// 	topvec := []int{0}

// 	for {
// 		fmt.Fprintln(w, "### topvec:", topvec)

// 		edgeIdx := topvec[len(topvec)-1]

// 		tree.Sprout(edgeIdx)

// 		smt.DebugTree(w, tree)

// 		if len(tree.Points()) < maxPoints {
// 			topvec = append(topvec, 0)
// 		} else { // pop all points being 2i
// 			for i := len(topvec) - 1; i >= 0; i-- {
// 				tree.Restore(topvec[i])
// 				if topvec[i] >= 2*(i+1) {
// 					// remove element
// 					topvec = topvec[:i]
// 				} else {
// 					// increment element and break
// 					topvec[i]++
// 					break
// 				}
// 			}
// 			if len(topvec) == 0 {
// 				// if topvec is empty break as we have done all
// 				break
// 			}
// 		}
// 	}
// }

// // This function only works with the equilat4.json as it optimizes that one
// func testOptimize(tree *smt.Tree) {
// 	w := bufio.NewWriter(os.Stdout)
// 	tree.Sprout(2)
// 	smt.DebugTree(w, tree)

// 	for i := 0; i < 10000; i++ {
// 		tree.SmithsIteration()
// 	}
// 	smt.DebugTree(w, tree)
// }

func optimize(tree *smt.Tree) {
	w := bufio.NewWriter(os.Stdout)
	maxPoints := 2*tree.N() - 2
	topvec := []int{0}
	STUB := -1.0

	for {
		edgeIdx := topvec[len(topvec)-1]
		tree.Sprout(edgeIdx)

		q := tree.Length()
		r := tree.Error()

		if q-r < STUB || STUB < 0 {
		for r > 0.005*q {
			tree.SmithsIteration()
			q = tree.Length()
			r = tree.Error()
		}

		if len(tree.Points()) >= maxPoints {
			for r > 0.0001*q {
				tree.SmithsIteration()
				q = tree.Length()
				r = tree.Error()
			}
			if q < STUB || STUB < 0 {
				smt.PrintTree(w, tree, topvec)
				STUB = q
			}	
		}
		}

		if len(tree.Points()) < maxPoints && (q-r < STUB || STUB < 0) {
			topvec = append(topvec, 0)
		} else { // pop all points being 2i
			for i := len(topvec) - 1; i >= 0; i-- {
				tree.Restore(topvec[i])
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

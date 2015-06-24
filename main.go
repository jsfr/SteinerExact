package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"log"
	"math"
	"os"
	"runtime"
	"runtime/pprof"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

type config struct {
	Points     smt.Points
	MaxThreads bool
	Offset     bool
	CPUProfile string
	SortPoints bool
}

func main() {
	c := initConfig()

	if c.MaxThreads {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}

	offset := 0
	if c.Offset {
		offset = 1
	}

	if c.CPUProfile != "" {
		f, err := os.Create(c.CPUProfile)
		if err != nil {
			log.Fatal(err)
		}
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()
	}

	if c.SortPoints {
		c.Points.SortPoints()
	}

	t := smt.InitTree(&c.Points)
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
	cpuprofile := flag.String("pprof", "", "write cpu profile to file")
	sort := flag.Bool("sort", false, "if set the terminals will be sorted "+
		" such that each conescutive pair of "+
		"terminals have the maximum distance to each other.")

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
	c.CPUProfile = *cpuprofile
	c.SortPoints = *sort

	return c
}

func optimize(t *smt.Tree, offset int) {
	w := bufio.NewWriter(os.Stdout)
	topvec := make([]int, t.N()-2)
	upperBound := math.Inf(1)
	stack := make([]int, t.N()*t.N())
	len := make([]float64, 2*t.N())
	k := 1
	m := 0

	for {
		nc := 0
		for x := 0; x < 2*k+1 && k <= t.N()-3; x++ {
			topvec[k] = x
			t.Sprout(x)
			q := t.Length()
			r := t.Error()
			eps := 1e-4 * r / float64(t.N())

			if q-r < upperBound {
				for r > 5e-3*q {
					t.SmithsIteration(eps)
					q = t.Length()
					r = t.Error()
					eps = 1e-4 * r / float64(t.N())
				}

				if k >= t.N()-3 {
					for r > 1e-4*q {
						t.SmithsIteration(eps)
						q = t.Length()
						r = t.Error()
						eps = 1e-4 * r / float64(t.N())
					}
					if q < upperBound {
						smt.PrintTree(w, t, topvec, offset)
						upperBound = q
					}
				} else {
					i := nc
					nc++
					for i > 0 && len[i] < q {
						stack[m+i+1] = stack[m+i]
						len[i+1] = len[i]
						i--
					}
					i++
					stack[m+i] = x
					len[i] = q
				}
			}
			if x < 2*k {
				t.Restore(x)
			}
		}
		m = m + nc

		for nc <= 0 {
			t.Restore(topvec[k])
			k--
			if k <= 0 {
				return
			}
			nc = stack[m]
			m--
		}

		t.Restore(topvec[k])
		topvec[k] = stack[m]
		t.Sprout(topvec[k])
		stack[m] = nc - 1
		if k < t.N()-3 {
			k++
		} else {
			m--
		}
	}
}

// func optimizeOld(t *smt.Tree, offset int) {
// 	w := bufio.NewWriter(os.Stdout)
// 	topvec := []int{0}
// 	upperBound := math.Inf(1)
// 	maxPoints := 2*t.N() - 2

// 	for {
// 		i := len(topvec) - 1
// 		t.Sprout(topvec[i])
// 		q := t.Length()
// 		r := t.Error()

// 		if q-r < upperBound {
// 			for r > 0.005*q {
// 				// t.SimpleIteration()
// 				t.SmithsIteration()
// 				q = t.Length()
// 				r = t.Error()
// 			}

// 			if len(t.Points()) >= maxPoints {
// 				for r > 0.0001*q {
// 					// t.SimpleIteration()
// 					t.SmithsIteration()
// 					q = t.Length()
// 					r = t.Error()
// 				}
// 				if q < upperBound {
// 					smt.PrintTree(w, t, topvec, offset)
// 					upperBound = q
// 				}
// 			}
// 		} else {
// 			if topvec[i] < 2*(i+1) {
// 				t.Restore(topvec[i])
// 				topvec[i]++
// 				continue
// 			}
// 		}

// 		topvec = nextTopvec(t, topvec)
// 		if len(topvec) == 0 {
// 			os.Exit(0)
// 		}
// 	}
// }

// func nextTopvec(t *smt.Tree, topvec []int) []int {
// 	maxPoints := 2*t.N() - 2
// 	if len(t.Points()) < maxPoints {
// 		topvec = append(topvec, 0)
// 	} else {
// 		// pop all points being 2i
// 		for i := len(topvec) - 1; i >= 0; i-- {
// 			t.Restore(topvec[i])
// 			if topvec[i] < 2*(i+1) {
// 				// increment element and break
// 				topvec[i]++
// 				break
// 			} else {
// 				// remove element
// 				topvec = topvec[:i]
// 			}
// 		}
// 	}
// 	return topvec
// }

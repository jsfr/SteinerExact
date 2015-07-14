package main

import (
	"bufio"
	"fmt"
	"log"
	"math"
	"os"
	"runtime"
	"runtime/pprof"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

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

	optimize(t, offset, c.Iteration)
}

func optimize(t *smt.Tree, offset, iterationType int) {
	w := bufio.NewWriter(os.Stdout)
	topvec := make([]int, t.N()-2)
	upperBound := math.Inf(1)
	stack := make([]int, t.N()*t.N())
	len := make([]float64, 2*t.N())
	k := 1
	m := 0
	iterCount := 0 // Number of times doOptimize is called
	treeCount := 0 // Number of trees we optimize on at least once

	doOptimize := func(oldError float64) (float64, float64) {
		switch iterationType {
		case IterationConstSimple:
			t.SimpleIteration()
		case IterationConstSmith:
			eps := 1e-4 * oldError / float64(t.N())
			t.SmithsIteration(eps)
		default:
			panic("Iteration type unknown.")
		}
		iterCount++
		return t.Length(), t.Error()
	}

	// Special case N = 3
	if t.N() == 3 {
		q := t.Length()
		r := t.Error()
		optimized := false

		for r > 1e-4*q {
			q, r = doOptimize(r)
			if !optimized {
				treeCount++
				optimized = true
			}
		}

		smt.PrintTree(w, t, topvec, offset)
		fmt.Fprintf(w, "%v trees optimized\n"+
			"%v optimization iterations\n\n",
			treeCount, iterCount)
		return
	}

	for {
		nc := 0
		for x := 0; x < 2*k+1 && k <= t.N()-3; x++ {
			topvec[k] = x
			t.Sprout(x)
			q := t.Length()
			r := t.Error()
			optimized := false

		ITER:
			if q-r < upperBound {
				if r > 5e-3*q {
					q, r = doOptimize(r)
					if !optimized {
						treeCount++
						optimized = true
					}
					goto ITER
				}

				if k >= t.N()-3 {
					for r > 1e-4*q {
						q, r = doOptimize(r)
						if !optimized {
							treeCount++
							optimized = true
						}
					}
					if q < upperBound {
						smt.PrintTree(w, t, topvec, offset)
						fmt.Fprintf(w, "%v trees optimized\n"+
							"%v optimization iterations\n\n",
							treeCount, iterCount)
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
				fmt.Fprintf(w, "%v trees optimized\n"+
					"%v optimization iterations\n",
					treeCount, iterCount)
				w.Flush()
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

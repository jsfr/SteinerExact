package main

import (
	"bufio"
	"fmt"
	"os"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

func main() {
	cfg := smt.InitConfig()
	tree := smt.InitTree(&cfg.Points)
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

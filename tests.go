package main

import (
	"bufio"
	"os"

	"bitbucket.org/jsfrv/smt-algorithm/smt"
)

func listAllTopologies(tree *smt.Tree) {
	w := bufio.NewWriter(os.Stdout)

	maxPoints := 2*tree.N() - 2
	topvec := []int{0}
	smt.PrintTree(w, tree, []int{}, 0)

	for {
		edgeIdx := topvec[len(topvec)-1]
		tree.Sprout(edgeIdx)
		smt.PrintTree(w, tree, topvec, 0)

		if len(tree.Points()) < maxPoints {
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
			if len(topvec) == 0 {
				// if topvec is empty break as we have done all
				break
			}
		}
	}
}

// This function only works with the equilat4.json as it optimizes that one
func testOptimize(tree *smt.Tree) {
	w := bufio.NewWriter(os.Stdout)
	tree.Sprout(2)
	smt.PrintTree(w, tree, []int{2}, 0)
	eps := 1e-4 * tree.Error() / float64(tree.N())

	for i := 0; i < 10000; i++ {
		tree.SmithsIteration(eps)
		eps = 1e-4 * tree.Error() / float64(tree.N())
	}
	smt.PrintTree(w, tree, []int{2}, 0)
}

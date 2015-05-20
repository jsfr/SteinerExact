package main

import "fmt"

func main() {
	cfg := InitConfig()
	tree := InitTree(cfg.Points)
	topvec := []int{0}
	maxPoints := 2*tree.N() - 2

	tree.Print()

	for {
		fmt.Println(topvec)
		edgeIdx := topvec[len(topvec)-1]

		tree.Sprout(edgeIdx)

		tree.Print()

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

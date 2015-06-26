package main

// func listAllTopologies(tree *smt.Tree) {
// 	w := bufio.NewWriter(os.Stdout)

// 	maxPoints := 2*tree.N() - 2
// 	topvec := []int{0}
// 	smt.PrintTree(w, tree, []int{}, 0)

// 	for {
// 		edgeIdx := topvec[len(topvec)-1]
// 		tree.Sprout(edgeIdx)
// 		smt.PrintTree(w, tree, topvec, 0)

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
// 	smt.PrintTree(w, tree, []int{2}, 0)
// 	eps := 1e-4 * tree.Error() / float64(tree.N())

// 	for i := 0; i < 10000; i++ {
// 		tree.SmithsIteration(eps)
// 		eps = 1e-4 * tree.Error() / float64(tree.N())
// 	}
// 	smt.PrintTree(w, tree, []int{2}, 0)
// }

// This only works with a triangle
// func simpleOptimize(t *smt.Tree, offset int) {
// 	w := bufio.NewWriter(os.Stdout)
// 	topvec := []int{}

// 	q := t.Length()
// 	r := t.Error()

// 	for r > 0.0001*q {
// 		// eps := 1e-4 * r / float64(t.N())
// 		t.SimpleIteration()
// 		// t.SmithsIteration(eps)
// 		q = t.Length()
// 		r = t.Error()
// 		smt.PrintTree(w, t, topvec, offset)
// 	}
// }

// This is the old naive optimize
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

// Helper for the naive optimize
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

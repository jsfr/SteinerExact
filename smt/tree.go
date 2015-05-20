package smt

import (
	"bufio"
	"fmt"
	"math/rand"
)

/**
 * A tree. This contains all N regular points, as the first N entries, and all
 * N-2 Steiner points, as the N+1..2N-2 entries. The topology is then defined by
 * the edges. To easily find the edges ending at a Steiner point, the indices of
 * all edges having Steiner point i as a end point are stored in adjacencies[i]
 * as a 3-element array
 */
type Tree struct {
	n      int
	dim    int
	points []Point
	edges  []Edge
}

func (t *Tree) N() int {
	return t.n
}

func (t *Tree) Dim() int {
	return t.dim
}

func (t *Tree) Points() []Point {
	points := make([]Point, len(t.points))
	n := copy(points, t.points)
	if n != len(t.points) {
		panic("Not all points were copied.")
	}
	return points
}

func (t *Tree) Edges() []Edge {
	edges := make([]Edge, len(t.edges))
	n := copy(edges, t.edges)
	if n != len(t.edges) {
		panic("Not all edges were copied.")
	}
	return edges
}

func (t *Tree) Terminals() []Point {
	terminals := t.points[:t.n]
	points := make([]Point, len(terminals))
	n := copy(points, terminals)
	if n != len(terminals) {
		panic("Not all terminals were copied.")
	}
	return points
}

func (t *Tree) SteinerPoints() []Point {
	steiner := t.points[t.n:]
	points := make([]Point, len(steiner))
	n := copy(points, steiner)
	if n != len(steiner) {
		panic("Not all Steiner points were copied.")
	}
	return points
}

/**
 * Initializes the tree represented by the null-vector the given points must
 * contain at least 3 points, and all given points are considered as regular
 * points. Thus the number of regular points N is set to the number of points
 * passed as an argument.
 */
func InitTree(points *[]Point) Tree {
	t := Tree{}
	t.n = len(*points)
	if t.n < 3 {
		panic("Too few points to initialize.")
	}

	// we use the first point as the decided dimension
	// all other points should have the same dimension
	t.dim = len((*points)[0])
	for _, p := range *points {
		if len(p) != t.dim {
			panic("All terminals are not of the same dimension.")
		}
	}

	t.points = make([]Point, 0, 2*t.n-2)
	t.points = append(t.points, *points...)
	s := t.PertubedCentroid(0, 1, 2)
	t.points = append(t.points, s)

	e0 := InitEdge(&t, 0, t.n)
	e1 := InitEdge(&t, 1, t.n)
	e2 := InitEdge(&t, 2, t.n)

	// there are n+k-1 edges = n+(n-2)-1 = 2n-3
	t.edges = make([]Edge, 0, 2*t.n-3)
	t.edges = append(t.edges, e0, e1, e2)

	return t
}

func (t *Tree) Sprout(edgeIdx int) {
	if len(t.points) >= 2*t.n-2 {
		panic("A FST cannot contain any more Steiner points")
	}

	// Select the terminals
	p0 := t.edges[edgeIdx].P0()
	p1 := t.edges[edgeIdx].P1()
	p2 := 2 + len(t.points) - t.n // The next terminal we need to connect

	// Get the new Steiner point and its number,
	// and append it to the point list
	s := t.PertubedCentroid(p0, p1, p2)
	sIdx := len(t.points)
	t.points = append(t.points, s)

	// Create the new edges and append them to the edge list
	e1 := InitEdge(t, p0, sIdx)
	e2 := InitEdge(t, p1, sIdx)
	t.edges = append(t.edges, e1)
	t.edges = append(t.edges, e2)

	// Change the end points of the original edge
	e0 := &t.edges[edgeIdx] // Should be defined AFTER append is used
	e0.UpdateEdge(t, p2, sIdx)
}

func (t *Tree) Restore(edgeIdx int) {
	if len(t.edges) < 5 {
		panic("There are not enough edges to remove two and keep the original three")
	}

	idx := len(t.edges)

	// Get the edges we need to remove (e0, e1)
	// and the edge we need to restore (e2)
	e0 := t.edges[idx-2]
	e1 := t.edges[idx-1]
	e2 := &t.edges[edgeIdx]

	if e0.P1() != e1.P1() || e1.P1() != e2.P1() || e2.P1() != e0.P1() {
		panic("The edges do not go to the same Steiner point")
	}

	// Restore edge
	e2.UpdateEdge(t, e0.P0(), e1.P0())

	// Remove Steiner point and last two edges
	t.points = t.points[:len(t.points)-1]
	t.edges = t.edges[:idx-2]
}

func (t *Tree) PertubedCentroid(idx0, idx1, idx2 int) Point {
	p0 := t.points[idx0]
	p1 := t.points[idx1]
	p2 := t.points[idx2]
	s := make(Point, len(p0))
	for i := range s {
		s[i] = (p0[i]+p1[i]+p2[i])/3.0 +
			0.001*rand.Float64()
	}
	return s
}

func (t *Tree) Print(w *bufio.Writer) {
	fmt.Fprint(w,
		"###### BEGIN TREE ######",
		"\n### Edges ###\n")
	for i, e := range t.Edges() {
		fmt.Fprintln(w, i, ":", e)
	}
	fmt.Fprint(w, "\n\n### Terminals ###\n")
	for i, p := range t.Terminals() {
		fmt.Fprintln(w, i, ":", p)
	}
	fmt.Fprint(w, "\n### Steiner points ###\n")
	for i, p := range t.SteinerPoints() {
		fmt.Fprintln(w, t.N()+i, ":", p)
	}
	fmt.Fprint(w,
		"\n### Length ###\n", t.Length(),
		"\n###### END TREE ######\n\n")
	w.Flush()
}

func (t *Tree) Error() float64 {
	return 0.0
}

func (t *Tree) Length() float64 {
	var length float64 = 0
	ch := make(chan float64, len(t.edges))
	worker := func(e Edge) {
		ch <- e.Length()
	}

	for _, e := range t.edges {
		go worker(e)
	}

	for range t.edges {
		length = length + <-ch
	}

	return length
}

package main

import "math/rand"

// A tree. This contains all N regular points, as the first N entries, and all
// N-2 Steiner points, as the N+1..2N-2 entries. The topology is then defined by
// the edges.
type Tree struct {
	N      int
	Points []Point
	Edges  []Edge
}

// Initializes the tree represented by the null-vector the given points must
// contain at least 3 points, and all given points are considered as regular
// points. Thus the number of regular points N is set to the number of points
// passed as an argument.
func (t *Tree) Init(points []Point) {
	n := len(points)
	if n < 3 {
		panic("Too few points to initialize.")
	}

	s := t.GetPertubedCentroid(0, 1, 2)

	e0 := Edge{0, n + 1}
	e1 := Edge{1, n + 1}
	e2 := Edge{2, n + 1}

	// there are n+k-1 edges = n+(n-2)-1 = 2n-3
	edges := make([]Edge, 3, 2*t.N-3)
	edges = []Edge{e0, e1, e2}

	t.N = n
	t.Points = append(t.Points, s)
	t.Edges = edges
}

func (t *Tree) Sprout(edgeIdx, p2 int) {

	// Select the edge we split. Use a pointer for easy editing
	e0 := &t.Edges[edgeIdx]

	// Select the terminals
	p0 := e0.P0
	p1 := e0.P1

	// Get the new Steiner point and its number, and append to point list
	s := t.GetPertubedCentroid(p0, p1, p2)
	sIdx := len(t.Points)
	t.Points = append(t.Points, s)

	// Create the new edges and append them to the edge list
	e1 := Edge{p0, sIdx}
	e2 := Edge{p1, sIdx}
	t.Edges = append(t.Edges, e1)
	t.Edges = append(t.Edges, e2)

	// Change the end points of the original edge
	e0.P0 = p2
	e0.P1 = sIdx
}

func (t *Tree) Restore(edgeIdx int) {
	idx := len(t.Edges)

	// Get the edges we need to remove (e0, e1)
	// and the edge we need to restore (e2)
	e0 := t.Edges[idx-1]
	e1 := t.Edges[idx-2]
	e2 := &t.Edges[edgeIdx]

	if e0.P1 != e1.P1 || e1.P1 != e2.P1 || e2.P1 != e0.P1 {
		panic("The edges does not go to the same Steiner point")
	}

	// Restore edge
	e2.P0 = e0.P0
	e2.P1 = e1.P0

	// Remove Steiner point and last two edges
	t.Points = t.Points[:len(t.Points)-1]
	t.Edges = t.Edges[:idx-2]

}

func (t *Tree) GetPertubedCentroid(idx0, idx1, idx2 int) Point {
	p0 := t.Points[idx0]
	p1 := t.Points[idx1]
	p2 := t.Points[idx2]
	p := make(Point, len(p0))
	for i := range p {
		p[i] = (p0[i]+p1[i]+p2[i])/3.0 +
			0.001*rand.Float64()
	}
	return p
}

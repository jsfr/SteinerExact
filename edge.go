package main

import "math"

// An edge is defined as the index of the two points defining is begin and end
// points in a tree. The edge should always follow the convention that P1 < P2.
type Edge struct {
	P0 int
	P1 int
}

func (e *Edge) Length(t *Tree) float64 {
	p0 := t.Points()[e.P0]
	p1 := t.Points()[e.P1]

	dist := 0.0
	for i := range p0 {
		diff := p0[i] - p1[i]
		dist = dist + (diff * diff)
	}
	dist = math.Sqrt(dist)
	return dist
}

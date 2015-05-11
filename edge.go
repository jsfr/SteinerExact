package main

import (
	"math"
)

// An edge is defined as the index of the two points defining is begin and end
// points in a tree. The edge should always follow the convention that P1 < P2.
type Edge struct {
	P1 int
	P2 int
	T  *Tree
}

func (e *Edge) Length() float64 {
	// Remeber that we 0-index the slices, but that point indexes start a 1!
	p1 := e.T.Points[e.P1-1]
	p2 := e.T.Points[e.P2-1]
	AssertDim(p1, p2)

	dist := 0.0
	for i := range p1 {
		diff := p1[i] - p2[i]
		dist = dist + (diff * diff)
	}
	dist = math.Sqrt(dist)
	return dist
}

package main

// A single regular or Steiner point Dim[i]
// stores the coordinate of the i-th dimension
type Point struct {
	Dim []float64
}

// A single edge. EndPoints contain the indexes of the end points for the
// edge. Length should be the euclidean length of the edge.
type Edge struct {
	EndPoints [2]int
	Length    float64
}

// A tree. This contains all N regular points, as the first N entries, and all
// N-2 Steiner points, as the N+1..2N-2 entries, in Points. The topology is then
// defined by the edges in Edges
type Tree struct {
	Points []Point
	Edges  []Edge
}

func (t *Tree) Init(N int) {
	// Init tree with null vector
}

func (t *Tree) Add() {
	// Add a point
}

func (t *Tree) Remove() {
	// Removes a RPoint and any extra SPoints created
}

func (t *Tree) Move() {
	// Puts the Rpoint another place in the tree by attaching its Steiner point somewhere else
}

func main() {
}

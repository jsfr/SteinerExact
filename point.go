package main

import "math/rand"

type Point []float64

func AssertDim(p1, p2 Point) {
	if len(p1) != len(p2) {
		panic("Dimension of points were not the same.")
	}
}

func GetPertubedCentroid(p1, p2, p3 Point) Point {
	p := make(Point, len(p1))
	for i := range p {
		p[i] = (p1[i]+p2[i]+p3[i])/3.0 +
			0.001*rand.Float64()
	}
	return p
}

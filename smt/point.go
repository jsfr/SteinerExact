package smt

import "math"

// Point represents a single point.
type Point []float64

// Points is simply a list of points
type Points []Point

// Subtract subtracts points p1 from p0 and returns a new point
func (p0 Point) Subtract(p1 Point) Point {
	p2 := make(Point, len(p0))
	for i := range p0 {
		p2[i] = p0[i] - p1[i]
	}
	return p2
}

// DotProduct gives the dot product of p0 and p1.
func DotProduct(p0, p1 Point) float64 {
	var product float64
	for i := range p0 {
		product = product + p0[i]*p1[i]
	}
	return product
}

// SortPoints sorts the given points such that every consecutive pair or points
// is as far away from each other as possible.
func (points *Points) SortPoints() {
	sortedPoints := make(Points, len(*points))
	sortedPoints[0] = (*points)[0]
	*points = (*points)[1:]

	for i := 0; i < len(sortedPoints)-1; i++ {
		bestIdx := 0
		var bestDist float64
		for j, p1 := range *points {
			var dist float64
			dist = math.Sqrt(squaredDistance(sortedPoints[i], p1))
			if dist > bestDist {
				bestIdx = j
				bestDist = dist
			}
		}
		sortedPoints[i+1] = (*points)[bestIdx]
		*points = append((*points)[:bestIdx], (*points)[bestIdx+1:]...)
	}

	*points = sortedPoints
}

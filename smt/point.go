package smt

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

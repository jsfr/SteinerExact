package smt

type Point []float64

// func (p *Point) Move(t *Tree, diff *[]float64) {
// 	if len(*p) != len(*diff) {
// 		panic("Point and the vector we move it by are not of same dimensionality.")
// 	}

// 	for i := range *p {
// 		(*p)[i] = (*p)[i] + (*diff)[i]
// 	}

// 	// Update all edge lengths of edges next to this point
// }

func (p0 *Point) Subtract(p1 *Point) Point {
	p0.AssertDim(p1)
	p2 := make(Point, len(*p0))
	for i := range *p0 {
		p2[i] = (*p0)[i] - (*p1)[i]
	}
	return p2
}

func (p0 *Point) Product(p1 *Point) float64 {
	p0.AssertDim(p1)
	var product float64 = 0
	for i := range *p0 {
		product = product + (*p0)[i]*(*p1)[i]
	}
	return product
}

func (p0 *Point) AssertDim(p1 *Point) {
	if len(*p0) != len(*p1) {
		panic("The points are do not have the same length.")
	}
}

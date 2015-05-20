package smt

type Point []float64

func (p *Point) Move(t *Tree, diff *[]float64) {
	if len(*p) != len(*diff) {
		panic("Point and the vector we move it by are not of same dimensionality.")
	}

	for i := range *p {
		(*p)[i] = (*p)[i] + (*diff)[i]
	}

	// Update all edge lengths of edges next to this point
}

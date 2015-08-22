package smt

import "math"

// Global variables for optimization
var q [3]float64
var _B [][3]float64
var _C [][]float64
var val []int
var leafQ stack
var eqnStack stack

// SmithsIteration runs a single of iteration as proposed by Smith.
// TODO: This implementation is more or less shamelessly copied from
// Smiths implementation. If there is time, it would be nice to try implementing
// the gaussian elimination better, and possibly using goroutines. E.g. we could
// have a goroutine for each dimension as these are actually separate.
func (t *Tree) SmithsIteration(epsilon float64) {

	// Set up equations
	// t.points[t.n+i][] = sum_{j}( _B[i][j] * t.points[j][] ) + _C[i]
	size := 2*t.n - 2

	if _B == nil || _C == nil {
		_B = make([][3]float64, size)
		_C = make([][]float64, size)
		for i := range _C {
			_C[i] = make([]float64, t.dim)
		}
		val = make([]int, size)
		leafQ = make(stack, 0, size)
		eqnStack = make(stack, 0, size)
	} else {
		for i := 0; i < size; i++ {
			for j := range _B[i] {
				_B[i][j] = 0
			}
			for j := range _C[i] {
				_C[i][j] = 0
			}
			val[i] = 0
			leafQ = leafQ[:0]
			eqnStack = eqnStack[:0]
		}
	}

	// Prepare equations
	for i := range t.points[t.n:] {
		adj := t.adjacencies[i]
		sIdx := i + t.n
		s := t.points[sIdx]
		p0 := t.points[adj[0]]
		p1 := t.points[adj[1]]
		p2 := t.points[adj[2]]

		q[0] = 1 / (math.Sqrt(s.squaredDistance(p0)) + epsilon)
		q[1] = 1 / (math.Sqrt(s.squaredDistance(p1)) + epsilon)
		q[2] = 1 / (math.Sqrt(s.squaredDistance(p2)) + epsilon)

		sum := q[0] + q[1] + q[2]
		q[0] = q[0] / sum
		q[1] = q[1] / sum
		q[2] = q[2] / sum

		for a := 0; a < 3; a++ {
			b := adj[a]
			c := q[a]
			if b >= t.n {
				val[i]++
				_B[sIdx][a] = c
			} else {
				for m := range _C[sIdx] {
					_C[sIdx][m] = _C[sIdx][m] + t.points[b][m]*c
				}
			}
		}

		if val[sIdx-t.n] <= 1 {
			leafQ.Put(sIdx)
		}
	}

	// Eliminate leaves
	for leafQ.Length() > 1 {
		sIdx := leafQ.Pop()
		val[sIdx-t.n]--
		adj := t.adjacencies[sIdx-t.n]

		eqnStack.Put(sIdx)

		j := -1
		for i := range _B[sIdx] {
			if _B[sIdx][i] != 0 {
				j = i
			}
		}
		if j < 0 {
			panic("No neighbor found")
		}

		q0 := _B[sIdx][j]
		j = adj[j]
		val[j-t.n]--
		if val[j-t.n] == 1 {
			leafQ.Put(j) // New leaf?
		}

		adj = t.adjacencies[j-t.n]
		l := -1
		for i := 0; i < 3; i++ {
			if adj[i] == sIdx {
				l = i
			}
		}
		if l < 0 {
			panic("No neighbor found")
		}

		q1 := _B[j][l]
		_B[j][l] = 0
		c := 1 / (1 - q0*q1)

		for i := range _B[j] {
			_B[j][i] = c * _B[j][i]
		}

		for i := range _C[j] {
			_C[j][i] = c * (_C[j][i] + q1*_C[sIdx][i])
		}

	}

	// Solve 1-vertex tree
	i := leafQ[0]
	copy(t.points[i], _C[i])

	// _Backsolve
	for eqnStack.Length() > 0 {
		i = eqnStack.Pop()
		l := -1
		for j := range _B[i] {
			if _B[i][j] != 0 {
				l = j
				break
			}
		}
		if l < 0 {
			panic("No neighbor found")
		}

		q0 := _B[i][l]
		adj := t.adjacencies[i-t.n]
		l = adj[l]
		for j := range t.points[i] {
			t.points[i][j] = _C[i][j] + q0*t.points[l][j]
		}
	}
}

// SimpleIteration runs a single simple iteration where all points are updated
// one at a time. Each Steiner point is calculated by finding the
// Fermat-Torricelli point.
func (t *Tree) SimpleIteration() {
	n := len(t.points) - t.n
	for i := 0; i < n; i++ {
		sIdx := t.n + i
		adj := t.adjacencies[i]
		t.points[sIdx] = fermatTorricelliPoint(adj[0], adj[1], adj[2], t)
	}
}

package smt

type stack []int

func (s stack) Length() int { return len(s) }

func (s stack) Empty() bool { return len(s) == 0 }

func (s stack) Peek() int   { return s[len(s)-1] }

func (s *stack) Put(i int)  { (*s) = append((*s), i) }

func (s *stack) Pop() int {
  d := (*s)[len(*s)-1]
  (*s) = (*s)[:len(*s)-1]
  return d
}

# About #
This is the original implementation of the algorithm described by Smith, and
also a version which removes a few buggy lines and changes the the output of the
implementation to allow counting trees.

# Building #
Simply run `make`

# Usage #
The program expects its input tree on `STDIN` and can thus be used by calling
e.g. `cat example.def | ./old_steinertree`. If one wants to use the trees in the
experiments (which are in JSON-format) one can convert them to Smith's format
using the script in `experiments/helpers/convert_json.sh` so one could e.g. do
`convert_json.sh example.json | ./old_steinertree`.

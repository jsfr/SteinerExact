# About #
The experiments are used to compare the performance of the new implementation
with the old one. it can be run by calling `make` in the directory of which one
wishes to perform. So if one wishes to run all experiments `make` should be run
in the the outer directory. If one only wants to run the Carioca set one should
run `make` in that directory. Finally if one only wish to run the new
implementation with the simple iteration and sorting of terminals one should run
`make` in the directory `Carioca/SimpleSort`.

The experiments log the results in the iteration directories of the sets as json
files in the format:

    {
        "nodes": How many terminals does this tree have?
        "trees": How many trees have we run at least on optimize on?
        "iterations": How many iterations of optimize has been run?
        "time": How many seconds did the tree take?
        "success": Did we finish within the given time (12h)?
    }

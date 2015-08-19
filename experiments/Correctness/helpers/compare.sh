#!/bin/bash

for f in ../Instances/*json; do
	basename="$(basename $f .json).txt"

	geosteinerLen=`grep length ../GeoSteiner/$basename | tail -n 1 | tr -s ' ' | cut -d ' ' -f 9 | cut -c 1-9`
	smithNewLen=`grep -A 1 Length ../SmithNew/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	smithNewSortLen=`grep -A 1 Length ../SmithNewSort/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	simpleLen=`grep -A 1 Length ../Simple/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	simpleSortLen=`grep -A 1 Length ../SimpleSort/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`

	diffGeosteiner=$(qalc -t $geosteinerLen - $geosteinerLen)
	diffSmithNew=$(qalc -t $smithNewLen - $geosteinerLen)
	diffSmithNewSort=$(qalc -t $smithNewSortLen - $geosteinerLen)
	diffSimple=$(qalc -t $simpleLen - $geosteinerLen)
	diffSimpleSort=$(qalc -t $simpleSortLen - $geosteinerLen)
	diff=$(qalc -t "$diffGeosteiner + $diffSmithNew + $diffSmithNewSort + $diffSimple + $diffSimpleSort >= 0.00001")

	if [ $diff -eq 1 ]; then
    	echo "INSTANCE $basename"
	    echo "LENGTH GeoSteiner      $geosteinerLen"
    	echo "LENGTH SmithNew        $smithNewLen"
    	echo "LENGTH SmithNewSort    $smithNewSortLen"
	    echo "LENGTH Simple          $simpleLen"
	    echo "LENGTH SimpleSort      $simpleSortLen"
    	echo "DIFFGEO GeoSteiner     $diffGeosteiner"
	    echo "DIFFGEO SmithNew       $diffSmithNew"
	    echo "DIFFGEO SmithNewSort   $diffSmithNewSort"
    	echo "DIFFGEO Simple         $diffSimple"
    	echo "DIFFGEO SimpleSort     $diffSimpleSort"
    	echo ""
	fi
done

#!/bin/perl

use strict;
use warnings;
use utf8;
use Statistics::Basic qw(:all);

my ($iterations, $filename) = @ARGV;

open(my $fh, '<', $filename) or die;

print "\nRunning benchmark with $iterations number " .
    "of iterations for each program\n";

while (my $program = <$fh>) {
    chomp $program;
    my @times;
    for (my $i = 0; $i < $iterations; $i++) {
        my $time = `{ /usr/bin/time -f '%U' $program; } 2>&1 | tail -1`;
        push @times, $time;
    }
    my $mean = mean(@times);
    my $stddev = stddev(@times);
    my $variance = variance(@times);
    print "\nCmd:\t\"$program\"" .
        "\nMean:\t$mean" .
        "\nStdDev:\t$stddev" .
        "\nVar:\t$variance\n";
}

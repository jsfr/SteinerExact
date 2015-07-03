#!/bin/perl

use strict;
use warnings;
use utf8;
use Statistics::Basic qw(mean stddev);
use List::MoreUtils qw(pairwise);

my ($iterations, $dir) = @ARGV;
my $time = "/usr/bin/time -f '%U'";
my $convert_def = "grep ',' | awk '{for (i=2; i<NF-1; i++) printf \$i \" \"; " .
    "print \$(NF-1) }; END {print NR,NF-2}' | sed 's/,//g;1h;1d;\$!H;\$!d;G'";

opendir my $dh, "$dir";
my @files = grep { $_ ne '.' && $_ ne '..' } readdir $dh;
closedir $dh;

my @out = ();
for my $file (@files) {
    my @cmds = (
        "$time ../steinertree -sort -iteration=simple $dir/$file",
        "$time ../steinertree -iteration=simple $dir/$file",
        "$time ../steinertree -sort -iteration=smith $dir/$file",
        "$time ../steinertree -iteration=smith $dir/$file",
        "cat $dir/$file | $convert_def | $time ./old_steinertree"
        );
    my @names = (
        "simple-sort",
        "simple",
        "smith-sort",
        "smith",
        "original"
        );
    my @means = ();
    my @stddevs = ();
    for my $cmd (@cmds) {
        my @times = ();
        for (my $i = 0; $i < $iterations; $i++) {
            my @output = split /\n/, `{ $cmd; } 2>&1 | tail -3`;
            push @times, $output[2];
        }
        my $mean = mean @times;
        my $stddev = stddev @times;
        push @means, $mean;
        push @stddevs, $stddev;
    }

    my @meansL = map length, @means;
    my @stddevsL = map length, @stddevs;
    my @namesL = map length, @names;

    my @lengths = pairwise { $a > $b ? $a : $b } @meansL, @stddevsL;
    @lengths = pairwise { $a > $b ? $a : $b } @namesL, @lengths;
    @lengths = map "%$_.2f", @lengths;

    printf
        "\n instance:  %s" .
        "\niteration:" . ("  %s" x @names) .
        "\n     mean:  " . (join "  ", @lengths) .
        "\n   stddev:  " . (join "  ", @lengths) .
        "\n", $file, @names, @means, @stddevs;
}

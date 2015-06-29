#!/bin/perl

use strict;
use warnings;
use utf8;
use Statistics::Basic qw(:all);

my ($iterations) = @ARGV;
my $time = "/usr/bin/time -f '%U'";

my $root = "examples";
opendir(my $dh, $root);
my @dirs = grep {-d "$root/$_" && ! /^\.{1,2}$/} readdir($dh);
closedir($dh);

for my $dir (@dirs) {
	opendir(my $dh, "$root/$dir");
	while (my $file = readdir($dh)) {
		my @cmds = ();
=comment
		push @cmds, "$time ./steinertree -sort -iteration=simple $root/$dir/$file";
		push @cmds, "$time ./steinertree -iteration=simple $root/$dir/$file";
		push @cmds, "$time ./steinertree -sort -iteration=smith $root/$dir/$file";
		push @cmds, "$time ./steinertree -iteration=smith $root/$dir/$file";
=cut
		push @cmds, "cat $root/$dir/$file | grep ',' | awk '{for (i=2; i<NF-1; i++) printf \$i \" \"; print \$(NF-1) }; END {print NR,NF-2}' | sed 's/,//g;1h;1d;\$!H;\$!d;G' | $time old/old_steinertree";
		for my $cmd (@cmds) {
			my @times = ();
			for (my $i = 0; $i < $iterations; $i++) {
				my $time = `{ $cmd; } 2>&1 | tail -1`;
				push @times, $time;
			}
	    	my $mean = mean(@times);
		    my $stddev = stddev(@times);
    		my $variance = variance(@times);
		    print "\ncmd:\t\"$cmd\"" .
    		    "\nmean:\t$mean" .
        		"\nstddev:\t$stddev\n";
		}
	}
	closedir($dh);
}

=comment
	open(my $fh, '<', $filename) or die;

	print "\nrunning benchmark with $iterations number " .
    	"of iterations for each program...\n";

	while (my $program = <$fh>) {
    	chomp $program;
	    $program =~ s/\{\{time\}\}/$cmd/;
    	my @times;
	    for (my $i = 0; $i < $iterations; $i++) {
    	    my $time = `{ $program; } 2>&1 | tail -1`;
        	push @times, $time;
	    }
    	my $mean = mean(@times);
	    my $stddev = stddev(@times);
    	my $variance = variance(@times);
	    print "\ncmd:\t\"$program\"" .
    	    "\nmean:\t$mean" .
        	"\nstddev:\t$stddev\n";
	}
=cut

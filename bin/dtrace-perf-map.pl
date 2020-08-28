#!/usr/bin/perl

use warnings;
no warnings 'portable';
use strict;

if (@ARGV != 1) {
    print "\nUsage: $0 perf-*.map\n";
    exit;
}

my $map_file = $ARGV[0];
my @map_entries;

open(my $fh, '<:encoding(UTF-8)', $map_file) or die "Could not open file '$map_file' $!";

# Create map of perf-*.map file entries.
while (my $row = <$fh>) {
  chomp $row;
  my @parts = split / /, $row;
  my $address = hex($parts[0]);
  my $size = hex($parts[1]);
  my $method = $parts[2];
  push @map_entries, [$address, $size, $method];
}

# Sort to allow for binary search
@map_entries = sort { $a->[0] <=> $b->[0] } @map_entries;

# Process STDIN and replace unkown methods with these provided by the perf-*.map file.
while (my $row = <STDIN>) {
    chomp $row;
    if ($row =~ "^              0x(.+)") {
        my $addr = hex($1);
        my $size = -1;
        # binary search map entries
        my ($l, $h) = (0, 0 + @map_entries);
        while (($h - $l) > 1) {
            my $m = int(($h + $l) / 2);
            my $entry_addr = $map_entries[$m][0];
            if ($entry_addr < $addr) {
                $l = $m;
            } else {
                $h = $m;
            }
        }
        my $entry_addr = $map_entries[$l][0];
        my $entry_size = $map_entries[$l][1];
        my $method =     $map_entries[$l][2];
        if ($addr >= $entry_addr && $addr <= ($entry_addr + $entry_size)) {
            $row = "              $method";
        }
    }
    print "$row\n"; 
}

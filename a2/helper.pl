#!/usr/bin/perl

open my $GNUPLOT, "|/usr/bin/gnuplot -persist 2>/dev/null";
open my $PLOTDATA, ">>plot.dat";

#Initialize x and y axis labels, leave y blank since its yet to be assigned
my $xlabel = "Sample #";
my $ylabel = "";

# Gnuplot Script
my $gplot = <<GPLOT;
set title "Tapplot Graph"
set terminal x11 enhanced font ",18"
set xlabel "$xlabel"
plot "plot.dat" using 1:2 with points lw 2 title "$ylabel"
pause -1
reread
GPLOT

my $sample = 0;
while (<STDIN>) {
    if (/^([\w\s]+)=(-?\d+)\s*/) {
        # Extract field title from input
        my $name = $1;
        my $value = $2;
        printf $PLOTDATA "%d %d\n", $sample++, $value; # Sample number and value of the specified field
        $PLOTDATA->flush;
        
        # Set the label for the y axis if it's not already set
        if (!$ylabel) {
            $ylabel = $name;
            $gplot =~ s/plot "plot.dat" using 1:2 with points lw 2 title ""/plot "plot.dat" using 1:2 with points lw 2 title "$ylabel"/;
        }
        
        print $GNUPLOT $gplot;
        $GNUPLOT->flush;
    }
}

close $GNUPLOT;
close $PLOTDATA;

# Delete the plot.dat file so we can run multiple times and not mess up the graph
unlink "plot.dat";
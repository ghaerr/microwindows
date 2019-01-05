#!/bin/perl

# This is a little perl script that converts XPM colors from
# X colors to their true RGB format

# Change this first, if you rgb.txt file lives somewhere else 
$RFILE="/usr/X11R6/lib/X11/rgb.txt";

# First, Open up the RGBFILE, and make a hash table

open (RGBIN, $RFILE) || die "Can not file $RGBFILE!";

while(<RGBIN>)
{
    chop($_);

    $firstchar = substr($_, 0, 1);

    if ($firstchar ne "!")
    {
	($r, $g, $b, $val) = /\s*(\d*)\s*(\d*)\s*(\d*)\s*(\w*)/;
	$rgbvals{$val} = sprintf("#%2.2x%2.2x%2.2x", $r, $g, $b);
    }
}

close(RGBIN);

# Ok then, are we in batch mode or individual mode?

if ($ARGV[0] eq "--batch")
{
    $srcdir = $ARGV[1];
    $destdir = $ARGV[2];

    # Batch mode, open up directory and fire 
    opendir(INDIR, $srcdir) || die "Directory $ARGV[1] doesn't exist";
    my @xpms = readdir(INDIR);
    closedir(INDIR);

    foreach $xpmfile (@xpms)
    {
	if ($xpmfile =~ /\.xpm/g)
	{
	    print "Converting $srcdir/$xpmfile to $destdir/$xpmfile...\n"
	    &convert_xpm("$srcdir/$xpmfile", "$destdir/$xpmfile");
	}
    }
}
else
{
    # regular mode
    &convert_xpm($ARGV[0], $ARGV[1]);
}


sub convert_xpm
{
    # Open up the file to convert it 

    $filea = $_[0];
    $fileb = $_[1];

    print"Writing from $filea to $fileb...\n";

    open(XPMIN, $filea) || die "Could not file $filea for processing...\n";
    open(XPMOUT,">$fileb") || die "Could not open file $fileb for writing...\n";

    $xpmline = <XPMIN>;
    
    die "$fila is Not an XPM file!\n" if (!($xpmline =~ /\/\* XPM \*\/\n/));
    
    $firstchar = substr($xpmline, 0, 1);   
    
    while($firstchar ne "\"")
    {
	print XPMOUT $xpmline;
	$xpmline = <XPMIN>;
	$firstchar = substr($xpmline, 0, 1);   
    }
    
    # Now we have the numbers.  Grab em! 
    
    ($width, $height, $colors, $chars) = ($xpmline =~ /\"(\d*) (\d*) (\d*) (\d*)\",\n/);

    print XPMOUT $xpmline;
    
    # Now read in the appropriate number of colors

    for($i = 0; $i < $colors; $i++)
    {
	$xpmline = <XPMIN>;
	chop($xpmline);
	
	($str, $val, $color) = ($xpmline =~ /\"(.+)\s+(\w*)\s+(.+)\"/);
	
	$firstchar = substr($color, 0, 1); 
	
	if ($firstchar ne "#")
	{
	    $color = $rgbvals{$color} if ($color ne "None");
	}
	
	print XPMOUT "\"$str  $val $color\",\n";
    }
    
    while(<XPMIN>)
    {
	print XPMOUT $_;
    }
    
    close(XPMIN);
    close(XPMOUT);
}




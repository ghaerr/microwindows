#!/usr/bin/perl

# A quick perl script that reads the /usr/include/X11/keysymdef.h file 
# and constructs a header file for easy comparision with an incoming string 

my($header) = "$ARGV[0]/keysymdef.h";
my($count) = 0;

open(FILE, $header) or die "Oops, couldn't open $header";

print "/* X Keysym header file    */\n";
print "/* Automatically generated */\n";
print "\n";
print "struct {\n";
print "char str[30];\n";
print "KeySym keysym;\n";
print "} nxKeyStrings[] = {\n";

while(<FILE>) {
    if (/#define (\w*)\s*(\w*)/) {
	my($keysym) = $1;
	my($value) =  $2;
	
        $keysym =~ /XK_(.*)/;

	if ($count != 0) {
	    print ",\n";
	}
	print "{\"$1\", $value }";
	$count++;
    }
}

print "\n}\;\n";
print "#define NX_KEYSYMSTR_COUNT $count\n";

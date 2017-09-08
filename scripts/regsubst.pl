#!/usr/bin/perl
#
# This script is documented in Perl Pod, you can pretty-print it using
#
# 	$ perldoc scripts/regsubst.pl.
#
# The real code starts after "=cut" below.

=head1 NAME

regsubst.pl - helper script to make use of defines

=head1 SYNOPSIS

B<regsubst.pl> [B<-I> I<includepath>] I<filename>

=head1 DESCRIPTION

B<regsubst.pl> parses the given file recursively for #include directives and
then substitutes raw hex values in I<filename> by definitions found and prints
the result to stdout.

This is targeted to make i.MX DCD tables more readable but for sure can be used
elsewhere too.

=head1 BUGS

B<regsubst.pl> is dumb and so might replace values to aggressively. So better
double check the result.

=head1 OPTIONS

=over 4

=item B<-I> I<includepath>

Add I<includepath> to the list of search paths for include files.

=back

=head1 EXAMPLE

First you have to add the right #include directives to your file:

 $ cat flash-header-myboard.imxcfg
 soc imx6
 loadaddr 0x20000000
 dcdofs 0x400
 
 #include <mach/imx6-ddr-regs.h>
 #include <mach/imx6dl-ddr-regs.h>
 
 wm 32 0x020e0774 0x000C0000
 wm 32 0x020e0754 0x00000000
 ...

Then you can process the file with B<regsubst.pl>:

 $ scripts/regsubst.pl -I arch/arm/mach-imx/include flash-header-myboard.imxcfg
 soc imx6
 loadaddr 0x20000000
 dcdofs 0x400
 
 #include <mach/imx6-ddr-regs.h>
 #include <mach/imx6dl-ddr-regs.h>
 
 wm 32 MX6_IOM_GRP_DDR_TYPE 0x000C0000
 wm 32 MX6_IOM_GRP_DDRPKE 0x00000000
 ...

If the result looks ok, you can replace the file:

 $ scripts/regsubst.pl -I arch/arm/mach-imx/include flash-header-myboard.imxcfg > u
 $ mv u flash-header-myboard.imxcfg

=cut

use strict;
use warnings;
use Getopt::Long qw(GetOptions);

my @includepaths;

GetOptions('I=s' => \@includepaths);

my %regnamemap;

sub scan {
	my @incfiles;

	push @incfiles, @_;

	foreach my $i (@incfiles) {
		foreach my $incpath (@includepaths) {
			if (-e "$incpath/$i") {
				open(my $fd, "<", "$incpath/$i") || die "Failed to open include file $incpath/$i";

				while (<$fd>) {
					if (/^\s*#\s*include\s+["<](.*)[">]/) {
						push @incfiles, $1;
					};

					if (/^\s*#\s*define\s+([A-Z_0-9]*)\s+(0x[0-9a-f]+)/) {
						my $regname = $1;
						my $regaddrre = $2 =~ s/^0x0*/0x0\*/r;
						$regnamemap{$regaddrre} = $regname;
					};
				};

				close($fd);

				last;
			}
		}
	}
}

while (<>) {
	my $line = $_;

	if (/#include ["<](.*)[">]/) {
		scan $1;
	}

	foreach my $regaddr (keys %regnamemap) {
		$line =~ s/$regaddr/$regnamemap{$regaddr}/ei;
	}
	print $line;
};

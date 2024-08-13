#!/usr/bin/env perl
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2021 Ahmad Fatoum

use strict;
use warnings;

use Cwd;
use File::Basename;
use File::Spec;
use File::Temp 'tempdir';
use Getopt::Long;
use List::Util 'first';
use Pod::Usage;
use YAML::XS 'LoadFile';

my @QEMU_INTERACTIVE_OPTS = qw(-serial mon:stdio -trace file=/dev/null);

my %targets;

my $LG_BUILDDIR;

if (exists $ENV{KBUILD_OUTPUT}) {
    $LG_BUILDDIR = $ENV{KBUILD_OUTPUT};
} elsif (-d 'build') {
    $LG_BUILDDIR = 'build';
} else {
    $LG_BUILDDIR = getcwd();
}


for my $arch (glob dirname(__FILE__) . "/*/") {
    for my $cfg (glob "$arch/*.yaml") {
	my $linkdest = readlink $cfg // '';

	my $yaml = LoadFile($cfg);

	defined $yaml && exists $yaml->{targets} && exists $yaml->{targets}{main}
	    or die "Invalid structure for $cfg\n";

	my $path = File::Spec->abs2rel($cfg, getcwd);
	$cfg = basename($cfg);
	$cfg =~ s/\.yaml$//;
	$linkdest =~ s{^.*?([^/]+)\.yaml$}{$1};

	$targets{basename $arch}{$cfg} = $yaml->{targets}{main};
	$targets{basename $arch}{$cfg}{path} = $path;
	$targets{basename $arch}{$cfg}{tools} = $yaml->{tools};
	$targets{basename $arch}{$cfg}{images} = $yaml->{images};
	$targets{basename $arch}{$cfg}{alias} = $linkdest if $linkdest && $linkdest ne $cfg;
    }
}

my %arch_aliases = (arm64 => 'arm', x86_64 => 'x86', i386 => 'x86', um => 'sandbox');

my ($dryrun, $help, @blks, $rng, $list, $shell, $runtime, @kconfig_add, $artifacts);
my ($tuxmake, $emulate, $clean, $extraconsoles, $test, $verbosity) = (1, 1, 1, 0, 0, 1);
my ($kconfig_base, $kconfig_full) = (1, 0);

my @OPTS;

if (defined (my $idx = first { $ARGV[$_] eq '--' } 0 .. @ARGV - 1)) {
    @OPTS = splice @ARGV, 1 + $idx;
}

my @emulate_only = qw/blk console rng/;

GetOptions(
	'help|?|h' => \$help,
	'dryrun|n' => \$dryrun,
	'list|l' => \$list,
	'tuxmake!' => \$tuxmake,
	'verbosity=i' => \$verbosity,
	'artifacts=s' => \$artifacts,
	'runtime=s' => \$runtime,
	'blk=s@' => \@blks,
	'console+' => \$extraconsoles,
	'rng' => \$rng,
	'emulate!' => \$emulate,
	'clean!' => \$clean,
	'shell' => \$shell,
	'kconfig-base!' => \$kconfig_base,
	'kconfig-full!' => \$kconfig_full,
	'kconfig-add|K=s@' => \@kconfig_add,
	'test' => \$test,
) or pod2usage(2);
pod2usage(1) if $help;

if ($test && (@blks || $extraconsoles || $rng)) {
    die "Virtual devices on command-line not supported with --test\n";
}

my @ARCH = split /\s/, $ENV{ARCH} // '';
@ARCH = @ARCH ? @ARCH : keys %targets;

my $success = 0;

for my $arch (@ARCH) {
	my @targets = @ARGV ? @ARGV : keys %{$targets{$arch}};
	@targets != 1 && !$tuxmake
		and die "can't use --no-tuxmake with more than one config\n";

	unless (defined $targets{$arch}) {
		die "Unknown ARCH=$arch. Supported values:\n",
		join(', ', keys %targets), "\n";
	}

	for my $config (@targets) {
	        $arch = $arch_aliases{$arch} // $arch;

		$config = fileparse($config, ".yaml");

		unless (defined $targets{$arch}{$config}) {
		    next;
		}

		if ($list) {
		    print "ARCH=$arch $config\n";
		    $success += 1;
		    next;
		}

		if (defined $targets{$arch}{$config}{alias}) {
		    next if grep { /^$targets{$arch}{$config}{alias}$/ } @targets;
		    $config = $targets{$arch}{$config}{alias};
		    redo;
		}

		print "# $config\n" if $dryrun;
		$success += process($arch, $config);
	}
}

$success > 0
	or die "No suitable config found. $0 -l to list all built-in.\n";

sub process {
	my ($ARCH, $defconfig, %keys) = @_;
	my $target = $targets{$ARCH}{$defconfig};

	if (!exists ($target->{runner}{tuxmake_arch})) {
	    $target->{runner}{tuxmake_arch} = $ARCH;
	}

	my $dir;

	$dir = tempdir("bareboxbuild-XXXXXXXXXX", TMPDIR => 1, CLEANUP => $clean);
	report('mkdir', $dir);

	my %opts = (
	    target => $target, builddir => $tuxmake ? $dir : getcwd,
	    defconfig => $defconfig,
	    extra_opts => [map { s/\{config\}/$defconfig/gr  } @OPTS],
	);

	build(%opts) if $tuxmake;

	while (my ($k, $v) = each %{$target->{runner}{download}}) {
	    if ($v =~ m{^[/.]}) {
		symlink_force($v, "$dir/$k");
	    } else {
		vsystem('curl', '-L', '--create-dirs', '-o', "$dir/$k", $v) == 0
		    or die "Failed to download resource `$v': $?\n";
	    }

	    symlink_force("$dir/$k", "$LG_BUILDDIR/$k") unless $tuxmake;
	}

	if ($shell) {
		pushd($dir);
		system($ENV{SHELL} // "/bin/sh");
		popd();
	}

	return 1 unless $emulate;

	if ($tuxmake) {
	    $ENV{KBUILD_OUTPUT} = $dir;
	    print "export KBUILD_OUTPUT=$ENV{KBUILD_OUTPUT}\n" if $dryrun;
	}

	my $success = $test ? test(%opts) : emulate(%opts);

        report("rm", "-rd", $dir) if $clean && $tuxmake;

	print "\n\n" if $dryrun;
	return $success;
}

sub build {
    my %args = @_;
    my ($runner, $dir) = ($args{target}{runner}, $args{builddir});

    $args{defconfig} =~ s/[^@]+@//g;

    my @TUXMAKE_ARGS = ('-a', $runner->{tuxmake_arch}, '-k', $args{defconfig});

    if (defined $runner->{kconfig_add}) {
	for my $cfg (@{$runner->{kconfig_add}}) {
	    push @TUXMAKE_ARGS, "--kconfig-add=$cfg"
	}
    }

    push @TUXMAKE_ARGS, "--kconfig-add=common/boards/configs/base.config" if $kconfig_base || $kconfig_full;
    push @TUXMAKE_ARGS, "--kconfig-add=common/boards/configs/full.config" if $kconfig_full;

    for (@kconfig_add) {
	push @TUXMAKE_ARGS, "--kconfig-add=$_";
    }

    push @TUXMAKE_ARGS, "--runtime=$runtime" if $runtime;

    push @TUXMAKE_ARGS, '-q' if $verbosity == 0;
    push @TUXMAKE_ARGS, '-v' if $verbosity == 2;

    vsystem('tuxmake', @TUXMAKE_ARGS, '-b', $dir, '-o',
	$artifacts // "$dir/artifacts", 'default') == 0
	or die "Error building: $?\n";
}

sub emulate {
    my %args = @_;
    my %target = %{$args{target}};
    my @OPTS = @{$args{extra_opts}};

    if (defined $target{drivers}{QEMUDriver}) {
	    my %qemu = %{$target{drivers}{QEMUDriver}};
	    my ($kernel, $bios, $dtb);
	    my $qemu_virtio;
	    my $i;

	    $kernel = abs_configpath($qemu{kernel}, \%args);
	    $bios   = abs_configpath($qemu{bios},   \%args);
	    $dtb    = abs_configpath($qemu{dtb},    \%args);

	    my @cmdline = ($target{tools}{$qemu{qemu_bin}},
		'-M', $qemu{machine}, '-cpu', $qemu{cpu}, '-m', $qemu{memory});

	    push @cmdline, '-kernel', $kernel if defined $kernel;
	    push @cmdline, '-bios', $bios if defined $bios;
	    push @cmdline, '-dtb',  $dtb if defined $dtb;

	    push @cmdline, @QEMU_INTERACTIVE_OPTS;
	    for (split /\s/, $qemu{extra_args}) {
		push @cmdline, $_;
	    }

	    if (has_feature(\%target, 'virtio-pci')) {
		    $qemu_virtio = 'pci,disable-legacy=on,disable-modern=off';
		    push @{$target{features}}, 'pci';
		    push @{$target{features}}, 'virtio';
	    } elsif (has_feature(\%target, 'virtio-mmio')) {
		    $qemu_virtio = 'device';
		    push @{$target{features}}, 'virtio';
	    }

	    $i = 0;
	    for my $blk (@blks) {
		if (has_feature(\%target, 'virtio')) {
		    $blk = rel2abs($blk);
		    push @cmdline,
		    '-drive', "if=none,file=$blk,format=raw,id=hd$i",
		    '-device', "virtio-blk-$qemu_virtio,drive=hd$i";
		} else {
		    die "--blk unsupported for target\n";
		}

		$i++;
	    }

	    # note that barebox doesn't yet support multiple virtio consoles
	    if ($extraconsoles) {
		$i = 0;

		if (defined $qemu_virtio) {
		    push @cmdline,
		    '-device', "virtio-serial-$qemu_virtio",
		    '-chardev', "pty,id=virtcon$i",
		    '-device', "virtconsole,chardev=virtcon$i,name=console.virtcon$i";

		    $i++;
		}

		if ($i < $extraconsoles) {
		    # ns16550 serial driver only works with x86 so far
		    if (has_feature(\%target, 'pci')) {
			for (; $i < $extraconsoles; $i++) {
			    push @cmdline,
			    '-chardev', "pty,id=pcicon$i",
			    '-device', "pci-serial,chardev=pcicon$i";
			}
		    } else {
			warn "barebox currently supports only a single extra virtio console\n";
		    }
		}
	    }

	    if (defined $rng) {
		if (has_feature(\%target, 'virtio')) {
		    push @cmdline, '-device', "virtio-rng-$qemu_virtio";
		} else {
		    die "--rng unsupported for target\n";
		}
	    }

	    pushd($args{builddir}) if $tuxmake;

	    vsystem(@cmdline, @OPTS) == 0 or die "Error running emulator: $?\n";

    } elsif (defined $target{drivers}{TinyEMUDriver}) {
	    my %temu = %{$target{drivers}{TinyEMUDriver}};
	    my $TEMU_CFG;
	    my $i = 0;

	    if (exists $temu{config}) {
		$temu{config} = rel2abs($temu{config});
		open(my $fh, '<', "$temu{config}")
		    or die "Could not open file '$temu{config}': $!";
		$TEMU_CFG = do { local $/; <$fh> };
	    }

	    print ("$temu{config}\n");

	    defined $TEMU_CFG or die "Unknown tinyemu-config\n";

	    open(my $fh, '>', "$args{builddir}/tinyemu.cfg")
		or die "Could not create file 'tinyemu.cfg': $!";
	    print $fh $TEMU_CFG;
	    print "cat >'tinyemu.cfg' <<EOF\n$TEMU_CFG\nEOF\n" if $dryrun;

	    for my $blk (@blks) {
		$blk = rel2abs($blk);
		$TEMU_CFG =~ s[\}(?!.*\})][drive$i: { file: "$blk" },\n}]ms
	    }

	    die "--console unsupported for target\n" if $extraconsoles;
	    die "--rng unsupported for target\n" if defined $rng;

	    pushd($args{builddir}) if $tuxmake;

	    vsystem($temu{temu_bin}, "tinyemu.cfg", @OPTS) == 0
		or die "Error running emulator: $?\n";
    } elsif (defined $target{drivers}{ExternalConsoleDriver}) {
	    my %exec = %{$target{drivers}{ExternalConsoleDriver}};

	    pushd($args{builddir}) if $tuxmake;

	    vsystem($exec{cmd}, @OPTS) == 0 or die "Error running emulator: $?\n";
    }

    popd() if $tuxmake;

    return 1;
}

sub test {
    my %args = @_;
    my @OPTS = @{$args{extra_opts}};
    my $pytest;
    my $dir;

    unless (defined $args{target}{drivers}{QEMUDriver}) {
	warn "$args{target}{path}: Skipping test, no QEMUDriver\n";
	return 0;
    }

    $pytest = `sh -c 'command -v labgrid-pytest'` ? 'labgrid-pytest' : 'pytest';

    unshift @OPTS, "--verbosity=$verbosity";

    vsystem($pytest, '--lg-env', "$args{target}{path}", "test/py", "--verbosity=1",
	'--lg-log', @OPTS) == 0 or die "Error running 'labgrid-pytest': $?\n";

    return 1;
}

sub has_feature {
    defined first { lc($_) eq $_[1] } @{$_[0]->{features}}
}

sub report {
    print join(' ', map { /\s/ ? "'$_'" : $_  } @_), "\n" if $dryrun;
    0;
}

sub vsystem {
    if ($dryrun) {
	return report(@_);
    } else {
	my $ret = system @_;
	warn "vsystem: $!\n" if $ret == -1;
	return $ret >> 8;
    }
}

sub rel2abs {
    File::Spec->rel2abs(@_)
}

sub abs_configpath {
    my ($path, $args) = @_;

    return unless defined $path;
    $path = $args->{target}{images}{$path};
    return unless defined $path;

    $path =~ s/\$LG_BUILDDIR\b/$LG_BUILDDIR/g;

    return rel2abs($path, $args->{builddir})
}

sub symlink_force {
    unlink($_[1]);
    symlink($_[0], $_[1]);
}

my @oldcwd;

sub pushd {
	my ($old, $new) = (getcwd, shift);
	report ("pushd", $new);
	push @oldcwd, $old;
	chdir $new;
	return $old;
};

sub popd {
	report("popd");
	chdir pop(@oldcwd)
};

__END__

=head1 NAME

emulate.pl - Build and run barebox under an emulator

=head1 SYNOPSIS

[ARCH=arch] emulate.pl [options] [defconfigs...] [--] [qemu/pytest-opts]

=head1 OPTIONS

Must be run from barebox source directory. If building out-of-tree,
either set C<KBUILD_OUTPUT> or ensure the out-of-tree directory
can be reached from a C<build> symlink in the current working
directory.

=over 8

=item B<-?>, B<-h>, B<--help>

Print this help message and exit

=item B<-n>, B<--dryrun>

Print commands and exit

=item B<-l>, B<--list>

Filter input with list of known targets

=item B<--verbosity>=%u

Specify output verbosity level for both tuxmake and pytest. Default value is 1.

=item B<--no-tuxmake>

Don't rerun tuxkmake. Assumes current working directory is finished build directory

=item B<--artifacts>=%s

Destination directory for the tuxmake artifacts. By default, this is
the artifacts/ subdirectory in the temporary build directory and is
not persisted (unless --no-clean is specified).

=item B<--blk>=%s

Pass block device to emulated barebox. Can be specified more than once

=item B<--console>

Pass one Virt I/O console to emulated barebox.

=item B<--rng>

instantiate Virt I/O random number generator

=item B<--no-emulate>

Don't emulate anything and exit directly after build

=item B<--no-clean>

Don't delete temporary working directory after

=item B<--shell>

Open shell in temporary working directory, after build, but before emulation

=item B<--test>

Instead of starting emulator interactively, run it under labgrid-pytest with
the in-tree python tests.

=item B<--runtime>=%s

Runtime to use for the tuxmake builds. By default, builds are
run natively on the build host.
Supported: null, podman-local, podman, docker, docker-local.

=item B<--no-kconfig-base>

Don't apply common/boards/configs/base.config. This may lead to more tests being
skipped.

=item B<--kconfig-full>

Applies common/boards/configs/full.config on top of base.config. This enables as much as
possible to avoid skipping tests for disabled functionality.

=item B<--kconfig-add>=%s, B<-K>=%s

Extra kconfig fragments, merged on top of the defconfig and Kconfig
fragments described by the YAML. In tree configuration fragment
(e.g. `common/boards/configs/virtio-pci.config`), path to local file, URL,
`CONFIG_*=[y|m|n]`, or `# CONFIG_* is not set` are supported.
Can be specified multiple times, and will be merged in the order given.

=back

=cut

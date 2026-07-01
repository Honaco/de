#!/usr/bin/perl

use File::Copy;

#	icl structure:
#	<icl>
#		<disk diskno="1">
#			<part partno="2">
#				<file>
#					<path>sbin/sucu_test</path>
#					<hash>117B772880D38629F5A7B0EB5E9970629243544C53D3C4574FEF3C055745117B</hash>
#				</file>
#			</part>
#		</disk>
#	</icl>
#
#	C:	D:	E:
#	1:	2:	3:
#
#	CRLF for Windows - "0x0D 0x0A"
#
#	amdz_fun_fs.cpp:
#	save_fs_data	->	save_fs_info_to_device	->	

#if (@ARGV < 2) {
if (@ARGV < 1) {
#	print "usage:\n\t$0 icl_old.txt icl_new.txt [DEFAULT_DISK_№] [DEFAULT_PART_№]\n";
	print "usage:\n\t$0 icl.txt [DEFAULT_DISK_№] [DEFAULT_PART_№]\n";
	print "\n";
	print "\ticl.txt\t\tfile with original AMDZ-5.5+ icl-lists that should be converted\n";
#	print "\ticl_new.txt\t-\tfilepath where converted icl-lists would be written (AMDZ-LE/GX/GXM/GXMH format)\n";
	print "\tDEFAULT_DISK_№\tdefault HDD-disk № used to create icl-lists on AMDZ-5.5 [default = 1]\n";
	print "\tDEFAULT_PART_№\tdefault partition starting № [in default, for C:\\ = 1, D:\\ = 2, etc]\n";
	exit 1;
}
open(icl_orig, $ARGV[0]) or die "Error - invalid original icl filepath!\nCheck whether \'".$ARGV[0]."\' file exists or permissions on it...\n";
open(icl_new, '>'.$ARGV[0].'.new') or die "Error - invalid original icl filepath!\nCheck whether \'".$ARGV[0]."\' file exists or permissions on it...\n";
open(icl_conv, '>'.$ARGV[0].'.tmp') or die "Error - invalid tmp filepath!\nCheck your permissions on \'".$ARGV[0].".tmp\' file...\n";
#open(icl_orig, $ARGV[0]) or die "Error - invalid original icl filepath!\nCheck whether \'".$ARGV[0]."\' file exists or permissions on it...\n";
#open(icl_conv, '>'.$ARGV[1]) or die "Error - invalid converted icl filepath!\nCheck your permissions on \'".$ARGV[1]."\' file...\n";

# array with Windows disk-labels
my @array1 = ('C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z');
#my @array2 = ('c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z');

# set the $default_hash variable
$default_hash = "117B772880D38629F5A7B0EB5E9970629243544C53D3C4574FEF3C055745117B";

# set the $default_diskno variable
if ($ARGV[1] eq '') {
	$default_diskno = 1;
}
else {
	$default_diskno = $ARGV[1];
}
# set the $default_partno variable
if ($ARGV[2] eq '') {
	$default_partno = 1;
}
else {
	$default_partno = $ARGV[2];
}

$count = 0;
while (<icl_orig>) {
	# cut only first string
	if ($count == 0) {
		chomp;
		($path, $hash) = split("\t");
#		print "$_\n";

		# set $partno
		$char = substr($path, 0, 1);
		my ($partno) = grep { $array1[$_] =~ /$char/ } 0..$#array1;
#		if ($partno > 23) {
#			$partno = grep { $array2[$_] =~ /$char/ } 0..$#array2;
#		}
		$partno += $default_partno;

		# cut first two chars ("С:", "D:", etc)
		$path = substr($path, 2);

		# convert '\' to '/'
		$path =~ s/\\/\//g;

		print icl_conv "$default_diskno$partno$path\n"; # $hash
	}
	else {
		print icl_new "$_";
	}
	$count++;
}
close(icl_orig);
close(icl_new);
move($ARGV[0].'.new', $ARGV[0]) or die "Failed to move $ARGV[0].new -> $ARGV[0]";
close(icl_conv);
exit 0;

#!/usr/bin/perl

#maketar.pl generates stm32dev.tar

$dfudir = "/home/arp/stm/dfu/stmdfu/";
@dfufiles = ("35-stm32-usbdfu.rules",
		"stm32flash",
		"bintodfu.c",
		"crc32.c",
		"crc32.h",
		"dfucommands.c",
		"dfucommands.h",
		"dfurequests.c",
		"dfurequests.h",
		"dfuse.c",
		"dfuse.h",
		"Makefile",
		"stmdfu.c",
		"stmdfu.h",
		"stm32f10x_stdperiph_lib.zip");

$tararg = "";

foreach $dfufile (@dfufiles)
{
	$tararg = $tararg.$dfufile." ";
}

$tarcmd = "cd $dfudir && tar -cjf stm32dev.tar.bz2 $tararg && cd -";

print $tarcmd;
print "\n";
`$tarcmd`;

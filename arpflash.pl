#!/usr/bin/perl

$pathtosrc = "/home/arp/stm/arp";
$pathtobintodfu = "/home/arp/stm/dfu/stmdfu";
$pathtostmdfu = "/home/arp/stm/dfu/stmdfu";

$domake = "cd $pathtosrc && make biquadblocktest";
$dobintodfu = "$pathtobintodfu/bintodfu $pathtosrc/arp.bin $pathtobintodfu/arp.dfuse";
$doerase = "$pathtostmdfu/stmdfu masserase";
$doflash = "$pathtostmdfu/stmdfu flash $pathtobintodfu/arp.dfuse";

print "$domake\n";
print `$domake`;
print "$dobintodfu\n";
print `$dobintodfu`;
print "$doerase\n";
print `$doerase`;
print "$doflash\n";
print `$doflash`;
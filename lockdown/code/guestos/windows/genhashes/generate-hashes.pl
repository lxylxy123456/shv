#!/usr/bin/perl
# generate-hashes.pl
# author(s): amit vasudevan (amitvasudevan@acm.org) 
# this script will generate SHA-1 hashes for 4K pages for PE files
# for a given windows installation

use lib '..';
use File::Find;

$tempdir = "/tmp";

# the hash databases that will be generated by the script
$hlistfull_filename = "./hashlist_full.dat";
$hlistpart_filename = "./hashlist_partial.dat";

# output default header within hash databases
system("echo '/* hashlist_full.dat is autogenerated */' > $hlistfull_filename");
system("echo '/* hashlist_partial.dat is autogenerated */' > $hlistpart_filename");

find(\&wanted, $ARGV[0]);


sub wanted { 
	# $File::Find::name should have the absolute filename path
	$shellcompatible_name = $File::Find::name;
	$shellcompatible_name =~ s/ /\\ /g;
	printf "%s -> %s\n", $File::Find::name, $shellcompatible_name;
}


#subroutine: given complete path to a file, if it is a PE, will dump both
#full and partial code page hashes
#return: 1 if the image is a PE and everything went well else 0 if image is not PE
#pre-defined vars used: tempdir, hlistfull_filename, hlistpart_filename
#assumption: hlistfull and hlistpart filenames have already been created
sub dumphashesforfile
{
	local(@lines);
	local($line, $totallines, $i, $ispe);
	local(@info);
	local($section_name, $section_vma, $section_size, $section_fileoffset);
		
	#$_[0] = arg1 = full path name to file
	$ispe = system("objdump -f $_[0] | grep 'pei-i386'");
	if ( $ispe != 0){
		return 0;
	}
	
	system("objdump -h $_[0] | awk -f pe_codesections_dump.awk > $tempdir/tmp.codesections");

	open(MYINPUTFILE, "< $tempdir/tmp.codesections"); 
	@lines = <MYINPUTFILE>; 
	close(MYINPUTFILE);
	$totallines = @lines;
	
	system("rm -rf $tempdir/tmp.codesections");
	
	for($i=0; $i < ($totallines/2); $i++){
		$line = $lines[$i];
		chomp($line);

		@info=split(/:/, $line);
		#/* section id, section name, section VMA, section size, section file offset*/
		$section_name= $info[1];
		$section_vma = $info[2];
		$section_size = $info[3];
		$section_fileoffset = $info[4];
		print "\t", "Full Hashes..." ,"\n";
 		system("./sha1 $_[0] $section_name $section_vma $section_size $section_fileoffset 0 >> $hlistfull_filename");
		print "\t", "Partial Hashes..." ,"\n";
		system("./sha1 $_[0] $section_name $section_vma $section_size $section_fileoffset 1 >> $hlistpart_filename");
	}

	return 1;
}


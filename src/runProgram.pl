use Getopt::Long;

#usage: perl runProgram.pl [-u] numMounts

my $unmount;

GetOptions(	"u=i" 	=> \$unmount)
or die "Error in command line arguments\n";



if(defined $unmount){
	print "MountNum:$unmount\n";
	&unmountAndDestroy($unmount);
}

else{
	my $mountNum = $ARGV[0];
	die "usage: perl runProgram.pl [-u] numMounts" unless defined($mountNum);
	&mountDrives($mountNum);
}

sub unmountAndDestroy{
	my $numMounts = $_[0];
	print "Unmount NumMounts:$numMounts\n";
	
	for(my $i = 0; $i < $numMounts; $i++){
		`fusermount -u ~/MyPFSbackup$i`;
		`rm -rf ~/MyPFSbackup$i`;
	}
	`fusermount -u ~/MyPFS`;
	`rm -rf ../backup/`;
	`rm -rf ~/MyPFS/`;
	`rm -f pfsmaster.log`;
}

sub mountDrives{
	my $numMounts = $_[0];
	print "Mounting $numMounts drives\n";
	
	unless(-e "pfs"){
		print "Making program\n";
		my $makeRes = `make`;
		die "Error in compiling" if($makeRes =~ /Error/);
	}
	
	#make the master directory
	mkdir("../backup/") or die "Cannot make ../backup/\n$!\n";
	mkdir("../backup/master/") or die "Cannot make ../backup/master/\n$!\n";
	mkdir("$ENV{HOME}/MyPFS/") or die "Cannot make ~/MyPFS/\n$!\n";
	
	for(my $i = 0; $i < $numMounts; $i++){
		mkdir("../backup/$i/") or die "Cannot make ../backup/$i\n$!\n";
		mkdir("$ENV{HOME}/MyPFSbackup$i/") or die "Cannot make ~/MyPFSbackup$i/\n$!\n";
	}
	
	#time to execute the programs
	my $res = `./pfs -m $numMounts pfsmaster.log ../backup/ ../backup/master/ ~/MyPFS/`;
	print $res."\n";
	
	for(my $i = 0; $i < $numMounts; $i++){
		print "Executing Mount $i\n";
		my $res = `./pfs pfs$i.log ../backup/ ../backup/$i/ ~/MyPFSbackup$i/`;
		print "$res\n";
	}
}



























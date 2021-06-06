#!/usr/bin/perl
use strict;
use warnings;

sub dms2d($) {
        my ($angle)=@_;

        my (@dms)=split(/:/,$angle);
        if (@dms>3 or $angle eq "") {
                return undef;
        }

        if (!($dms[0]=~ /^[+-]?\d+$/)) {
                 return undef;
        }
        if ($dms[1]<0 or $dms[1]>59 or $dms[1]=~/[\D]/) {
                return undef;
        }
        if ($dms[2]<0 or $dms[2]>59.99) {
                return undef;
        }

        if ($dms[0]=~ /^-/) {
                return ($dms[0]-$dms[1]/60-$dms[2]/3600);
        } else {
                return ($dms[0]+$dms[1]/60+$dms[2]/3600);
        }
}

sub hms2d($) {
        my ($hours)=@_;

        my (@hms)=split(/:/,$hours);
        if (@hms>3 or $hours eq "") {
                return undef;
        }

        if ($hms[0]<0 or $hms[0]>23 or $hms[0]=~/[\D]/) {
                return undef;
        }
        if ($hms[1]<0 or $hms[1]>59 or $hms[1]=~/[\D]/) {
                return undef;
        }
        if ($hms[2]<0 or $hms[2]>59.99) {
                return undef;
        }

        return (($hms[0]+$hms[1]/60+$hms[2]/3600)*15);
}

my %types2 = (
    '*' => 'Star',
    '**' => 'Double star',
    '*Ass' => 'Association of stars',
    'OCl' => 'Open Cluster',
    'GCl' => 'Globular Cluster',
    'Cl+N' => 'Star cluster + Nebula',
    'G' => 'Galaxy',
    'GPair' => 'Galaxy Pair',
    'GTrpl' => 'Galaxy Triplet',
    'GGroup' => 'Group of galaxies',
    'PN' => 'Planetary Nebula',
    'HII' => 'HII Ionized region',
    'DrkN' => 'Dark Nebula',
    'EmN' => 'Emission Nebula',
    'Neb' => 'Nebula',
    'RfN' => 'Reflection Nebula',
    'SNR' => 'Supernova remnant',
    'Nova' => 'Nova star',
    'NonEx' => 'Nonexistent object',
    'Dup' => 'Duplicated object',
    'Other' => 'Other classification'
);

my %types = (
    '*' => 'Star',
    '**' => 'Double star',
    '*Ass' => 'ASSOCIATION_OF_STARS',
    'OCl' => 'OPEN_CLUSTER',
    'GCl' => 'GLOBULAR_CLUSTER',
    'Cl+N' => 'STAR_CLUSTER_NEBULA',
    'G' => 'GALAXY',
    'GPair' => 'GALAXY_PAIR',
    'GTrpl' => 'GALAXY_TRIPLET',
    'GGroup' => 'GROUP_OF_GALAXIES',
    'PN' => 'PLANETARY_NEBULA',
    'HII' => 'HII_REGION',
    'DrkN' => 'DARK_NEBULA',
    'EmN' => 'EMISSION_NEBULA',
    'Neb' => 'NEBULA',
    'RfN' => 'REFLECTION_NEBULA',
    'SNR' => 'SUPERNOVA_REMNANT',
    'Nova' => 'NOVA_STAR',
    'NonEx' => 'NONEXISTANT',
    'Dup' => 'DUPLICATE',
    'Other' => 'OTHER_CLASSIFICATION'
);

my %extra_names = (
	"NGC1952" => "Crab nebula",
	"NGC2573" => "Polarissima Australis",
	"NGC3172" => "Polarissima Borealis",
	"NGC3918" => "Blue planetary nebula",
	"NGC4435" => "The Eyes",
	"NGC4438" => "The Eyes",
	"NGC4567" => "Siamese Twins",
	"NGC6523" => "Hourglass nebula",
	"NGC6960" => "Filamentary nebula",
	"NGC6992" => "Network nebula",
	"NGC6995" => "Network nebula"
);


my $file = $ARGV[0] or die "Need to get CSV file on the command line\n";

my $sum = 0;
open(my $data, '<', $file) or die "Could not open '$file' $!\n";

print "indigo_dso_entry indigo_dso_data[] = {\n";

print "\t{ \"Sh 2-276\", EMISSION_NEBULA, 5.45, -3.96, 5.0, 600.0, 600.0, 0.0, \"Barnard's Loop\" },\n";
print "\t{ \"Cl Melotte 22\", OPEN_CLUSTER, 3.79, 24.11, 1.6, 110, 110, 0, \"M45,  Seven Sisters, The Pleiades\" },\n";
print "\t{ \"C41\", OPEN_CLUSTER, 4.45, 15.87, 0.5, 330, 330, 0, \"Melotte 25, The Hyades\" },\n";

while (my $line = <$data>) {
	chomp $line;
     	my @f = split ";" , $line;

     	if ($f[0] eq "Name") {
     		next;
     	}

     	if (($f[1] eq "Dup") or ($f[1] eq "NonEx") or ($f[1] eq "Other") or ($f[1] eq "*") or ($f[1] eq "**")) {
     		next;
     	}

     	print "\t{ ";

	if (defined $f[0]) {
		$f[0] =~ s/NGC0+/NGC/;
		$f[0] =~ s/IC0+/IC/;
		print "\"$f[0]\"";
	}
	print ", ";

	if (defined $f[1]) {
		my $type = $types{$f[1]};
		print "$type";
	}
	print ", ";

	if (defined $f[2]) {
		my $ra = dms2d($f[2]);
		if(defined $ra) {
			printf "%.4f", $ra;
		}
	}
	print ", ";

	if (defined $f[3]) {
		my $dec = dms2d($f[3]);
		if(defined $dec) {
			printf "%.4f", $dec;
		}
	}
	print ", ";

	# magnitude
	my $mag_sum = 0;
	my $mag_count = 0;

	if (defined $f[8] && $f[8] ne "") {
		$mag_sum = $mag_sum + $f[8];
		$mag_count++;
	}

	if (defined $f[9] && $f[9] ne "") {
		$mag_sum = $mag_sum + $f[9];
		$mag_count++;
	}

	if (defined $f[10] && $f[10] ne "") {
		$mag_sum = $mag_sum + $f[10];
		$mag_count++;
	}

	if (defined $f[11] && $f[11] ne "") {
		$mag_sum = $mag_sum + $f[11];
		$mag_count++;
	}

	if (defined $f[12] && $f[12] ne "") {
		$mag_sum = $mag_sum + $f[12];
		$mag_count++;
	}


	if ($mag_count > 0) {
		printf "%.2f", $mag_sum/$mag_count;
	} else {
		print "0.0";
	}
	print ", ";

	# axes
	my $ax = $f[5];
	if (defined $f[5] && $f[5] ne "") {
		print "$f[5]";
	} else {
		print "0.0";
		$ax = "0.0";
	}
	print ", ";

	if (defined $f[6] && $f[6] ne "") {
		print "$f[6]";
	} else {
		print "$ax";
	}
	print ", ";

	# angle
	if (defined $f[7] && $f[7] ne "") {
		print "$f[7]";
	} else {
		print "0";
	}
	print ", ";


	# Names
	my $names = "";
	if (defined $f[18] && $f[18] ne "") {
		my $i = int($f[18]);
		$names = "M$i, ";
	}

	if (defined $f[19] && $f[19] ne "") {
		$f[19]=~ s/^0+//;
		$names = $names . "NGC$f[19], ";
	}

	if (defined $f[20] && $f[20] ne "") {
		$f[20]=~ s/^0+//;
		$names = $names . "IC$f[20], ";
	}

	if (defined $f[23] && $f[23] ne "") {
		$f[23]=~ s/,/, /g;
		$names = $names . "$f[23], ";
	}

	if (defined $extra_names{$f[0]}) {
		$names = $names . "$extra_names{$f[0]}, ";
	}

	$names =~ s/,\s+$//;
	$names =~ s/,\s+$//;
	print "\"$names\" },\n";
}
print "\t{ NULL }\n";
print "};\n";

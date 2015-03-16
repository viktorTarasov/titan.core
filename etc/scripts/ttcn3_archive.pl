###############################################################################
# Copyright (c) 2000-2014 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
###############################################################################
#!/usr/bin/perl
###############################################################################
# This script is intended to archive the project hierarchy from a TPD file structure
###############################################################################
use 5.010;
use strict;
use warnings;
use Cwd;
use File::Copy;

my $ttcn3_dir = $ENV{'TTCN3_DIR'};
my $bindir = $ttcn3_dir . "/bin"; 
my $home = cwd();
my @list = readFile();
my $tpd = getTPDFileName(\@list);
chomp ($tpd);
if (-l $tpd)  #get the path if it is a symlink
{
  $tpd = `readlink $tpd`;
  chomp ($tpd);
}
my $root = getPathToRootDir(\@list); # get the workspace directory of the OS
chomp ($root);
chdir ($root) or die "cannot change: $!\n";
my $cutstring = cwd; # this is string generated from the the absolut path to the workspace
my $archiveDir = getArchiveDir(\@list); #directory to place the archive
$archiveDir = $home . "/" .  $archiveDir;
chomp ($archiveDir);
my $createDir = "mkdir -p " .  $archiveDir;
my $res = system($createDir);
my $backupFileName = createBackupFileName();
my $backupfile = $archiveDir . "/" . $backupFileName;
if ($res != 0) { die (" creating directory " . $archiveDir ." failed\n"); }
my $archive = $bindir . "/" . "ttcn3_makefilegen" ." -V -P " . $cutstring .  " -t " .  $tpd . " | xargs tar cfz ". $archiveDir . "/" .  $backupFileName . " 2> /dev/null";
system($archive);  #running it
if (-e $backupfile) { print ("archiving succeeded\n"); }
else                { print ("archiving failed\n"); }
chdir ($home) or die "cannot change: $!\n";
############################################################
sub readFile
{
  my $makefile = "Makefile";
  open ( FILE,  "<", $makefile ) or die ( "failed to open file: $home\/$makefile\n" );
  my @lines = <FILE>;
  close FILE;
  return  @lines;   
}
############################################################
sub getPathToRootDir #get the relative path to OS workspace 
{
  my @list = @{$_[0]};
  my $search = qr/^ROOT_DIR =/s;
  my $offset = 0;
  my $line;
  for  my $i ( 0 .. $#list )
    {
        if ( $list[$i] =~ $search ) 
          {
             $line = $list[$i];
             my $dot = '.';
             $offset = index($list[$i], $dot);
             last;
          }
    }
  if ($offset  == 0) { die ( "no ROOT_DIR variable was found in the Makefile\n" ); }
  my $path = substr $line,  $offset;
  return $path;
}
############################################################
sub getTPDFileName # TPD filename what the Makefile is created from
{
  my @list = @{$_[0]};
  my $search = qr/^TPD =/s;
  my $offset = 0;
  my $line;
  for  my $i ( 0 .. $#list )
    {
        if ( $list[$i] =~ $search ) 
          {
             $line = $list[$i];
             my $assign = '=';
             $offset = index($list[$i], $assign);
             last;
          }
    }
  if ($offset  == 0) { die ( "no TPD variable was found in the Makefile\n" ); }
  my $file = substr $line,  $offset + 1;
  $file =~ s/^\s+|\s+$//;  # remove heading and traling whitespaces 
  return $file;
}
############################################################
sub getArchiveDir # the name of the archive directory
{
  my @list = @{$_[0]};
  my $search = qr/^ARCHIVE_DIR =/s;
  my $offset = 0;
  my $line;
  for  my $i ( 0 .. $#list )
    {
        if ( $list[$i] =~ $search ) 
          {
             $line = $list[$i];
             my $assign = '=';
             $offset = index($list[$i], $assign);
             last;
          }
    }
  if ($offset  == 0) { die ( "no ARCHIVE_DIR variable was found in the Makefile\n" ); }
  my $dir = substr $line,  $offset + 1;
  $dir =~ s/^\s+|\s+$//;  # remove heading and trailing whitespaces 
  return $dir;
}
############################################################
sub getExecutableName # the name of the target executable
{
  my @list = @{$_[0]};
  my $search = qr/^EXECUTABLE =/s;
  my $offset = 0;
  my $line;
  for  my $i ( 0 .. $#list )
    {
        if ( $list[$i] =~ $search ) 
          {
             $line = $list[$i];
             my $assign = '=';
             $offset = index($list[$i], $assign);
             last;
          }
    }
  if ($offset  == 0) { die ( "no EXCUTABLE variable was found in the Makefile\n" ); }
  my $exec = substr $line,  $offset + 1;
  $exec =~ s/^\s+|\s+$//;  # remove heading and trailing whitespaces 
  return $exec;
}
############################################################
sub createBackupFileName
{
  my $backupFile = getExecutableName(\@list);
  my $dot = '.';
  my $result = index($backupFile, $dot);
  if ($result > -1)
  {
     $backupFile = substr $backupFile,  0, $result;
  }
  chomp ($backupFile);
  my $date =  `date '+%y%m%d-%H%M'`;
  chomp ($date);
  my $baseName = $backupFile . "-" . $date . ".tgz";
  chomp ($baseName);
  return $baseName;
}
############################################################
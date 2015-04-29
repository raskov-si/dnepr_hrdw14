#!/usr/bin/perl
use IO::Socket::INET;

# flush after every write
$| = 1;

my ($socket,$received_data);
my ($peeraddress,$peerport);

$socket = new IO::Socket::INET ( 
    MultiHomed => '1',
    LocalAddr => $ARGV[0],
    LocalPort => defined ($ARGV[1])?$ARGV[1]:'30583',
    Proto => 'udp'
) or die "ERROR in Socket Creation : $! \n";
print "Waiting for data...";
while(1)
{
$socket->recv($recieved_data,1024);
$peer_address = $socket->peerhost();
$peer_port = $socket->peerport();
chomp($recieved_data);
print "\n($peer_address , $peer_port) said : $recieved_data";

#send the data to the client at which the read/write operations done recently.
$data = "echo: $recieved_data\n";
$socket->send("$data");

}

$socket->close();

#!/usr/bin/perl
use IO::Socket::INET;
use bytes

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
$socket->recv($recieved_data, 16);
$header_length = substr $recieved_data, 0, 4;
$header_cookie = substr $recieved_data, 4, 4;
$header_id     = substr $recieved_data, 8, 4;
$data          = substr $recieved_data, 12, 4;

$peer_address = $socket->peerhost();
$peer_port = $socket->peerport();
printf "\n ($peer_address , $peer_port) said : %04X %04X %04X %04X", $header_length, $header_cookie, $header_id, $data;
printf "\n ($peer_address , $peer_port) said : $header_length $header_cookie $header_id $data";


#send the data to the client at which the read/write operations done recently.
#$data = "echo: $recieved_data";
$length    = v0.0.0.20; 
$cookie    = $header_cookie;
$commandid = v0.0.0.1;
$result    = v0.0.0.0;
$role      = v0.0.0.1;
$data = $length.$cookie.$commandid.$result.$role;
$socket->send($data);

}

$socket->close();

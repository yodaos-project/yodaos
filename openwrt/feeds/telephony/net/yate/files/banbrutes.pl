#!/usr/bin/perl

# This yate module will monitor failed authentications and send the source
# IP addresses of users who fail to authenticate to the iptables extension
# "recent" for filtering.
#
# You have to have the iptables extension "recent" installed and you need to
# create and reference a "recent" list in your firewall configuration.
# For most people it's probably enough to add this custom firewall rule
# to /etc/firewall.user:
#
#  iptables -A input_rule -m recent --name yate_auth_failures --rcheck --seconds 3600 --hitcount 5 -j DROP
#
# This line will drop all incoming traffic from users who have failed to
# authenticate 5 consecutive times within the last hour.
#
# To enable this script in yate, add this script to the [scripts] section
# in /etc/yate/extmodule.conf.


use strict;
use warnings;
use lib '/usr/share/yate/scripts';
use Yate;

my $RECENT_LIST_NAME = '/proc/net/xt_recent/yate_auth_failures';

sub OnAuthenticationRequest($) {
  my $yate = shift;
  my $remote_ip = $yate->param('ip_host');

  if ($yate->header('processed') eq 'true') {
    # Successful authentication, forget previous failures
    `echo -$remote_ip > $RECENT_LIST_NAME`;
    return;
  }

  `echo +$remote_ip > $RECENT_LIST_NAME`;
}


my $yate = new Yate();

if (! -f $RECENT_LIST_NAME) {
  $yate->output("iptables recent list $RECENT_LIST_NAME does not exist");
  exit 1;
}

$yate->install_watcher('user.auth', \&OnAuthenticationRequest);
$yate->listen();

# Please, read doc/hooks-Perl.txt to learn more about Grn's hooks.

GRN::add_hook("TemplateNew", "template_new");
GRN::add_hook("TemplateReply", "template_reply");
GRN::add_hook("TemplateForward", "template_forward");
GRN::add_hook("TemplateMNew", "template_mail_new");
GRN::add_hook("TemplateMReply", "template_mail_reply");
GRN::add_hook("TemplateMForward", "template_mail_forward");


my $greeting = "            Hi, %a!\n\n";
my $greeting1 = "            Hello!\n\n";
my $sig = "\n\n-- \n sc\t\tmailto:sc\@ivvs.ul.ru\n";
my $fortune = '/usr/games/fortune /usr/share/games/fortune/sc';

sub get_footer
{
  my $ret = $sig;
  my $f = `$fortune`;
  $ret .= "\n$f" if ($f);
  return $ret;
}


sub template_new
{
  my $t = $greeting;
  $t =~ s/%a/All/g;
  $MSG{pos} = length($t);
  $t .= get_footer();
  $MSG{body} = $t;
  return 1;
}

sub template_reply
{
  $MSG{'X-Comment-To'} = $MSG1{From};
  $MSG{'X-Comment-To'} = $MSG1{'X-FTN-Sender'}  if ($MSG1{'X-FTN-Sender'});

  my $to = $MSG{'X-Comment-To'};
  $to =~ s/^"?(\w+)"?\s+.*/$1/;

  my $t = $greeting;
  $t =~ s/%a/$to/g;

  if ($MSG1{'X-Comment-To'})
  {
    $t .= "On $MSG1{Date} you wrote to $MSG1{'X-Comment-To'}:\n\n";
  }
  else {
    $t .= "On $MSG1{Date} you wrote:\n\n";
  }

  $MSG{pos} = length($t);
  $t .= &quote($MSG1{body}, &get_initials($MSG{'X-Comment-To'}))."\n";
  $t .= get_footer();
  $MSG{body} = $t;
  
  if ($MSG1{Subject} eq $MSG{Subject})	# Modify only unchanged subject
  {
    my $subj = $MSG1{Subject};
    $subj =~ s/^(.*[Rr][Ee]\^?\d*:\s?)*(.*)/$2/;
    $subj =~ s/^(.*[Ff][Ww][Dd]\^?\d*:\s?)*(.*)/$2/;
    $subj = "Re: ".$subj;
    $MSG{Subject} = $subj;
  }
  return 1;
}

sub template_forward
{
  my $t = $greeting;
  $t =~ s/%a/All/g;
  $MSG{pos} = length($t);
  
  $t .= "\n\n".&pad(63, "=")."\n";
  $t .= "*Newsgroups:* $MSG1{Newsgroups}\n";
  $t .= "*From:* $MSG1{From}\n";
  if ($MSG1{'X-Comment-To'})
  { $t .= "*To:* $MSG1{'X-Comment-To'}\n"; }

  my $subj = $MSG1{Subject};
  $subj =~ s/^(.*[Rr][Ee]\^?\d*:\s?)*(.*)/$2/;
  $subj =~ s/^(.*[Ff][Ww][Dd]\^?\d*:\s?)*(.*)/$2/;
  $t .= "*Subject:* $subj\n";

  $t .= &pad(63, "-")."\n";
  $t .= $MSG1{body};
  $t .= "\n".&pad(63, "=");
  
  $t .= get_footer();
  $MSG{body} = $t;

  if ($MSG1{Subject} eq $MSG{Subject})	# Modify only unchanged subject
  {
    $subj = "Fwd: ".$subj;
    $MSG{Subject} = $subj;
  }
  return 1;
}


sub template_mail_new
{
  my $to = $MSG{To};
  my $t = $greeting1;

  if (not $to =~ /, /)
  {
    $t = $greeting;
    $t =~ s/%a/$to/g;
  }
  $MSG{pos} = length($t);
  $t .= get_footer();
  $MSG{body} = $t;
  return 1;
}

sub template_mail_reply
{
  my $to = $MSG1{From};
  $to =~ s/^"?(\w+)"?\s+.*/$1/;

  my $t = $greeting;
  $t =~ s/%a/$to/g;
  $t .= "On $MSG1{Date} you wrote in $MSG1{Newsgroups}:\n\n";
  $MSG{pos} = length($t);
  $t .= &quote($MSG1{body}, &get_initials($MSG1{From}))."\n";
  $t .= get_footer();
  $MSG{body} = $t;
  
  if ($MSG1{Subject} eq $MSG{Subject})	# Modify only unchanged subject
  {
    my $subj = $MSG1{Subject};
    $subj =~ s/^(.*[Rr][Ee]\^?\d*:\s?)*(.*)/$2/;
    $subj =~ s/^(.*[Ff][Ww][Dd]\^?\d*:\s?)*(.*)/$2/;
    $subj = "Re: ".$subj;
    $MSG{Subject} = $subj;
  }
  return 1;
}

sub template_mail_forward
{
  my $to = $MSG{To};

  my $t = $greeting;
  $t =~ s/%a/$to/g;
  $MSG{pos} = length($t);
  
  $t .= "\n\n".&pad(63, "=")."\n";
  $t .= "*Newsgroups:* $MSG1{Newsgroups}\n";
  $t .= "*From:* $MSG1{From}\n";
  if ($MSG1{'X-Comment-To'})
  { $t .= "*To:* $MSG1{'X-Comment-To'}\n"; }

  my $subj = $MSG1{Subject};
  $subj =~ s/^(.*[Rr][Ee]\^?\d*:\s?)*(.*)/$2/;
  $subj =~ s/^(.*[Ff][Ww][Dd]\^?\d*:\s?)*(.*)/$2/;
  $t .= "*Subject:* $subj\n";

  $t .= &pad(63, "-")."\n";
  $t .= $MSG1{body};
  $t .= "\n".&pad(63, "=");
  
  $t .= get_footer();
  $MSG{body} = $t;

  if ($MSG1{Subject} eq $MSG{Subject})	# Modify only unchanged subject
  {
    $subj = "Fwd: ".$subj;
    $MSG{Subject} = $subj;
  }
  return 1;
}


sub get_initials
{
  my ($s) = $_[0];
  $s =~ s/^"?(.+)"?\s+<.*>/$1/;
  my @words = split(/[\._ \t\@-]+/, $s);
  my ($ret, $w);
  foreach $w (@words)
  {
    $ret .= substr($w, 0, 1);
  }
  return $ret;
}

# This is rather simple quoting method. It accepts only '>' as a quoting char.
sub quote
{
  my ($body, $in) = ($_[0], $_[1]);
  my $is_sig = 0;
  my ($l, $ret);
  my @lines = split(/\n/, $body);
  foreach $l (@lines)
  {
    if ($l =~ /^(\s{0,2}[^<\s_\.\(\)-,]*)(>+.*)/)	# Already quoted line
    {
      $l = $1.'>'.$2;
    }
    elsif (($l =~ /^-- $/) or $is_sig)		# Ignore signature
    {
#      $is_sig = 1;  next;
    }
    elsif ($l =~ /^\s*$/) { next; }		# Empty strings are skipped
    else {					# Not quoted lines
      $l = " $in> ".$l;
    }
    $ret .= $l."\n";
  }
  return $ret;
}

sub pad
{
  my ($n, $char) = ($_[0], $_[1]);
  my ($ret, $i);
  for ($i = 0; $i <= $n; $i++)  { $ret .= $char; }
  return $ret;
}

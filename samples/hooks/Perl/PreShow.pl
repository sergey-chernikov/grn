# Please, read doc/hooks-Perl.txt to learn more about Grn's hooks.

# This function makes some additional highlighting for message body. It can
# handle *bold*, _underlined_, /italic/ text. Also it makes some additional
# text translations. It substitutes Grn's built-in parsing routine, so no
# pre-parsing or post-parsing performed.
# If you would like to change it, don't forget to use GRN::html_escape().
#
#
# The following considerations are presumed:
#
# Font palette index:
# 1: bold font
# 2: underlined font
# 3: italic font
#
# Color palette index:
# 1: header name
# 2: header value
# 3: underlined font
# 4: C comments


GRN::add_hook("PreShow", "simple_html");
GRN::add_hook("PreShowHeader", "show_header");
GRN::add_hook("PreShowFooter", "show_footer");
GRN::add_hook("PreShowPart", "show_part");


# %MSG is in global scope

sub simple_html
{
  my $re_q = GRN::get_config("re_quoting");
  my ($isig, $sig) = (0, "");
  my $b = "";

  $b .= &process_header();

  @lines = split(/\n/, $MSG{body});
  foreach $l (@lines)
  {
    $l .= "\n";
    if (($l eq "-- \n") or ($isig))
    {
      $sig .= $l;
      $isig += 1;
    }
    elsif ($l =~ /$re_q/)
    {
      $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor1')."\">".&process_line($l)."</FONT>";
    }
    else {
      $b .= &process_line($l);
    }
    if ($isig > 16)	# No more than 16 lines in the signature
    {
      $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor2')."\">".&process_line($sig)."\n</FONT>";
      $isig = 0;
    }
  }
  $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor2')."\">".&process_line($sig)."\n</FONT>"  if $isig;

  $b .= &process_footer();
  $b = &post_process($b);
  
  my $title = GRN::html_escape($MSG{Subject});
  $title .= " - [".$MSG{'Newsgroups'}."]"  if ($MSG{'Newsgroups'});

  $MSG{body} = "<HTML><HEAD><TITLE>$title</TITLE>\n".
		"</HEAD><BODY>$b</BODY></HTML>\n";

  $MSG{From} = $MSG{'X-FTN-Sender'}  if ($MSG{'X-FTN-Sender'});
  return 1;
}


sub post_process
{
  my $s = $_[0];
  $s =~ s/\n/<BR>/g;
  $s =~ s/(.*)(<[bB][rR]>)+$/$1/mg;
  return $s;
}

sub process_header
{
  my $s = "";
  my $ihdr = 0;

  if ($MSG{'X-FTN-REALNAME'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\">RealName: </FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'X-FTN-REALNAME'})."</FONT><BR>";
    $ihdr = 1;
  }
  $s .= "<BR>"  if ($ihdr);
  return $s;
}

sub show_header
{
  my $b = &process_header();
  $b = &post_process($b);

  my $title = GRN::html_escape($MSG{Subject});
  $title .= " - [".$MSG{'Newsgroups'}."]"  if ($MSG{'Newsgroups'});

  $MSG{body} = "<HTML><HEAD><TITLE>$title</TITLE>\n".
		"</HEAD><BODY>$b";
  $MSG{From} = $MSG{'X-FTN-Sender'}  if ($MSG{'X-FTN-Sender'});

  return 1;
}

sub process_footer
{
  my $s = "";

  if ($MSG{'X-FTN-Tearline'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\">---</FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'X-FTN-Tearline'})."</FONT><BR>";
  }
  elsif ($MSG{'User-Agent'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\">User-Agent: </FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'User-Agent'})."</FONT><BR>";
  }
  elsif ($MSG{'X-Newsreader'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\">X-Newsreader: </FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'X-Newsreader'})."</FONT><BR>";
  }
  if ($MSG{'X-FTN-Origin'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\"> * Origin: </FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'X-FTN-Origin'})."</FONT><BR>";
  }
  elsif ($MSG{'Organization'})
  {
    $s .= "<FONT COLOR=\"".GRN::get_color_pal(1)."\">Organization: </FONT>".
          "<FONT COLOR=\"".GRN::get_color_pal(2)."\">".
          &process_line($MSG{'Organization'})."</FONT><BR>";
  }
  return $s;
}

sub show_footer
{
  my $b = &process_footer();
  $b = &post_process($b);

  $MSG{body} = "\n$b</BODY></HTML>\n";
  return 1;
}

sub show_part
{
  return 0  if (($MPART{type} eq "multipart") or ($MPART{type} eq "message"));

  my $info = "_~_~_~_~_ MIME part: $MPART{type}/$MPART{subtype} ".
             "Size = $MPART{size}; Encoding: $MPART{encoding} _~_~_~_~_";
  my $b = "";

  if ($MPART{type} =~ /text/i)
  {
    if ($MPART{subtype} =~ /html/i)
    {
      $b = $MPART{data};
      $b =~ s/\<\!.+\>//mg;
    }
    else 
    {
      my $re_q = GRN::get_config("re_quoting");
      my ($isig, $sig) = (0, "");

      @lines = split(/\n/, $MPART{data});
      foreach $l (@lines)
      {
        $l .= "\n";
        if (($l eq "-- \n") or ($isig))
        {
          $sig .= $l;
          $isig += 1;
        }
        elsif ($l =~ /$re_q/)
        {
          $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor1')."\">".&process_line($l)."</FONT>";
        }
        else {
          $b .= &process_line($l);
        }
        if ($isig > 16)	# No more than 16 lines in the signature
        {
          $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor2')."\">".&process_line($sig)."\n</FONT>";
          $isig = 0;
        }
      }
      $b .= "<FONT COLOR=\"".GRN::get_config('fgcolor2')."\">".&process_line($sig)."\n</FONT>"  if $isig;
      $b = &post_process($b);
    }
  }
  $info = GRN::html_escape($info);
  $MPART{data} = "<BR><BR>$info<BR>$b";
  return 1;
}


sub process_line
{
  my $s = $_[0];
  $s =~ tr/≥£/Â≈/;	# For koi8-r locale only and fonts without \:e

  $s = &prc_line($s);
  if ($s =~ /^([ \t]+)(.*)/s)
  {
    my ($s1, $s2) = ($1, $2);
    $s1 =~ s/ /&nbsp;/g;
    $s1 =~ s/\t/&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;/g;
    $s = $s1.$s2;
  }
  if ($s =~ /[ \t]{3,}/s)
  {
    while ($s =~ /(.*)([ \t]{2,})(.*)/s)
    {
      my ($s1, $s2, $s3) = ($1, $2, $3);
      $s2 =~ s/ /&nbsp;/g;
      $s2 =~ s/\t/&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;/g;
      $s2 =~ s/^(&nbsp;)(.+)/ $2/g;
      $s = $s1.$s2.$s3;
    }
  }
  if ($s =~ /<\/.+>[ \t]<.+>/s)
  {
    $s =~ s/(<\/.+>)[ \t](<.+>)/$1&nbsp;$2/g;
  }
  return $s;
}


my $delim1_l = '[\s\[({<"\.,]+|^';
my $delim1_r = '[\s\.,;:?!\])}>"]+|$';
my $delim2_l = '[\s\[({<#"\.,]+|^';
my $delim2_r = '[\s\.,;:?!\])"}>#]+|$';
my $delim3_l = '[\s\[({]+|^';
my $delim3_r = '[\s\.,;:?!)}\]]+|$';
my $bold_in = '[^\*]+';
my $it_in   = '[^\/]+';

sub prc_line
{
  my $s = $_[0];
  my $pr = 0;

  if ($s =~ /(.*)($delim3_l)(\/\*.+\*\/)($delim3_r)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = GRN::html_escape($mid);  $end = prc_line($end);
    $s = $beg."<FONT COLOR=\"".GRN::get_color_pal(4)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim1_l)\*($bold_in)\*($delim1_r)(.*)/s)
  {
    goto no_prc  if ($3 =~ /^\s+.*/);
    goto no_prc  if ($3 =~ /.*\s+$/);
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT FACE=\"".GRN::get_font_pal(1)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim3_l)<[bB]>(.+)<\/[bB]>($delim3_r)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT FACE=\"".GRN::get_font_pal(1)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim3_l)<[bB][oO][lL][dD]>(.+)<\/[bB][oO][lL][dD]>($delim3_r)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT FACE=\"".GRN::get_font_pal(1)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim3_l)<[sS][tT][rR][oO][nN][gG]>(.+)<\/[sS][tT][rR][oO][nN][gG]>($delim3_r)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT FACE=\"".GRN::get_font_pal(1)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim2_l)_([^_]+)_([$delim2_r]+|$)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT COLOR=\"".GRN::get_color_pal(3)."\" FACE=\"".
         GRN::get_font_pal(2)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim2_l)_([^ ]+)_([$delim2_r]+|$)(.*)/s)
  {
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $mid =~ s/_/ /g;
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT COLOR=\"".GRN::get_color_pal(3)."\" FACE=\"".
         GRN::get_font_pal(2)."\">".$mid."</FONT>".$end;
  }
  elsif ($s =~ /(.*)($delim2_l)\/($it_in)\/($delim2_r)(.*)/s)
  {
    goto no_prc  if ($3 =~ /^\s+.*/);
    goto no_prc  if ($3 =~ /.*\s+$/);
    $pr = 1;
    my ($beg, $mid, $end) = ($1.$2, $3, $4.$5);
    $beg = prc_line($beg);  $mid = prc_line($mid);  $end = prc_line($end);
    $s = $beg."<FONT FACE=\"".GRN::get_font_pal(3)."\">".$mid."</FONT>".$end;
  }

no_prc:
#  $s =~ s/(<\/.+>)\s+(<.+>)/$1&nbsp;$2/gi;
  return $s  if ($pr);
  $s = GRN::html_escape($s);
  return $s;
}

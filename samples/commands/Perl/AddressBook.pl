# Reads Gnome-Card's file

# Requires Gtk-Perl
use Gtk;

GRN::add_command(4, "Address book", "set_address");
GRN::add_command(1, "Add to AddrBook", "add_to_ab");


my $gcrd = ".gnome/GnomeCard.gcrd";
my $cb_replace = 1;
my $clist = undef;
my $in_vcard = 0;
my @person = undef;

sub set_address
{
  return  if ($clist);
  chdir;
  unless (open(HGCRD, $gcrd))
  {
    GRN::error_dialog("Address book", "GnomeCard's data file not found")
      if GRN::get_config("cfm_show_warnings");
    return;
  }

  my @titles = ("Handle", "Name", "Address");

  Gtk->init;
  my $abw = Gtk::Widget->new("GtkWindow", -type=>'toplevel',
                             -title=>'Address book');
  $abw->signal_connect("destroy", \&abw_destroy, "");
  my $box = Gtk::HBox->new(0, 0);
  $box->border_width(5);
  $abw->add($box);
  my $box1 = Gtk::VBox->new(0, 0);
  $box->pack_start($box1, 1, 1, 0);
  my $box2 = Gtk::VBox->new(0, 0);
  $box->pack_start($box2, 0, 1, 4);

  my $scrwin = Gtk::ScrolledWindow->new(undef, undef);
  $scrwin->set_policy('automatic', 'automatic');
  $clist = Gtk::CList->new_with_titles(@titles);
  $clist->set_usize(450, 300);
  $clist->set_column_auto_resize(0, 1);
  $clist->set_column_auto_resize(1, 1);
  $clist->set_column_auto_resize(2, 1);
#  $clist->set_selection_mode('multiple');
  $clist->set_selection_mode('extended');
  $scrwin->add($clist);
  $box1->pack_start($scrwin, 1, 1, 0);
  
  my $cb = Gtk::CheckButton->new("Replace address[es]");
  $cb->set_active($cb_replace);
  $cb->signal_connect("clicked", \&cb_toggle, "");
  $box1->pack_start($cb, 0, 0, 2);

  my $btn = Gtk::Button->new("Ok");
  $btn->signal_connect("clicked", \&abw_ok, $abw);
  $box2->pack_start($btn, 0, 1, 2);
  $btn = Gtk::Button->new("Cancel");
  $btn->signal_connect("clicked", \&abw_cancel, $abw);
  $box2->pack_start($btn, 0, 1, 2);
  
  @person = undef;
  while (<HGCRD>)
  {
    chomp;
    if ($_ =~ /^BEGIN:VCARD/i)
    {
      $in_vcard = 1;
    }
    elsif ($_ =~ /^END:VCARD/i)
    {
      $in_vcard = 0;
      $clist->append(@person)  if ($person[2]);
      @person = undef;
    }
    elsif (($_ =~ /^FN:(\S+)/i) and $in_vcard)
    {
      $person[0] = $1;
    }
    elsif (($_ =~ /^N:(\S+)/i) and $in_vcard)
    {
      ($surname, $name, $secname) = split(/;/, $1, 3);
      $person[1] = $name;
      $person[1] .= " $secname."  if ($secname);
      $person[1] .= " $surname";
    }
    elsif (($_ =~ /^EMAIL;INTERNET:(\S+)/i) and $in_vcard)
    {
      if ($person[2])
      {
        $clist->append(@person);
	$person[2] = $1;
      }
      else { $person[2] = $1; }
    }
  }
  close(HGCRD);

  $abw->show_all;
}


sub abw_destroy
{
  $clist = undef;
}

sub cb_toggle
{
  my ($cb, $tmp) = @_;
  $cb_replace = $cb->active;
}

sub abw_cancel
{
  my ($btn, $wnd) = @_;
  $wnd->destroy;
}

sub abw_ok
{
  my ($btn, $wnd) = @_;

  my ($old_to, $res) = (GRN::entry_text_get("To", 0, -1), "");
  my $addr;  my $addrs;
  foreach ($clist->selection)
  {
    my $name = $clist->get_text($_, 1);
    if (length($name) > 0)
    {
      $addr = "$name <".$clist->get_text($_, 2).">";
    }
    else { $addr = $clist->get_text($_, 2); }

    if ($addrs)	{ $addrs .= ", ".$addr; }
    else	{ $addrs = $addr; }
  }
  $res = $addrs;
  $res = $old_to.", ".$addrs  if ($old_to && (not $cb_replace));
  GRN::entry_text_set("To", $res);
  $wnd->destroy;
}


sub add_to_ab
{
  my $from = GRN::entry_text_get("From", 0, -1);
  my ($n, $addr, $name) = (undef, undef, undef);

  if (not $from)
  {
    GRN::error_dialog("Address book", "Nothing to add")
      if GRN::get_config("cfm_show_warnings");
    return;
  }
  if ($from =~ /^(.+)\s+<(.+)>/)
  {
    ($name, $addr) = ($1, $2, "");
    my ($name1, $name2, $surname) = split(/\s+/, $name, 3);
    if (not $surname)
    {
      $n = $name2.";".$name1;
    }
    else {
      $name2 =~ s/(.+)\.$/$1/;
      $n = $surname.";".$name1.";".$name2;
    }
  }
  chdir;

  if (open(HGCRD, $gcrd))
  {
    my $a = $addr;
    $a = $from  if (! $a);

    while (<HGCRD>)
    {
      chomp;
      if ($_ =~ /^BEGIN:VCARD/i)
      {
        $in_vcard = 1;
      }
      elsif ($_ =~ /^END:VCARD/i)
      {
        $in_vcard = 0;
	if ($person[2] eq $a)
	{
	  GRN::error_dialog("Address book", "The address already exists")
            if GRN::get_config("cfm_show_warnings");
          close(HGCRD);
	  return;
	}
        @person = undef;
      }
      elsif (($_ =~ /^EMAIL;INTERNET:(\S+)/i) and $in_vcard)
      {
        if ($person[2])
        {
	  if ($person[2] eq $a)
	  {
	    GRN::error_dialog("Address book", "The address already exists")
              if GRN::get_config("cfm_show_warnings");
            close(HGCRD);
	    return;
	  }
	  $person[2] = $1;
        }
        else { $person[2] = $1; }
      }
    }
    close(HGCRD);
  }

  unless (open(HGCRD, ">>$gcrd"))
  {
    GRN::error_dialog("Address book", "GnomeCard's data file not writable")
      if GRN::get_config("cfm_show_warnings");
    return;
  }
  print HGCRD "BEGIN:VCARD\n";
  print HGCRD "FN:_grn_added\n";
  if ($n)
  {
    print HGCRD "N:$n\n";
    print HGCRD "EMAIL;INTERNET:$addr\n";
  }
  else {
    print HGCRD "EMAIL;INTERNET:$from\n";
  }
  print HGCRD "END:VCARD\n\n";
  close(HGCRD);
}

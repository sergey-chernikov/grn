# The first argument for add_command is a context:
#   1	Main window
#   2	Message editing window (posting and sending)
#   3	Message posting window
#   4	Message sending window
# The second argument is the name which will appear in the menu.
#
# The GRN::mew_*() functions are not documented yet (see src/perl.c).


GRN::add_command(2, "Skipper", "mcr_skipper");


sub mcr_skipper
{
  my ($start, $end) = GRN::mew_text_get_selection();
  if ($start && $end)
  {
    GRN::mew_text_delete($start, $end);
  }
  GRN::mew_text_insert("\n[skipped]\n");
  return 1;
}

# Please, read doc/hooks-Perl.txt to learn more about Grn's hooks.

# one subroutine may be called as different hooks
GRN::add_hook("PrePost", "pre_post");
GRN::add_hook("PreSend", "pre_post");

# You can use these hooks to make some translation, e.g. "î" (in cp866) to H
# (rarely useful, because FTN<->Usenet gates do it automatically).

# %MSG is in global scope
sub pre_post
{
# example for variable substitution function
  $MSG{'User-Agent'} = GRN::subst_vars('@longpid @ver [@un_s/@un_m @un_r]');

  $MSG{'Subject'} =~ tr/î/H/;
  return 1;
# returning of 0 will discard all the changes in %MSG
}

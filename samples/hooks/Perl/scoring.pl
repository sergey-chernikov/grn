# Please, read doc/hooks-Perl.txt to learn more about Grn's hooks.

# All color indexes from 0 to 15 correspond to standard VGA textmode palette:
# 1 - blue, 2 - green, 3 - cyan and so on.

GRN::add_hook("GetScore", "get_score");


# %MSG is in global scope
sub get_score
{
  # eliminate moderatorials in group 'fido.humor.filtered'
  if ($MSG{Group} eq "fido.humor.filtered")
  {
    if ($MSG{From} =~ /Moderator/i)
    { return 99; }
  }

  # mark moderatorials by ltred ball
  if ($MSG{From} =~ /Moderator/i)
  { return 12; }

  # mark FAQs or FAQ-robots' postings by red ball
  if ((($MSG{From} =~ /FAQ/) or ($MSG{Subject} =~ /FAQ/)) and
      ($MSG{References} eq '') and !($MSG{Subject} =~ /^Re.*:.+/i))
  { return 4; }

  # ignore spammer's postings in all groups
  if ($MSG{From} =~ /spammer/i)
  { return 99; }

  # mark messages from "Interesting Person" by blue ball
  if ($MSG{From} =~ /Interesting.Person/i)
  { return 1; }

  return 0;
}

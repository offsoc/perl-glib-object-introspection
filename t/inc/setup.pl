use Glib::Object::Introspection;
use Test::More;

unless (-e 'build/libregress.so' && -e 'build/libgimarshallingtests.so') {
  plan skip_all => 'Need the test libraries';
}

unless (defined $ENV{LD_LIBRARY_PATH} &&
        $ENV{LD_LIBRARY_PATH} =~ m/\bbuild\b/)
{
  plan skip_all => 'Need "build" in LD_LIBRARY_PATH';
}

Glib::Object::Introspection->setup(
  basename => 'Regress',
  version => '1.0',
  package => 'Regress',
  search_path => 'build');

Glib::Object::Introspection->setup(
  basename => 'GIMarshallingTests',
  version => '1.0',
  package => 'GI',
  search_path => 'build');

# Inspired by Test::Number::Delta
sub delta_ok ($$;$) {
  my ($a, $b, $msg) = @_;
  ok (abs ($a - $b) < 1e-6, $msg);
}

1;

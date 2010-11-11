#!/usr/bin/env perl

BEGIN { require './t/inc/setup.pl' };

use strict;
use warnings;

plan tests => 2;

is (test_enum_param ('value1'), 'value1');
is (test_unsigned_enum_param ('value2'), 'value2');

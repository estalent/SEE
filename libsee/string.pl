#!/usr/bin/env perl
# $Id$
#
# The author of this software is David Leonard.
#
# Copyright (c) 2003
#      David Leonard.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of Mr Leonard nor the names of the contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY DAVID LEONARD AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL DAVID LEONARD OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

#
# Usage:   string.pl [h|c] string.defs
#
# Reads a file of string definitions and outputs C code that compiles
# to static UTF-16 strings. Each string definitions must fit one of these
# forms:
#
#       <ident>
#       <ident> = "<text>"
#
# Blank lines and lines starting with a hash ('#') are ignored.
#
# Modes:
#   'h': generate a C header file which defines 
#            struct SEE_string *STR(<ident>);
#            extern struct SEE_string SEE_stringtab[];
#            extern int SEE_nstringtab;
#   'c': generates a C file that implements the header using
#        a static table of strings
#

$mode = shift @ARGV;	#-- mode

#-- Phase 1: scan the string.defs file, recoridng the ident and text
#
%seen = ();	# mapping from <ident> to boolean indicating nonuniquenes
@strings = ();	# mapping from index to {<ident>,<text>}
while (<>) {
	next if m/^\s*(#|$)/;	# ignore blank lines and comment lines

	my $ident, $text;
	if    (m/^\s*(\w+)\s*=\s*"(.*)"\s*$/) { ($ident,$text) = ($1,$2); }
	elsif (m/^\s*(\w+)\s*$/)              { $ident = $text = $1; }
	else { die "$ARGV:$.: malformed line\n"; }

	die "$ARGV:$.: duplicate ident\n" if $seen{$ident};
	$seen{$ident} = 1;

	push(@strings, { 'ident' => $ident, 'text' => $text});
#	print STDERR $strings[$#strings]->{'ident'} . "/" .
#	             $strings[$#strings]->{'text'} . "\n";
}

if    ($mode eq "h") { &h_mode; }
elsif ($mode eq "c") { &c_mode; }
else                 { die "unknown mode $mode\n"; }

#-- returns an array of unicode codepoints from a <text> string
sub text_to_unicode {
	my $text = shift @_;
	my $strlen = 0;
	my @codepoints = ();
	while ($text ne '') {
	    if ($text =~ s/^\\u(....)//) { 
	    	push(@codepoints, hex($1)); 
	    } elsif ($text =~ s/^\\([0-7]{1,3})//) { 
	    	push(@codepoints, oct($1));
	    } elsif ($text =~ s/^\\(.)//) {
	    	push(@codepoints, ord($1)); 
	    } elsif ($text =~ s/^(.)//) {
	    	push(@codepoints, ord($1)); 
	    } 
	}
	return @codepoints;
}

#-- generates the .h text
sub h_mode {
	print "/* This is a generated file: do not edit */

struct SEE_string;
extern struct SEE_string SEE_stringtab[];
extern const unsigned int SEE_nstringtab;

#define STR(x) (&SEE_stringtab[SEE_STR_##x])
";
	for ($i = 0; $i <= $#strings; $i++) {
	    my $ident = $strings[$i]->{'ident'};
	    print "#define SEE_STR_$ident $i\n";
	}
}

#-- generates the .c text
sub c_mode {
	print "/* This is a generated file: do not edit */

#include <see/string.h>

#define STR_SEGMENT(offset,length) \\
    { length, (SEE_char_t *)stringtext + offset, 0, 0, SEE_STRING_FLAG_STATIC }

static const SEE_char_t stringtext[] = {\n";
	my @segment = ();
	my @length = ();
	my @codepoints = ();
	my $prevlen = 0;
	for ($i = 0; $i <= $#strings; $i++) {
	    print ",\n" if $prevlen;
	    my $text = $strings[$i]->{'text'};
	    my @cp = &text_to_unicode($text);
	    push(@segment, [$#codepoints + 1, $#cp + 1]);
	    push(@codepoints, @cp);
	    print "\t".join(",", @cp);
	    $prevlen = $#cp + 1;
	}
	print "};
struct SEE_string SEE_stringtab[] = {
";
	for ($i = 0; $i <= $#strings; $i++) {
	    print ",\n" if $i;
	    my ($offset,$length) = @{$segment[$i]};
	    print "\tSTR_SEGMENT($offset, $length)";
	}
	print "};
const unsigned int SEE_nstringtab = ".($#strings + 1).";
";
}



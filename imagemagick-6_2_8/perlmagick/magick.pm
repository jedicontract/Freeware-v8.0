package Image::Magick;

# Released Feb. 17, 1997  by Kyle Shorter (magick@wizards.dupont.com)
# Public Domain

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT $AUTOLOAD);

require 5.002;
require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT =
  qw(
      Success Transparent Opaque QuantumDepth QuantumRange MaxRGB
      WarningException ResourceLimitWarning TypeWarning OptionWarning
      DelegateWarning MissingDelegateWarning CorruptImageWarning
      FileOpenWarning BlobWarning StreamWarning CacheWarning CoderWarning
      ModuleWarning DrawWarning ImageWarning XServerWarning RegistryWarning
      ConfigureWarning ErrorException ResourceLimitError TypeError
      OptionError DelegateError MissingDelegateError CorruptImageError
      FileOpenError BlobError StreamError CacheError CoderError
      ModuleError DrawError ImageError XServerError RegistryError
      ConfigureError FatalErrorException
    );

$VERSION = '6.2.8';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my $constname;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    my($pack,$file,$line) = caller;
	    die "Your vendor has not defined PerlMagick macro $pack\:\:$constname, used at $file line $line.\n";
	}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

bootstrap Image::Magick $VERSION;

# Preloaded methods go here.

sub new
{
    my $this = shift;
    my $class = ref($this) || $this || "Image::Magick";
    my $self = [ ];
    bless $self, $class;
    $self->set(@_) if @_;
    return $self;
}

sub New
{
    my $this = shift;
    my $class = ref($this) || $this || "Image::Magick";
    my $self = [ ];
    bless $self, $class;
    $self->set(@_) if @_;
    return $self;
}

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

Image::Magick - Perl extension for calling ImageMagick's libMagick methods

=head1 SYNOPSIS

  use Image::Magick;
  $p = new Image::Magick;
  $p->Read("imagefile");
  $p->Set(attribute => value, ...)
  ($a, ...) = $p->Get("attribute", ...)
  $p->routine(parameter => value, ...)
  $p->Mogrify("Routine", parameter => value, ...)
  $p->Write("filename");

=head1 DESCRIPTION

This Perl extension allows the reading, manipulation and writing of
a large number of image file formats using the ImageMagick library.
It was originally developed to be used by CGI scripts for Web pages.

A Web page has been set up for this extension. See:

	http://www.imagemagick.org/script/perl-magick.php

=head1 AUTHOR

Kyle Shorter	magick-users@imagemagick.org

=head1 BUGS

Has all the bugs of ImageMagick and much, much more!

=head1 SEE ALSO

perl(1).

=cut

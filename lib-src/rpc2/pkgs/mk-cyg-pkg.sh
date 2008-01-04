# Make a cygwin package.
#

if [ x$FULLMAKE = x ] ; then
    FULLMAKE=yes
fi

CYGWINREV=1
PKG=rpc2
FLIST="usr/bin/cygrpc2-5.dll  usr/bin/cygse-5.dll usr/bin/rp2gen.exe \
    usr/include/rpc2/errors.h usr/include/rpc2/multi.h \
    usr/include/rpc2/rpc2.h usr/include/rpc2/rpc2_addrinfo.h \
    usr/include/rpc2/se.h usr/include/rpc2/secure.h usr/include/rpc2/sftp.h \
    usr/lib/librpc2.a usr/lib/librpc2.dll.a usr/lib/librpc2.la \
    usr/lib/libse.a usr/lib/libse.dll.a usr/lib/libse.la \
    usr/lib/pkgconfig/rpc2.pc"

# Sanity checks ...

if [ `uname -o` != Cygwin ] ; then
    echo This script must be run under Cygwin
    exit 1
fi

WD=`pwd`

if [ `basename $WD` != $PKG ] ; then
   DIR=`dirname $WD`
   if [ `basename $DIR` != $PKG ] ; then
       echo This script must be started in $PKG or $PKG/pkgs
       exit 1
   fi
   cd ..
   WD=`pwd`
   if [ `basename $WD` != $PKG ] ; then
       echo This script must be started in $PKG or $PKG/pkgs
       exit 1
   fi
fi

# Get the revision number ...
function AC_INIT() { \
  REV=$2; \
}
eval `grep AC_INIT\( configure.in | tr "(,)" "   "`
if [ x$REV = x ] ; then
    echo Could not get revision number
    exit 1
fi

if [ $FULLMAKE = yes ] ; then
    echo Building $PKG-$REV cygwin binary and source packages

# Bootstrap it !

    echo Running bootstrap.sh
    if ! ./bootstrap.sh ; then
	echo "Can't bootstrap.  Stoppped."
	exit 1
    fi
    
# Build it ..
    
    if [ ! -d zobj-cygpkg ] ; then 
	if ! mkdir zobj-cygpkg ; then
	    echo Could not make build directory.
	    exit 1
	fi
    fi
    
    if ! cd zobj-cygpkg ; then
	echo Could not cd to build directory.
	exit 1
    fi
    
    if ! ../configure --prefix=/usr ; then
	echo Could not configure for build.
	exit 1
    fi
    
    if ! make ; then
	echo Could not make.
	exit 1;
    fi
    
    if ! make install ; then
	echo Could not install.
	exit 1;
    fi
    
    cd ..
fi

# strip it!

echo Stripping files.

for f in $FLIST ; do 
  if [ `basename $f` != `basename $f .exe` ] ; then
      echo Stripping /$f
      strip /$f
  fi
done

# package it

echo "Creating binary tar.bz2 file."

(cd / ; if ! tar -cjf $WD/$PKG-$REV-$CYGWINREV.tar.bz2 $FLIST ; then \
    echo Could not create the tar file. ; \
    rm $WD/$PKG-$REV-$CYGWINREV.tar.bz2 ; \
fi )
if [ ! -f $PKG-$REV-$CYGWINREV.tar.bz2 ] ; then
    exit 1;
fi

# make source tar

SRCLST=`grep / CVS/Entries  | grep -v .cvsignore | cut -d/ -f2`

echo "Creating source tar.bz2 file."

if ! mkdir $PKG-$REV-$CYGWINREV ; then
    echo Could not make new source dir.
    exit 1
fi

if ! cp -rp $SRCLST $PKG-$REV-$CYGWINREV ; then
    echo Could not copy sources.
    exit 1
fi

find $PKG-$REV-$CYGWINREV -name CVS -exec rm -rf \{\} \;

tar -cjf $PKG-$REV-$CYGWINREV-src.tar.bz2 $PKG-$REV-$CYGWINREV

# cleanup

echo Cleaning ...

rm -rf $PKG-$REV-$CYGWINREV zobj-cygpkg

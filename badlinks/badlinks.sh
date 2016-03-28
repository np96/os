#! /bin/bash
touch tempfile
touch -m -d '1 week ago' tempfile
linkcheck() {
for i in $1/*; do
    if [ ! -e "$i" -a -h "$i" -a "$tempfile" -nt "$i" ]
    then echo "$i"
    fi
    [ -d "$i" ] && linkcheck $i
done
}
directory=${1-`pwd`}
linkcheck $directory
rm -f tempfile

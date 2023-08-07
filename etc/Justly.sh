#!/bin/sh
bin_dir=`dirname $0`
prefix=`dirname $bin_dir`
OPCODE6DIR64="$prefix/plugins" $bin_dir/Justly "$@"
#!/bin/sh
bin_dir=`dirname $0`
OPCODE6DIR64="$bin_dir/csound_plugins" AUDIO_DRIVER="pulse" $bin_dir/Justly "$@"
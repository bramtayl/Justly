#!/bin/sh
bin=`dirname $0`
LD_LIBRARY_PATH=$bin/../lib OPCODE6DIR64=$bin/csound_plugins rtaudio="pa" $bin/Justly "$@"
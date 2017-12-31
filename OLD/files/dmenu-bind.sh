#!/bin/bash
exe=`dmenu_run -b -fn 'Fantasque Sans Mono-14' -nb '#151617' -nf '#d8d8d8' -sb '#d8d8d8' -sf '#151617'` && eval "exec $exe"
#exe=`dmenu_run -b -fn 'Fantasque Sans Mono-14' -nb '#151617' -nf '#d8d8d8' -sb '#d8d8d8' -sf '#151617'`
#if [ "${exe}" == "vim" ]; then
#  eval "exec terminator -e $exe"
#else
#  eval "exec $exe"
#fi

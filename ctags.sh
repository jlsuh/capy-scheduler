#!/bin/sh
ctags -o - --kinds-C=f -x --_xformat="%{typeref} %{name}%{signature};" $1 | tr ':' ' ' | sed -e 's/^typename //'

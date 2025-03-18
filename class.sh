#!/usr/bin/bash

if [[ ! -z $1 && ! -z $2 ]]
then
    if [ ! -d "src/$1" ]
    then
        mkdir "src/$1"
    fi
    if [ ! -d "include/$1" ]
    then
        mkdir "include/$1"
    fi

    if [ ! -f "src/${1}/${2}.cpp" ]
    then
        touch "src/${1}/${2}.cpp"
        echo -e "#include \"stdafx.hpp\"\n#include \"$1/$2.hpp\"\n" > "src/${1}/${2}.cpp"
    fi

    if [ ! -f "include/${1}/${2}.hpp" ]
    then
        touch "include/${1}/${2}.hpp"
        echo -e "#pragma once" > "include/${1}/${2}.hpp"
    fi

else
    echo "Usage: <folder> <Class>"
fi

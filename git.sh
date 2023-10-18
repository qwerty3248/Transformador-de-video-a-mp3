#!/bin/bash

git add *

git commit -m "$1"


if [ $2==1 ]; then

	git push

fi


git status



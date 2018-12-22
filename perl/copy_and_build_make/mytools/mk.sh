#!/bin/bash
clear
perl -w "./mytools/make.pl" $* 2>&1 |tee ~make.log
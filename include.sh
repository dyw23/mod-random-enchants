#!/usr/bin/env bash
	
	MOD_RANDOM_ENCHANTS_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/" && pwd )"
	
	source $MOD_RANDOM_ENCHANTS_ROOT"/conf/conf.sh.dist"
	
	if [ -f $MOD_RANDOM_ENCHANTS_ROOT"/conf/conf.sh" ]; then
	    source $MOD_RANDOM_ENCHANTS_ROOT"/conf/conf.sh"
	fi

#!/bin/bash

GREEN=$(tput setaf 10)
YELLOW=$(tput setaf 11)
NONE=$(tput sgr0)

update_submodule()
{
  echo "${GREEN}${1}${NONE}: '${YELLOW}git -C $1 submodule update --init --recursive${NONE}'"
  git -C $1 submodule update --init --recursive || exit 2
}

update_submodule "imgui"
exit 0
